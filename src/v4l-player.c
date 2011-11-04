/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

/*  libgtk2.0-dev */
/* Compile with: gcc v4ltest.c `pkg-config --libs --cflags gtk+-2.0` */

/* TODO: clean code, use gtk_image_set_from_pixbuf() */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>

static GtkWidget *window;
static GtkWidget *image;

struct buffer {
        void *start;
        size_t length;
};

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit (EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
        int r;

        assert(arg);

        do r = ioctl(fd, request, arg);
        while(r < 0 && EINTR == errno);

        return r;
}

#define CLIP(color) (unsigned char) (((color) > 0xFF) ?                   \
                                    0xff : (((color) < 0) ? 0 : (color)))

void v4lconvert_yuyv_to_rgb24(const unsigned char *src, unsigned char *dest,
                              int width, int height)
{
        /* From: Hans de Goede <j.w.r.degoede@hhs.nl> */
        int j;

        while(--height >= 0) {
                for(j = 0; j < width; j += 2) {
                        int u = src[1];
                        int v = src[3];
                        int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
                        int rg = (((u - 128) << 1) +  (u - 128) +
                                 ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
                        int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;

                        *dest++ = CLIP(src[0] + v1);
                        *dest++ = CLIP(src[0] - rg);
                        *dest++ = CLIP(src[0] + u1);

                        *dest++ = CLIP(src[2] + v1);
                        *dest++ = CLIP(src[2] - rg);
                        *dest++ = CLIP(src[2] + u1);

                        src += 4;
                }
        }
}

static void convert_v4l_image_and_display(unsigned char *img, size_t len)
{
        unsigned char img2[640*480*3] = {0};
        v4lconvert_yuyv_to_rgb24(img, img2, 640, 480);

        GdkPixbuf *pb = gdk_pixbuf_new_from_data(img2, GDK_COLORSPACE_RGB,
                                                 FALSE, 24/3, 640, 480, 640*3,
                                                 NULL, NULL);
        if(image != NULL)
                gtk_container_remove(GTK_CONTAINER(window), image);
        image = gtk_image_new_from_pixbuf(pb);
        gtk_container_add(GTK_CONTAINER(window), image);
        gtk_widget_show_all(window);
}

static void process_v4l_image(unsigned char *img, size_t len)
{
        assert(img);
        convert_v4l_image_and_display(img, len);
}

static int read_v4l_frame(int fd, unsigned int n_buffers,
                          struct buffer *buffers)
{
        struct v4l2_buffer buf;

        assert(buffers);

        memset(&buf, 0, sizeof(buf));

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if(xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
                switch(errno) {
                case EAGAIN:
                        return 0;
                case EIO:
                default:
                        errno_exit("VIDIOC_DQBUF");
                }
        }

        assert(buf.index < n_buffers);

        process_v4l_image(buffers[buf.index].start, buffers[buf.index].length);
        if(xioctl(fd, VIDIOC_QBUF, &buf) < 0)
                errno_exit("VIDIOC_QBUF");

        return 1;
}

static int open_v4l_device(const char *dev_name)
{
        int fd;
        struct stat st;

        assert(dev_name);

        if(stat(dev_name, &st) < 0) {
                fprintf(stderr, "Cannot identify %s: %d, %s\n",
                        dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if(!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open (dev_name, O_RDWR | O_NONBLOCK, 0);
        if(fd < 0) {
                fprintf(stderr, "Cannot open %s: %d, %s\n",
                        dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        return fd;
}

static void close_vl4_device(int fd)
{
        if(fd < 0)
                return;
        close(fd);
}

static int start_v4l_capturing(int fd, unsigned int n_buffers)
{
        unsigned int i;
        enum v4l2_buf_type type;

        for(i = 0; i < n_buffers; ++i) {
                struct v4l2_buffer buf;

                memset(&buf, 0, sizeof(buf));

                buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index  = i;

                if(xioctl(fd, VIDIOC_QBUF, &buf) < 0)
                        errno_exit("VIDIOC_QBUF");
        }
       
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if(xioctl(fd, VIDIOC_STREAMON, &type) < 0)
                errno_exit("VIDIOC_STREAMON");

        return 0;
}

static int stop_v4l_capturing(int fd)
{
        enum v4l2_buf_type type;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if(xioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
                errno_exit("VIDIOC_STREAMOFF");

        return 0;
}

static int init_mmap(int fd, const char *dev_name, unsigned int *n_buffers,
                     struct buffer **buffers)
{
        struct v4l2_requestbuffers req;

        assert(buffers);
        assert(n_buffers);
        assert(dev_name);

        memset(&req, 0, sizeof(req));

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if(xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
                if(EINVAL == errno) {
                        fprintf(stderr, "%s does not support memory mapping\n",
                                dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if(req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\n",
                        dev_name);
                exit(EXIT_FAILURE);
        }

        (*buffers) = calloc(req.count, sizeof (**buffers));
        if(!(*buffers)) {
                fprintf (stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for((*n_buffers) = 0; (*n_buffers) < req.count; ++(*n_buffers)) {
                struct v4l2_buffer buf;

                memset(&buf, 0, sizeof(buf));

                buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index  = (*n_buffers);

                if(xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
                        errno_exit("VIDIOC_QUERYBUF");

                (*buffers)[(*n_buffers)].length = buf.length;
                (*buffers)[(*n_buffers)].start = mmap(NULL, buf.length,
                                                   PROT_READ | PROT_WRITE,
                                                   MAP_SHARED, fd, buf.m.offset);
                if(MAP_FAILED == (*buffers)[(*n_buffers)].start)
                        errno_exit("mmap");
        }
       
        return 0;
}

static int init_v4l_device(int fd, const char *dev_name, unsigned int *n_buffers,
                           struct buffer **buffers)
{
        unsigned int min;

        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;

        if(xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
                if(EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n", dev_name);
                exit(EXIT_FAILURE);
        }

        if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
                exit(EXIT_FAILURE);
        }

        memset(&cropcap, 0, sizeof(cropcap));

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if(xioctl(fd, VIDIOC_CROPCAP, &cropcap) == 0) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c    = cropcap.defrect;

                if(xioctl(fd, VIDIOC_S_CROP, &crop) < 0) {
                        switch(errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {        
                /* Errors ignored. */
        }

        memset(&fmt, 0, sizeof(fmt));

        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = 640;//640;
        fmt.fmt.pix.height      = 480;//480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

        if(xioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
                errno_exit("VIDIOC_S_FMT");

        /* XXX: VIDIOC_S_FMT may change width and height. */

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if(fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if(fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        return init_mmap(fd, dev_name, n_buffers, buffers);
}

static int cleanup_v4l_device(int fd, unsigned int n_buffers,
                              struct buffer *buffers)
{
        unsigned int i;

        for(i = 0; i < n_buffers; ++i)
                if(munmap(buffers[i].start, buffers[i].length) < 0)
                        errno_exit("munmap");
        free(buffers);
        return 0;
}

static void destroy(void) {
        gtk_main_quit();
}

/* XXX: Remove here */
int fd;
unsigned int v4l_n_buffers;
struct buffer *v4l_buffers;

gboolean update_v4l_image(gpointer null)
{
        read_v4l_frame(fd, v4l_n_buffers, v4l_buffers);
        return TRUE;
}

int main(int argc, char **argv)
{
        const char *dev_name = "/dev/video0";

        gtk_init(&argc, &argv);

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "V4L Player");
        gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

        gtk_signal_connect(GTK_OBJECT(window), "destroy",
                           GTK_SIGNAL_FUNC(destroy), NULL);

        fd = open_v4l_device(dev_name);
        init_v4l_device(fd, dev_name, &v4l_n_buffers, &v4l_buffers);
        start_v4l_capturing(fd, v4l_n_buffers);

        gint func_ref = g_timeout_add(10, update_v4l_image, NULL);
        gtk_widget_show_all(window);
        gtk_main();
        g_source_remove(func_ref);

        stop_v4l_capturing(fd);
        cleanup_v4l_device(fd, v4l_n_buffers, v4l_buffers);
        close_vl4_device(fd);

        return 0;
}

