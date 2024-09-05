
#ifndef XPMP2_EXPORT_H
#define XPMP2_EXPORT_H

#ifdef XPMP2_STATIC_DEFINE
#  define XPMP2_EXPORT
#  define XPMP2_NO_EXPORT
#else
#  ifndef XPMP2_EXPORT
#    ifdef XPMP2_EXPORTS
        /* We are building this library */
#      define XPMP2_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define XPMP2_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef XPMP2_NO_EXPORT
#    define XPMP2_NO_EXPORT 
#  endif
#endif

#ifndef XPMP2_DEPRECATED
#  define XPMP2_DEPRECATED __declspec(deprecated)
#endif

#ifndef XPMP2_DEPRECATED_EXPORT
#  define XPMP2_DEPRECATED_EXPORT XPMP2_EXPORT XPMP2_DEPRECATED
#endif

#ifndef XPMP2_DEPRECATED_NO_EXPORT
#  define XPMP2_DEPRECATED_NO_EXPORT XPMP2_NO_EXPORT XPMP2_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef XPMP2_NO_DEPRECATED
#    define XPMP2_NO_DEPRECATED
#  endif
#endif

#endif /* XPMP2_EXPORT_H */
