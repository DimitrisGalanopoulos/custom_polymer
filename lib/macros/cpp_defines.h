#ifndef CPP_DEFINES_H
#define CPP_DEFINES_H


#ifdef __cplusplus

/*
 * Replace C idioms with equivalent of C++.
 */

/* 
 * __cplusplus
 *     This macro is defined when the C++ compiler is in use.
 *     You can use __cplusplus to test whether a header is compiled by a C compiler or a C++ compiler.
 *     This macro is similar to __STDC_VERSION__, in that it expands to a version number. 
 */
	#include <type_traits>

	#define __auto_type  auto
	#define typeof(t)  std::decay<decltype(t)>::type

	#define static_cast(type, expression)  static_cast<type>(expression)

#else

	#define static_cast(type, expression)  (type) expression

#endif /* __cplusplus */



#undef alloc

#define alloc(n, ptr)                                                      \
do {                                                                       \
	ptr = static_cast(typeof(ptr), malloc((n) * sizeof(*(ptr))));      \
} while(0)



#endif /* CPP_DEFINES_H */

