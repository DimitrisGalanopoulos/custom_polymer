#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
--------------------------------------------------------------------------------------------------------------------------------------------
-                                                               Attributes                                                                 -
--------------------------------------------------------------------------------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#define __inline__  inline __attribute__((always_inline))

#ifndef CACHE_LINE_SIZE
	#define CACHE_LINE_SIZE  64
#endif
#define __cache_aligned__  __attribute__((aligned (CACHE_LINE_SIZE)))


#define __silence_Wunused__  __attribute__((unused))


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
--------------------------------------------------------------------------------------------------------------------------------------------
-                                                              GCC Builtins                                                                -
--------------------------------------------------------------------------------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#define likely(x)    __builtin_expect((x),1)
#define unlikely(x)  __builtin_expect((x),0)


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
--------------------------------------------------------------------------------------------------------------------------------------------
-                                                             Various Helpers                                                              -
--------------------------------------------------------------------------------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


// Intermediate step to expand arguments before calling "macro" with them (e.g. for concatenation ##).
#define EXPAND_ARGS_AND_CALL(macro, args...) macro(args)


#define _STRING(...) #__VA_ARGS__
#define STRING(...) _STRING(__VA_ARGS__)


#define _CONCAT(a, b) a ## b
#define CONCAT(a, b) _CONCAT(a, b)


// __VA_OPT__ replacement (almost, can't use the comma ',' with it).
#define _OPT_ID(...) __VA_ARGS__
#define _OPT_SINK(...)
#define _OPT(optional, fun, ...)  fun(optional)
#define OPT(optional, ...)  _OPT(optional, _OPT_ID, ##__VA_ARGS__ (_OPT_SINK))
#define _OPT_NEG(optional, fun, ...)  fun(optional)
#define OPT_NEG(optional, ...)  _OPT(optional, _OPT_SINK, ##__VA_ARGS__ ()_OPT_ID)


// Pack args in a tuple. 
#define PACK(...) (__VA_ARGS__)

// Unpack tuple as arg list, do nothing when not a tuple.
#define _UNPACK_ID(...)  __VA_ARGS__
#define _UNPACK_TEST_IF_TUPLE(...)  /* one */, /* two */
#define _UNPACK(args, one, ...) OPT(_UNPACK_ID, ##__VA_ARGS__) args
// Always passed as only 2 arguments, so we pass them again.
#define _UNPACK_EXPAND_ARGS(...) _UNPACK(__VA_ARGS__)
// When 'tuple' is a tuple '_UNPACK_TEST_IF_TUPLE' is called and it returns two arguments.
// When 'tuple' is not a tuple it is not called, and the two are counted as one argument (no commas).
#define UNPACK(tuple) _UNPACK_EXPAND_ARGS(tuple, _UNPACK_TEST_IF_TUPLE tuple)


// This is usefull for the _Pragma operator, which expects a single string literal, when we want to parameterize the string.
// We pass the argument unstringized, like in a #pragma directive.
#define PRAGMA(...) _Pragma(STRING(__VA_ARGS__))


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
--------------------------------------------------------------------------------------------------------------------------------------------
-                                                              Common Macros                                                               -
--------------------------------------------------------------------------------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#ifndef SWAP
#define SWAP(a, b)               \
do {                             \
	__auto_type __tmp = a;   \
	a = b;                   \
	b = __tmp;               \
} while (0)
#endif


#ifndef MIN
#define MIN(a, b)                  \
({                                 \
	__auto_type __a = a;       \
	__auto_type __b = b;       \
	(__a < __b) ? __a : __b;   \
})
#endif


#ifndef MAX
#define MAX(a, b)                  \
({                                 \
	__auto_type __a = a;       \
	__auto_type __b = b;       \
	(__a > __b) ? __a : __b;   \
})
#endif



#endif /* COMMON_MACROS_H */


