/*
 * transsip - the telephony network
 * By Daniel Borkmann <daniel@transsip.org>
 * Copyright 2011, 2012 Daniel Borkmann <dborkma@tik.ee.ethz.ch>,
 * Swiss federal institute of technology (ETH Zurich)
 * Subject to the GPL, version 2.
 */

#ifndef BUILT_IN_H
#define BUILT_IN_H

#ifndef __aligned_16
# define __aligned_16		__attribute__((aligned(16)))
#endif

#ifndef likely
# define likely(x)		__builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
# define unlikely(x)		__builtin_expect(!!(x), 0)
#endif

#ifndef __extension__
# define __extension__
#endif

#ifndef __deprecated
# define __deprecated		/* unimplemented */
#endif

#ifndef __read_mostly
# define __read_mostly		__attribute__((__section__(".data.read_mostly")))
#endif

#ifndef __always_inline
# define __always_inline	inline
#endif

#ifndef __hidden
# define __hidden		__attribute__((visibility("hidden")))
#endif

/* From Linux */
#ifndef array_size
# define array_size(x)		(sizeof(x) / sizeof((x)[0]) + __must_be_array(x))
#endif

#ifndef __must_be_array
# define __must_be_array(x)		\
	build_bug_on_zero(__builtin_types_compatible_p(typeof(x), typeof(&x[0])))
#endif

#ifndef max
# define max(a, b)			\
	({				\
		typeof (a) _a = (a);	\
		typeof (b) _b = (b);	\
		_a > _b ? _a : _b;	\
	})
#endif

#ifndef min
# define min(a, b)			\
	({				\
		typeof (a) _a = (a);	\
		typeof (b) _b = (b);	\
		_a < _b ? _a : _b;	\
	})
#endif

#ifndef bug
# define bug()			__builtin_trap()
#endif

#ifndef build_bug_on_zero
# define build_bug_on_zero(e)	(sizeof(char[1 - 2 * !!(e)]) - 1)
#endif

#endif /* BUILT_IN_H */
