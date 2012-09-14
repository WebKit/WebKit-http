/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_Platform_h
#define WTF_Platform_h

/* Include compiler specific macros */
#include <wtf/Compiler.h>

/* ==== PLATFORM handles OS, operating environment, graphics API, and
   CPU. This macro will be phased out in favor of platform adaptation
   macros, policy decision macros, and top-level port definitions. ==== */
#define PLATFORM(WTF_FEATURE) (defined WTF_PLATFORM_##WTF_FEATURE  && WTF_PLATFORM_##WTF_FEATURE)


/* ==== Platform adaptation macros: these describe properties of the target environment. ==== */

/* CPU() - the target CPU architecture */
#define CPU(WTF_FEATURE) (defined WTF_CPU_##WTF_FEATURE  && WTF_CPU_##WTF_FEATURE)
/* HAVE() - specific system features (headers, functions or similar) that are present or not */
#define HAVE(WTF_FEATURE) (defined HAVE_##WTF_FEATURE  && HAVE_##WTF_FEATURE)
/* OS() - underlying operating system; only to be used for mandated low-level services like 
   virtual memory, not to choose a GUI toolkit */
#define OS(WTF_FEATURE) (defined WTF_OS_##WTF_FEATURE  && WTF_OS_##WTF_FEATURE)


/* ==== Policy decision macros: these define policy choices for a particular port. ==== */

/* USE() - use a particular third-party library or optional OS service */
#define USE(WTF_FEATURE) (defined WTF_USE_##WTF_FEATURE  && WTF_USE_##WTF_FEATURE)
/* ENABLE() - turn on a specific feature of WebKit */
#define ENABLE(WTF_FEATURE) (defined ENABLE_##WTF_FEATURE  && ENABLE_##WTF_FEATURE)


/* ==== CPU() - the target CPU architecture ==== */

/* This also defines CPU(BIG_ENDIAN) or CPU(MIDDLE_ENDIAN) or neither, as appropriate. */

/* CPU(ALPHA) - DEC Alpha */
#if defined(__alpha__)
#define WTF_CPU_ALPHA 1
#endif

/* CPU(IA64) - Itanium / IA-64 */
#if defined(__ia64__)
#define WTF_CPU_IA64 1
/* 32-bit mode on Itanium */
#if !defined(__LP64__)
#define WTF_CPU_IA64_32 1
#endif
#endif

/* CPU(MIPS) - MIPS 32-bit */
/* Note: Only O32 ABI is tested, so we enable it for O32 ABI for now.  */
#if (defined(mips) || defined(__mips__) || defined(MIPS) || defined(_MIPS_)) \
    && defined(_ABIO32)
#define WTF_CPU_MIPS 1
#if defined(__MIPSEB__)
#define WTF_CPU_BIG_ENDIAN 1
#endif
#define WTF_MIPS_PIC (defined __PIC__)
#define WTF_MIPS_ARCH __mips
#define WTF_MIPS_ISA(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH == v)
#define WTF_MIPS_ISA_AT_LEAST(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH >= v)
#define WTF_MIPS_ARCH_REV __mips_isa_rev
#define WTF_MIPS_ISA_REV(v) (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == v)
#define WTF_MIPS_DOUBLE_FLOAT (defined __mips_hard_float && !defined __mips_single_float)
#define WTF_MIPS_FP64 (defined __mips_fpr && __mips_fpr == 64)
/* MIPS requires allocators to use aligned memory */
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif /* MIPS */

/* CPU(PPC) - PowerPC 32-bit */
#if   defined(__ppc__)     \
    || defined(__PPC__)     \
    || defined(__powerpc__) \
    || defined(__powerpc)   \
    || defined(__POWERPC__) \
    || defined(_M_PPC)      \
    || defined(__PPC)
#define WTF_CPU_PPC 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(PPC64) - PowerPC 64-bit */
#if   defined(__ppc64__) \
    || defined(__PPC64__)
#define WTF_CPU_PPC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SH4) - SuperH SH-4 */
#if defined(__SH4__)
#define WTF_CPU_SH4 1
#endif

/* CPU(SPARC32) - SPARC 32-bit */
#if defined(__sparc) && !defined(__arch64__) || defined(__sparcv8)
#define WTF_CPU_SPARC32 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SPARC64) - SPARC 64-bit */
#if defined(__sparc__) && defined(__arch64__) || defined (__sparcv9)
#define WTF_CPU_SPARC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SPARC) - any SPARC, true for CPU(SPARC32) and CPU(SPARC64) */
#if CPU(SPARC32) || CPU(SPARC64)
#define WTF_CPU_SPARC 1
#endif

/* CPU(S390X) - S390 64-bit */
#if defined(__s390x__)
#define WTF_CPU_S390X 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(S390) - S390 32-bit */
#if defined(__s390__)
#define WTF_CPU_S390 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(X86) - i386 / x86 32-bit */
#if   defined(__i386__) \
    || defined(i386)     \
    || defined(_M_IX86)  \
    || defined(_X86_)    \
    || defined(__THW_INTEL)
#define WTF_CPU_X86 1
#endif

/* CPU(X86_64) - AMD64 / Intel64 / x86_64 64-bit */
#if   defined(__x86_64__) \
    || defined(_M_X64)
#define WTF_CPU_X86_64 1
#endif

/* CPU(ARM) - ARM, any version*/
#if   defined(arm) \
    || defined(__arm__) \
    || defined(ARM) \
    || defined(_ARM_)
#define WTF_CPU_ARM 1

#if defined(__ARM_PCS_VFP)
#define WTF_CPU_ARM_HARDFP 1
#endif

#if defined(__ARMEB__) || (COMPILER(RVCT) && defined(__BIG_ENDIAN))
#define WTF_CPU_BIG_ENDIAN 1

#elif !defined(__ARM_EABI__) \
    && !defined(__EABI__) \
    && !defined(__VFP_FP__) \
    && !defined(_WIN32_WCE) \
    && !defined(ANDROID)
#define WTF_CPU_MIDDLE_ENDIAN 1

#endif

#define WTF_ARM_ARCH_AT_LEAST(N) (CPU(ARM) && WTF_ARM_ARCH_VERSION >= N)

/* Set WTF_ARM_ARCH_VERSION */
#if   defined(__ARM_ARCH_4__) \
    || defined(__ARM_ARCH_4T__) \
    || defined(__MARM_ARMV4__) \
    || defined(_ARMV4I_)
#define WTF_ARM_ARCH_VERSION 4

#elif defined(__ARM_ARCH_5__) \
    || defined(__ARM_ARCH_5T__) \
    || defined(__MARM_ARMV5__)
#define WTF_ARM_ARCH_VERSION 5

#elif defined(__ARM_ARCH_5E__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_ARM_ARCH_VERSION 5
/*ARMv5TE requires allocators to use aligned memory*/
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1

#elif defined(__ARM_ARCH_6__) \
    || defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6T2__) \
    || defined(__ARMV6__)
#define WTF_ARM_ARCH_VERSION 6

#elif defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7R__)
#define WTF_ARM_ARCH_VERSION 7

/* RVCT sets _TARGET_ARCH_ARM */
#elif defined(__TARGET_ARCH_ARM)
#define WTF_ARM_ARCH_VERSION __TARGET_ARCH_ARM

#if defined(__TARGET_ARCH_5E) \
    || defined(__TARGET_ARCH_5TE) \
    || defined(__TARGET_ARCH_5TEJ)
/*ARMv5TE requires allocators to use aligned memory*/
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif

#else
#define WTF_ARM_ARCH_VERSION 0

#endif

/* Set WTF_THUMB_ARCH_VERSION */
#if   defined(__ARM_ARCH_4T__)
#define WTF_THUMB_ARCH_VERSION 1

#elif defined(__ARM_ARCH_5T__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_THUMB_ARCH_VERSION 2

#elif defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6M__)
#define WTF_THUMB_ARCH_VERSION 3

#elif defined(__ARM_ARCH_6T2__) \
    || defined(__ARM_ARCH_7__) \
    || defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7R__) \
    || defined(__ARM_ARCH_7M__)
#define WTF_THUMB_ARCH_VERSION 4

/* RVCT sets __TARGET_ARCH_THUMB */
#elif defined(__TARGET_ARCH_THUMB)
#define WTF_THUMB_ARCH_VERSION __TARGET_ARCH_THUMB

#else
#define WTF_THUMB_ARCH_VERSION 0
#endif


/* CPU(ARMV5_OR_LOWER) - ARM instruction set v5 or earlier */
/* On ARMv5 and below the natural alignment is required. 
   And there are some other differences for v5 or earlier. */
#if !defined(ARMV5_OR_LOWER) && !WTF_ARM_ARCH_AT_LEAST(6)
#define WTF_CPU_ARMV5_OR_LOWER 1
#endif


/* CPU(ARM_TRADITIONAL) - Thumb2 is not available, only traditional ARM (v4 or greater) */
/* CPU(ARM_THUMB2) - Thumb2 instruction set is available */
/* Only one of these will be defined. */
#if !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2)
#  if defined(thumb2) || defined(__thumb2__) \
    || ((defined(__thumb) || defined(__thumb__)) && WTF_THUMB_ARCH_VERSION == 4)
#    define WTF_CPU_ARM_TRADITIONAL 0
#    define WTF_CPU_ARM_THUMB2 1
#  elif WTF_ARM_ARCH_AT_LEAST(4)
#    define WTF_CPU_ARM_TRADITIONAL 1
#    define WTF_CPU_ARM_THUMB2 0
#  else
#    error "Not supported ARM architecture"
#  endif
#elif CPU(ARM_TRADITIONAL) && CPU(ARM_THUMB2) /* Sanity Check */
#  error "Cannot use both of WTF_CPU_ARM_TRADITIONAL and WTF_CPU_ARM_THUMB2 platforms"
#endif /* !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2) */

#if defined(__ARM_NEON__) && !defined(WTF_CPU_ARM_NEON)
#define WTF_CPU_ARM_NEON 1
#endif

#if CPU(ARM_NEON) && (!COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 7, 0))
// All NEON intrinsics usage can be disabled by this macro.
#define HAVE_ARM_NEON_INTRINSICS 1
#endif

#endif /* ARM */

#if CPU(ARM) || CPU(MIPS) || CPU(SH4) || CPU(SPARC)
#define WTF_CPU_NEEDS_ALIGNED_ACCESS 1
#endif

/* ==== OS() - underlying operating system; only to be used for mandated low-level services like 
   virtual memory, not to choose a GUI toolkit ==== */

/* OS(ANDROID) - Android */
#ifdef ANDROID
#define WTF_OS_ANDROID 1
#endif

/* OS(AIX) - AIX */
#ifdef _AIX
#define WTF_OS_AIX 1
#endif

/* OS(DARWIN) - Any Darwin-based OS, including Mac OS X and iPhone OS */
#ifdef __APPLE__
#define WTF_OS_DARWIN 1

#include <Availability.h>
#include <AvailabilityMacros.h>
#include <TargetConditionals.h>
#endif

/* OS(IOS) - iOS */
/* OS(MAC_OS_X) - Mac OS X (not including iOS) */
#if OS(DARWIN) && ((defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) \
    || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)                 \
    || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR))
#define WTF_OS_IOS 1
#elif OS(DARWIN) && defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define WTF_OS_MAC_OS_X 1

/* FIXME: These can be removed after sufficient time has passed since the removal of BUILDING_ON / TARGETING macros. */

#define ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED 0 / 0
#define ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED 0 / 0

#define BUILDING_ON_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED
#define BUILDING_ON_SNOW_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED
#define BUILDING_ON_LION ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED

#define TARGETING_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#define TARGETING_SNOW_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#define TARGETING_LION ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#endif

/* OS(FREEBSD) - FreeBSD */
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#define WTF_OS_FREEBSD 1
#endif

/* OS(HURD) - GNU/Hurd */
#ifdef __GNU__
#define WTF_OS_HURD 1
#endif

/* OS(LINUX) - Linux */
#ifdef __linux__
#define WTF_OS_LINUX 1
#endif

/* OS(NETBSD) - NetBSD */
#if defined(__NetBSD__)
#define WTF_OS_NETBSD 1
#endif

/* OS(OPENBSD) - OpenBSD */
#ifdef __OpenBSD__
#define WTF_OS_OPENBSD 1
#endif

/* OS(QNX) - QNX */
#if defined(__QNXNTO__)
#define WTF_OS_QNX 1
#endif

/* OS(SOLARIS) - Solaris */
#if defined(sun) || defined(__sun)
#define WTF_OS_SOLARIS 1
#endif

/* OS(WINCE) - Windows CE; note that for this platform OS(WINDOWS) is also defined */
#if defined(_WIN32_WCE)
#define WTF_OS_WINCE 1
#endif

/* OS(WINDOWS) - Any version of Windows */
#if defined(WIN32) || defined(_WIN32)
#define WTF_OS_WINDOWS 1
#endif

#define WTF_OS_WIN ERROR "USE WINDOWS WITH OS NOT WIN"
#define WTF_OS_MAC ERROR "USE MAC_OS_X WITH OS NOT MAC"

/* OS(UNIX) - Any Unix-like system */
#if   OS(AIX)              \
    || OS(ANDROID)          \
    || OS(DARWIN)           \
    || OS(FREEBSD)          \
    || OS(HURD)             \
    || OS(LINUX)            \
    || OS(NETBSD)           \
    || OS(OPENBSD)          \
    || OS(QNX)              \
    || OS(SOLARIS)          \
    || defined(unix)        \
    || defined(__unix)      \
    || defined(__unix__)
#define WTF_OS_UNIX 1
#endif

/* Operating environments */

/* FIXME: these are all mixes of OS, operating environment and policy choices. */
/* PLATFORM(CHROMIUM) */
/* PLATFORM(QT) */
/* PLATFORM(WX) */
/* PLATFORM(GTK) */
/* PLATFORM(BLACKBERRY) */
/* PLATFORM(MAC) */
/* PLATFORM(WIN) */
#if defined(BUILDING_CHROMIUM__)
#define WTF_PLATFORM_CHROMIUM 1
#elif defined(BUILDING_QT__)
#define WTF_PLATFORM_QT 1
#elif defined(BUILDING_WX__)
#define WTF_PLATFORM_WX 1
#elif defined(BUILDING_GTK__)
#define WTF_PLATFORM_GTK 1
#elif defined(BUILDING_BLACKBERRY__)
#define WTF_PLATFORM_BLACKBERRY 1
#elif OS(DARWIN)
#define WTF_PLATFORM_MAC 1
#elif OS(WINDOWS)
#define WTF_PLATFORM_WIN 1
#endif

/* PLATFORM(IOS) */
/* FIXME: this is sometimes used as an OS switch and sometimes for higher-level things */
#if (defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define WTF_PLATFORM_IOS 1
#endif

/* PLATFORM(IOS_SIMULATOR) */
#if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define WTF_PLATFORM_IOS 1
#define WTF_PLATFORM_IOS_SIMULATOR 1
#endif

/* Graphics engines */

/* USE(CG) and PLATFORM(CI) */
#if PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_CG 1
#endif
#if PLATFORM(MAC) || PLATFORM(IOS) || (PLATFORM(WIN) && USE(CG))
#define WTF_USE_CA 1
#endif

/* USE(SKIA) for Win/Linux/Mac/Android */
#if PLATFORM(CHROMIUM)
#if OS(DARWIN)
#define WTF_USE_SKIA 1
#define WTF_USE_ICCJPEG 1
#define WTF_USE_QCMSLIB 1
#elif OS(ANDROID)
#define WTF_USE_SKIA 1
#define WTF_USE_LOW_QUALITY_IMAGE_INTERPOLATION 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_DITHERING 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_FANCY_UPSAMPLING 1
#else
#define WTF_USE_SKIA 1
#define WTF_USE_ICCJPEG 1
#define WTF_USE_QCMSLIB 1
#endif
#endif

#if OS(QNX)
#define USE_SYSTEM_MALLOC 1
#endif

#if PLATFORM(BLACKBERRY)
#define WTF_USE_MERSENNE_TWISTER_19937 1
#define WTF_USE_SKIA 1
#define WTF_USE_LOW_QUALITY_IMAGE_INTERPOLATION 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_DITHERING 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_FANCY_UPSAMPLING 1
#endif

#if PLATFORM(GTK)
#define WTF_USE_CAIRO 1
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#endif


#if OS(WINCE)
#define WTF_USE_MERSENNE_TWISTER_19937 1
#endif

/* On Windows, use QueryPerformanceCounter by default */
#if OS(WINDOWS)
#define WTF_USE_QUERY_PERFORMANCE_COUNTER  1
#endif

#if OS(WINCE) && !PLATFORM(QT)
#define NOSHLWAPI      /* shlwapi.h not available on WinCe */

/* MSDN documentation says these functions are provided with uspce.lib.  But we cannot find this file. */
#define __usp10__      /* disable "usp10.h" */

#define _INC_ASSERT    /* disable "assert.h" */
#define assert(x)

#endif  /* OS(WINCE) && !PLATFORM(QT) */

#if OS(WINCE) && !PLATFORM(QT)
#define WTF_USE_WCHAR_UNICODE 1
#elif PLATFORM(GTK)
/* The GTK+ Unicode backend is configurable */
#else
#define WTF_USE_ICU_UNICODE 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS)
#if CPU(X86_64)
#define WTF_USE_PLUGIN_HOST_PROCESS 1
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#define ENABLE_GESTURE_EVENTS 1
#define ENABLE_RUBBER_BANDING 1
#define WTF_USE_SCROLLBAR_PAINTER 1
#define HAVE_XPC 1
#endif
#if !defined(ENABLE_JAVA_BRIDGE)
#define ENABLE_JAVA_BRIDGE 1
#endif
#if !defined(ENABLE_DASHBOARD_SUPPORT)
#define ENABLE_DASHBOARD_SUPPORT 1
#endif
#define WTF_USE_CF 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#define HAVE_READLINE 1
#define HAVE_RUNLOOP_TIMER 1
#define ENABLE_FULLSCREEN_API 1
#define ENABLE_SMOOTH_SCROLLING 1
#define ENABLE_WEB_ARCHIVE 1
#define ENABLE_WEB_AUDIO 1
#if defined(ENABLE_VIDEO)
#define ENABLE_VIDEO_TRACK 1
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define HAVE_LAYER_HOSTING_IN_WINDOW_SERVER 1
#endif
#define WTF_USE_APPKIT 1
#define WTF_USE_SECURITY_FRAMEWORK 1
#endif /* PLATFORM(MAC) && !PLATFORM(IOS) */

#if PLATFORM(CHROMIUM) && OS(DARWIN)
#define WTF_USE_CF 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1

#define WTF_USE_WK_SCROLLBAR_PAINTER 1
#endif

#if PLATFORM(IOS)
#define DONT_FINALIZE_ON_MAIN_THREAD 1
#endif

#if PLATFORM(QT) && OS(DARWIN)
#define WTF_USE_CF 1
#endif

#if OS(DARWIN) && !PLATFORM(GTK) && !PLATFORM(QT)
#define ENABLE_PURGEABLE_MEMORY 1
#endif

#if PLATFORM(IOS)
#define ENABLE_CONTEXT_MENUS 0
#define ENABLE_DRAG_SUPPORT 0
#define ENABLE_GEOLOCATION 1
#define ENABLE_ICONDATABASE 0
#define ENABLE_INSPECTOR 1
#define ENABLE_JAVA_BRIDGE 0
#define ENABLE_NETSCAPE_PLUGIN_API 0
#define ENABLE_ORIENTATION_EVENTS 1
#define ENABLE_REPAINT_THROTTLING 1
#define ENABLE_WEB_ARCHIVE 1
#define HAVE_NETWORK_CFDATA_ARRAY_CALLBACK 1
#define HAVE_PTHREAD_RWLOCK 1
#define HAVE_READLINE 1
#define WTF_USE_CF 1
#define WTF_USE_CFNETWORK 1
#define WTF_USE_PTHREADS 1

#if PLATFORM(IOS_SIMULATOR)
    #define ENABLE_CLASSIC_INTERPRETER 1
    #define ENABLE_JIT 0
    #define ENABLE_YARR_JIT 0
#else
    #define ENABLE_CLASSIC_INTERPRETER 0
    #define ENABLE_JIT 1
    #define ENABLE_LLINT 1
    #define ENABLE_YARR_JIT 1
#endif

#define WTF_USE_APPKIT 0
#define WTF_USE_SECURITY_FRAMEWORK 0
#endif

#if PLATFORM(WIN) && !OS(WINCE)
#define WTF_USE_CF 1
#endif

#if PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(CHROMIUM) && !PLATFORM(WIN_CAIRO)
#define WTF_USE_CFNETWORK 1
#endif

#if USE(CFNETWORK) || PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_CFURLCACHE 1
#define WTF_USE_CFURLSTORAGESESSIONS 1
#endif

#if PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(CHROMIUM) && !PLATFORM(QT)
#define ENABLE_WEB_ARCHIVE 1
#endif

#if PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(CHROMIUM) && !PLATFORM(WIN_CAIRO) && !PLATFORM(QT)
#define ENABLE_FULLSCREEN_API 1
#endif

#if PLATFORM(WX)
#if !CPU(PPC)
#define ENABLE_ASSEMBLER 1
#define ENABLE_JIT 1
#endif
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#define ENABLE_LLINT 0
#if OS(DARWIN)
#define WTF_USE_CF 1
#define ENABLE_WEB_ARCHIVE 1
#endif
#endif

#if OS(UNIX) && (PLATFORM(GTK) || PLATFORM(QT))
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#endif

#if !defined(HAVE_ACCESSIBILITY)
#if PLATFORM(IOS) || PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(GTK) || (PLATFORM(CHROMIUM) && !OS(ANDROID))
#define HAVE_ACCESSIBILITY 1
#endif
#endif /* !defined(HAVE_ACCESSIBILITY) */

#if OS(UNIX)
#define HAVE_SIGNAL_H 1
#define WTF_USE_OS_RANDOMNESS 1
#endif

#if (OS(FREEBSD) || OS(OPENBSD)) && !defined(__GLIBC__)
#define HAVE_PTHREAD_NP_H 1
#endif

#if !defined(HAVE_VASPRINTF)
#if !COMPILER(MSVC) && !COMPILER(RVCT) && !COMPILER(MINGW) && !(COMPILER(GCC) && OS(QNX))
#define HAVE_VASPRINTF 1
#endif
#endif

#if !defined(HAVE_STRNSTR)
#if OS(DARWIN) || (OS(FREEBSD) && !defined(__GLIBC__))
#define HAVE_STRNSTR 1
#endif
#endif

#if !OS(WINDOWS) && !OS(SOLARIS) \
    && !OS(RVCT) \
    && !OS(ANDROID)
#define HAVE_TM_GMTOFF 1
#define HAVE_TM_ZONE 1
#define HAVE_TIMEGM 1
#endif

#if OS(DARWIN)

#define HAVE_ERRNO_H 1
#define HAVE_LANGINFO_H 1
#define HAVE_MMAP 1
#define HAVE_MERGESORT 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMEB_H 1
#define WTF_USE_ACCELERATE 1

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060

#define HAVE_DISPATCH_H 1
#define HAVE_HOSTED_CORE_ANIMATION 1

#if !PLATFORM(IOS)
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MADV_FREE 1
#define HAVE_PTHREAD_SETNAME_NP 1
#endif

#endif

#if PLATFORM(IOS)
#define HAVE_MADV_FREE 1
#define HAVE_PTHREAD_SETNAME_NP 1
#endif

#elif OS(WINDOWS)

#if !OS(WINCE)
#define HAVE_SYS_TIMEB_H 1
#define HAVE_ALIGNED_MALLOC 1
#define HAVE_ISDEBUGGERPRESENT 1
#endif
#define HAVE_VIRTUALALLOC 1
#define WTF_USE_OS_RANDOMNESS 1

#elif OS(QNX)

#define HAVE_ERRNO_H 1
#define HAVE_MMAP 1
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MADV_FREE 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1
#define WTF_USE_PTHREADS 1

#elif OS(ANDROID)

#define HAVE_ERRNO_H 1
#define HAVE_NMAP 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1

#else

/* FIXME: is this actually used or do other platforms generate their own config.h? */

#define HAVE_ERRNO_H 1
#define HAVE_LANGINFO_H 1
#define HAVE_MMAP 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1

#endif

/* ENABLE macro defaults */

#if PLATFORM(QT)
/* We must not customize the global operator new and delete for the Qt port. */
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#if !OS(UNIX)
#define USE_SYSTEM_MALLOC 1
#endif
#endif

#if !defined(ENABLE_ICONDATABASE)
#define ENABLE_ICONDATABASE 1
#endif

#if !defined(ENABLE_SQL_DATABASE)
#define ENABLE_SQL_DATABASE 1
#endif

#if !defined(ENABLE_JAVASCRIPT_DEBUGGER)
#define ENABLE_JAVASCRIPT_DEBUGGER 1
#endif

#if !defined(ENABLE_FTPDIR)
#define ENABLE_FTPDIR 1
#endif

#if !defined(ENABLE_CONTEXT_MENUS)
#define ENABLE_CONTEXT_MENUS 1
#endif

#if !defined(ENABLE_DRAG_SUPPORT)
#define ENABLE_DRAG_SUPPORT 1
#endif

#if !defined(ENABLE_INSPECTOR)
#define ENABLE_INSPECTOR 1
#endif

#if !defined(ENABLE_NETSCAPE_PLUGIN_API)
#define ENABLE_NETSCAPE_PLUGIN_API 1
#endif

#if !defined(ENABLE_GLOBAL_FASTMALLOC_NEW)
#define ENABLE_GLOBAL_FASTMALLOC_NEW 1
#endif

#if !defined(ENABLE_PARSED_STYLE_SHEET_CACHING)
#define ENABLE_PARSED_STYLE_SHEET_CACHING 1
#endif

#if !defined(ENABLE_SUBPIXEL_LAYOUT)
#if PLATFORM(CHROMIUM)
#define ENABLE_SUBPIXEL_LAYOUT 1 
#else
#define ENABLE_SUBPIXEL_LAYOUT 0
#endif
#endif

#if !defined(ENABLE_GESTURE_ANIMATION)
#if PLATFORM(QT) || !ENABLE(SMOOTH_SCROLLING)
#define ENABLE_GESTURE_ANIMATION 0
#else
#define ENABLE_GESTURE_ANIMATION 1
#endif
#endif

#if !defined(ENABLE_SATURATED_LAYOUT_ARITHMETIC)
#define ENABLE_SATURATED_LAYOUT_ARITHMETIC 0
#endif

#if ENABLE(ENABLE_SATURATED_LAYOUT_ARITHMETIC) && !ENABLE(ENABLE_SUBPIXEL_LAYOUT)
#error "ENABLE_SATURATED_LAYOUT_ARITHMETIC requires ENABLE_SUBPIXEL_LAYOUT"
#endif

#define ENABLE_DEBUG_WITH_BREAKPOINT 0
#define ENABLE_SAMPLING_COUNTERS 0
#define ENABLE_SAMPLING_FLAGS 0
#define ENABLE_SAMPLING_REGIONS 0
#define ENABLE_OPCODE_SAMPLING 0
#define ENABLE_CODEBLOCK_SAMPLING 0
#if ENABLE(CODEBLOCK_SAMPLING) && !ENABLE(OPCODE_SAMPLING)
#error "CODEBLOCK_SAMPLING requires OPCODE_SAMPLING"
#endif
#if ENABLE(OPCODE_SAMPLING) || ENABLE(SAMPLING_FLAGS) || ENABLE(SAMPLING_REGIONS)
#define ENABLE_SAMPLING_THREAD 1
#endif

#if !defined(ENABLE_TEXT_CARET) && !PLATFORM(IOS)
#define ENABLE_TEXT_CARET 1
#endif

#if !defined(WTF_USE_JSVALUE64) && !defined(WTF_USE_JSVALUE32_64)
#if (CPU(X86_64) && (OS(UNIX) || OS(WINDOWS))) \
    || (CPU(IA64) && !CPU(IA64_32)) \
    || CPU(ALPHA) \
    || CPU(SPARC64) \
    || CPU(S390X) \
    || CPU(PPC64)
#define WTF_USE_JSVALUE64 1
#else
#define WTF_USE_JSVALUE32_64 1
#endif
#endif /* !defined(WTF_USE_JSVALUE64) && !defined(WTF_USE_JSVALUE32_64) */

/* Disable the JIT on versions of GCC prior to 4.1 */
#if !defined(ENABLE_JIT) && COMPILER(GCC) && !GCC_VERSION_AT_LEAST(4, 1, 0)
#define ENABLE_JIT 0
#endif

/* JIT is not implemented for Windows 64-bit */
#if !defined(ENABLE_JIT) && OS(WINDOWS) && CPU(X86_64)
#define ENABLE_JIT 0
#endif

#if !defined(ENABLE_JIT) && CPU(SH4) && PLATFORM(QT)
#define ENABLE_JIT 1
#endif

/* The JIT is enabled by default on all x86, x86-64, ARM & MIPS platforms. */
#if !defined(ENABLE_JIT) \
    && (CPU(X86) || CPU(X86_64) || CPU(ARM) || CPU(MIPS)) \
    && (OS(DARWIN) || !COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 1, 0)) \
    && !OS(WINCE) \
    && !OS(QNX)
#define ENABLE_JIT 1
#endif

/* If possible, try to enable a disassembler. This is optional. We proceed in two
   steps: first we try to find some disassembler that we can use, and then we
   decide if the high-level disassembler API can be enabled. */
#if !defined(WTF_USE_UDIS86) && ENABLE(JIT) && PLATFORM(MAC) && (CPU(X86) || CPU(X86_64))
#define WTF_USE_UDIS86 1
#endif

#if !defined(ENABLE_DISASSEMBLER) && USE(UDIS86)
#define ENABLE_DISASSEMBLER 1
#endif

/* On some of the platforms where we have a JIT, we want to also have the 
   low-level interpreter. */
#if !defined(ENABLE_LLINT) \
    && ENABLE(JIT) \
    && (OS(DARWIN) || OS(LINUX)) \
    && (PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(GTK)) \
    && (CPU(X86) || CPU(X86_64) || CPU(ARM_THUMB2))
#define ENABLE_LLINT 1
#endif

#if !defined(ENABLE_DFG_JIT) && ENABLE(JIT)
/* Enable the DFG JIT on X86 and X86_64.  Only tested on Mac and GNU/Linux. */
#if (CPU(X86) || CPU(X86_64)) && (PLATFORM(MAC) || OS(LINUX))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARMv7.  Only tested on iOS. */
#if CPU(ARM_THUMB2) && (PLATFORM(IOS) || PLATFORM(BLACKBERRY))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARM. */
#if CPU(ARM_TRADITIONAL)
#define ENABLE_DFG_JIT 1
#endif
#endif

/* Profiling of types and values used by JIT code. DFG_JIT depends on it, but you
   can enable it manually with DFG turned off if you want to use it as a standalone
   profiler. In that case, you probably want to also enable VERBOSE_VALUE_PROFILE
   below. */
#if !defined(ENABLE_VALUE_PROFILER) && ENABLE(DFG_JIT)
#define ENABLE_VALUE_PROFILER 1
#endif

#if !defined(ENABLE_VERBOSE_VALUE_PROFILE) && ENABLE(VALUE_PROFILER)
#define ENABLE_VERBOSE_VALUE_PROFILE 0
#endif

#if !defined(ENABLE_SIMPLE_HEAP_PROFILING)
#define ENABLE_SIMPLE_HEAP_PROFILING 0
#endif

/* Counts uses of write barriers using sampling counters. Be sure to also
   set ENABLE_SAMPLING_COUNTERS to 1. */
#if !defined(ENABLE_WRITE_BARRIER_PROFILING)
#define ENABLE_WRITE_BARRIER_PROFILING 0
#endif

/* Ensure that either the JIT or the interpreter has been enabled. */
#if !defined(ENABLE_CLASSIC_INTERPRETER) && !ENABLE(JIT) && !ENABLE(LLINT)
#define ENABLE_CLASSIC_INTERPRETER 1
#endif

/* If the jit and classic interpreter is not available, enable the LLInt C Loop: */
#if !ENABLE(JIT) && !ENABLE(CLASSIC_INTERPRETER)
    #define ENABLE_LLINT 1
    #define ENABLE_LLINT_C_LOOP 1
    #define ENABLE_DFG_JIT 0
#endif

/* Do a sanity check to make sure that we at least have one execution engine in
   use: */
#if !(ENABLE(JIT) || ENABLE(CLASSIC_INTERPRETER) || ENABLE(LLINT))
#error You have to have at least one execution model enabled to build JSC
#endif
/* Do a sanity check to make sure that we don't have both the classic interpreter
   and the llint C loop in use at the same time: */
#if ENABLE(CLASSIC_INTERPRETER) && ENABLE(LLINT_C_LOOP)
#error You cannot build both the classic interpreter and the llint C loop together
#endif

/* Configure the JIT */
#if CPU(X86) && COMPILER(MSVC)
#define JSC_HOST_CALL __fastcall
#elif CPU(X86) && COMPILER(GCC)
#define JSC_HOST_CALL __attribute__ ((fastcall))
#else
#define JSC_HOST_CALL
#endif

/* Configure the interpreter */
#if COMPILER(GCC) || (RVCT_VERSION_AT_LEAST(4, 0, 0, 0) && defined(__GNUC__))
#define HAVE_COMPUTED_GOTO 1
#endif
#if HAVE(COMPUTED_GOTO) && ENABLE(CLASSIC_INTERPRETER)
#define ENABLE_COMPUTED_GOTO_CLASSIC_INTERPRETER 1
#endif

/* Determine if we need to enable Computed Goto Opcodes or not: */
#if (HAVE(COMPUTED_GOTO) && ENABLE(LLINT)) || ENABLE(COMPUTED_GOTO_CLASSIC_INTERPRETER)
#define ENABLE_COMPUTED_GOTO_OPCODES 1
#endif

/* Regular Expression Tracing - Set to 1 to trace RegExp's in jsc.  Results dumped at exit */
#define ENABLE_REGEXP_TRACING 0

/* Yet Another Regex Runtime - turned on by default for JIT enabled ports. */
#if !defined(ENABLE_YARR_JIT) && (ENABLE(JIT) || ENABLE(LLINT_C_LOOP)) && !PLATFORM(CHROMIUM)
#define ENABLE_YARR_JIT 1

/* Setting this flag compares JIT results with interpreter results. */
#define ENABLE_YARR_JIT_DEBUG 0
#endif

#if ENABLE(JIT) || ENABLE(YARR_JIT)
#define ENABLE_ASSEMBLER 1
#endif

/* Pick which allocator to use; we only need an executable allocator if the assembler is compiled in.
   On x86-64 we use a single fixed mmap, on other platforms we mmap on demand. */
#if ENABLE(ASSEMBLER)
#if CPU(X86_64) || PLATFORM(IOS)
#define ENABLE_EXECUTABLE_ALLOCATOR_FIXED 1
#else
#define ENABLE_EXECUTABLE_ALLOCATOR_DEMAND 1
#endif
#endif

#if !defined(ENABLE_PAN_SCROLLING) && OS(WINDOWS)
#define ENABLE_PAN_SCROLLING 1
#endif

/* Use the QXmlStreamReader implementation for XMLDocumentParser */
/* Use the QXmlQuery implementation for XSLTProcessor */
#if PLATFORM(QT)
#if !USE(LIBXML2)
#define WTF_USE_QXMLSTREAM 1
#define WTF_USE_QXMLQUERY 1
#endif
#endif

/* Accelerated compositing */
#if PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(QT) || (PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(WIN_CAIRO))
#define WTF_USE_ACCELERATED_COMPOSITING 1
#endif

#if PLATFORM(MAC) || PLATFORM(IOS)
#define ENABLE_CSS_IMAGE_SET 1
#endif


/* Qt always uses Texture Mapper */
#if PLATFORM(QT)
#define WTF_USE_TEXTURE_MAPPER 1
#if USE(3D_GRAPHICS)
#define WTF_USE_TEXTURE_MAPPER_GL 1
#endif
#endif

#if ENABLE(WEBGL) && !defined(WTF_USE_3D_GRAPHICS)
#define WTF_USE_3D_GRAPHICS 1
#endif

/* Compositing on the UI-process in WebKit2 */
#if PLATFORM(QT)
#define WTF_USE_COORDINATED_GRAPHICS 1
#endif

#if PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_PROTECTION_SPACE_AUTH_CALLBACK 1
#endif

#if !ENABLE(NETSCAPE_PLUGIN_API) || (ENABLE(NETSCAPE_PLUGIN_API) && ((OS(UNIX) && (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(WX))) || PLATFORM(EFL)))
#define ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define ENABLE_THREADED_SCROLLING 1
#endif

/* Set up a define for a common error that is intended to cause a build error -- thus the space after Error. */
#define WTF_PLATFORM_CFNETWORK Error USE_macro_should_be_used_with_CFNETWORK

/* FIXME: Eventually we should enable this for all platforms and get rid of the define. */
#if PLATFORM(IOS) || PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
#define WTF_USE_PLATFORM_STRATEGIES 1
#endif

#if PLATFORM(WIN)
#define WTF_USE_CROSS_PLATFORM_CONTEXT_MENUS 1
#endif

#if PLATFORM(MAC) && HAVE(ACCESSIBILITY)
#define WTF_USE_ACCESSIBILITY_CONTEXT_MENUS 1
#endif

#if CPU(ARM_THUMB2)
#define ENABLE_BRANCH_COMPACTION 1
#endif

#if !defined(ENABLE_THREADING_LIBDISPATCH) && HAVE(DISPATCH_H)
#define ENABLE_THREADING_LIBDISPATCH 1
#elif !defined(ENABLE_THREADING_OPENMP) && defined(_OPENMP)
#define ENABLE_THREADING_OPENMP 1
#elif !defined(THREADING_GENERIC)
#define ENABLE_THREADING_GENERIC 1
#endif

#if ENABLE(GLIB_SUPPORT)
#include <wtf/gobject/GTypedefs.h>
#endif

/* FIXME: This define won't be needed once #27551 is fully landed. However, 
   since most ports try to support sub-project independence, adding new headers
   to WTF causes many ports to break, and so this way we can address the build
   breakages one port at a time. */
#if !defined(WTF_USE_EXPORT_MACROS) && (PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(WX) || PLATFORM(BLACKBERRY))
#define WTF_USE_EXPORT_MACROS 1
#endif

#if !defined(WTF_USE_EXPORT_MACROS_FOR_TESTING) && (PLATFORM(GTK) || PLATFORM(WIN))
#define WTF_USE_EXPORT_MACROS_FOR_TESTING 1
#endif

#if (PLATFORM(QT) && !OS(DARWIN) && !OS(WINDOWS)) || PLATFORM(GTK) || PLATFORM(EFL)
#define WTF_USE_UNIX_DOMAIN_SOCKETS 1
#endif

#if !defined(ENABLE_COMPARE_AND_SWAP) && (OS(WINDOWS) || (COMPILER(GCC) && (CPU(X86) || CPU(X86_64) || CPU(ARM_THUMB2))))
#define ENABLE_COMPARE_AND_SWAP 1
#endif

#define ENABLE_OBJECT_MARK_LOGGING 0

#if !defined(ENABLE_PARALLEL_GC) && !ENABLE(OBJECT_MARK_LOGGING) && (PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(BLACKBERRY) || PLATFORM(GTK)) && ENABLE(COMPARE_AND_SWAP)
#define ENABLE_PARALLEL_GC 1
#elif PLATFORM(QT)
// Parallel GC is temporarily disabled on Qt because of regular crashes, see https://bugs.webkit.org/show_bug.cgi?id=90957 for details
#define ENABLE_PARALLEL_GC 0
#endif

#if !defined(ENABLE_GC_VALIDATION) && !defined(NDEBUG)
#define ENABLE_GC_VALIDATION 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#define WTF_USE_AVFOUNDATION 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define WTF_USE_COREMEDIA 1
#endif

#if PLATFORM(MAC) || PLATFORM(GTK) || PLATFORM(EFL) || (PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(WIN_CAIRO)) || PLATFORM(BLACKBERRY)
#define WTF_USE_REQUEST_ANIMATION_FRAME_TIMER 1
#endif

#if PLATFORM(MAC) || PLATFORM(BLACKBERRY)
#define WTF_USE_REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR 1
#endif

#if PLATFORM(MAC) && (PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)
#define HAVE_INVERTED_WHEEL_EVENTS 1
#endif

#if PLATFORM(MAC)
#define WTF_USE_COREAUDIO 1
#endif

#if !defined(WTF_USE_V8) && PLATFORM(CHROMIUM)
#define WTF_USE_V8 1
#endif

/* Not using V8 implies using JSC and vice versa */
#if !USE(V8)
#define WTF_USE_JSC 1
#endif

#if ENABLE(NOTIFICATIONS) && PLATFORM(MAC)
#define ENABLE_TEXT_NOTIFICATIONS_ONLY 1
#endif

#if !defined(WTF_USE_ZLIB) && !PLATFORM(QT)
#define WTF_USE_ZLIB 1
#endif

#if PLATFORM(GTK)
#define WTF_DEPRECATED_STRING_OPERATORS
#endif

#if PLATFORM(QT)
#include <qglobal.h>
#if defined(QT_OPENGL_ES_2) && !defined(WTF_USE_OPENGL_ES_2)
#define WTF_USE_OPENGL_ES_2 1
#endif
#endif

#endif /* WTF_Platform_h */
