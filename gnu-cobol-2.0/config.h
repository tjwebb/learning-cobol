/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* long int is 32 bits */
/* #undef COB_32_BIT_LONG */

/* Pointers are longer than 32 bits */
#define COB_64_BIT_POINTER 1

/* Executable extension */
#define COB_EXEEXT ""

/* Enable experimental code (Developers only!) */
/* #undef COB_EXPERIMENTAL */

/* Compile/link option for exporting symbols */
#define COB_EXPORT_DYN ""

/* Keyword for inline */
#define COB_KEYWORD_INLINE __inline

/* long int is long long */
#define COB_LI_IS_LL 1

/* Can not dlopen self */
/* #undef COB_NO_SELFOPEN */

/* Object extension */
/* #undef COB_OBJEXT */

/* Enable minimum parameter check for system libraries */
/* #undef COB_PARAM_CHECK */

/* Compile/link option for PIC code */
#define COB_PIC_FLAGS "-fno-common -DPIC"

/* Compile/link option for shared code */
#define COB_SHARED_OPT "-bundle -flat_namespace -undefined suppress"

/* Can use strftime for timezone */
#define COB_STRFTIME 1

/* Strip command */
#define COB_STRIP_CMD "strip -x"

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Has __attribute__((aligned)) */
#define HAVE_ATTRIBUTE_ALIGNED 1

/* Define to 1 if you have the `canonicalize_file_name' function. */
/* #undef HAVE_CANONICALIZE_FILE_NAME */

/* Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
#define HAVE_CFLOCALECOPYCURRENT 1

/* Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
#define HAVE_CFPREFERENCESCOPYAPPVALUE 1

/* Has clock_gettime function and CLOCK_REALTIME */
/* #undef HAVE_CLOCK_GETTIME */

/* curses has color_set function */
#define HAVE_COLOR_SET 1

/* ncurses has _nc_freeall function */
#define HAVE_CURSES_FREEALL 1

/* Define to 1 if you have the <curses.h> header file. */
/* #undef HAVE_CURSES_H */

/* Define to 1 if you have the <db.h> header file. */
#define HAVE_DB_H 1

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
/* #undef HAVE_DCGETTEXT */

/* Has designated initializers */
#define HAVE_DESIGNATED_INITS 1

/* Define to 1 if you have the <disam.h> header file. */
/* #undef HAVE_DISAM_H */

/* Has dladdr function */
#define HAVE_DLADDR 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the `fcntl' function. */
#define HAVE_FCNTL 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fdatasync' function. */
#define HAVE_FDATASYNC 1

/* Declaration of finite function in ieeefp.h instead of math.h */
/* #undef HAVE_FINITE_IEEEFP_H */

/* Define to 1 if you have the `getexecname' function. */
/* #undef HAVE_GETEXECNAME */

/* Define if the GNU gettext() function is already present or preinstalled. */
/* #undef HAVE_GETTEXT */

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <gmp.h> header file. */
#define HAVE_GMP_H 1

/* Define if you have the iconv() function and it works. */
#define HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <isam.h> header file. */
/* #undef HAVE_ISAM_H */

/* Define if you have <langinfo.h> and nl_langinfo(CODESET). */
#define HAVE_LANGINFO_CODESET 1

/* Define to 1 if you have the `curses' library (-lcurses). */
/* #undef HAVE_LIBCURSES */

/* Define to 1 if you have the `ncurses' library (-lncurses). */
#define HAVE_LIBNCURSES 1

/* Define to 1 if you have the `ncursesw' library (-lncursesw). */
/* #undef HAVE_LIBNCURSESW */

/* Define to 1 if you have the `pdcurses' library (-lpdcurses). */
/* #undef HAVE_LIBPDCURSES */

/* Define to 1 if you have the `posix4' library (-lposix4). */
/* #undef HAVE_LIBPOSIX4 */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the `localeconv' function. */
#define HAVE_LOCALECONV 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the <ltdl.h> header file. */
/* #undef HAVE_LTDL_H */

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Do we have mp_get_memory_functions in gmp */
#define HAVE_MP_GET_MEMORY_FUNCTIONS 1

/* Has nanosleep function */
#define HAVE_NANO_SLEEP 1

/* Define to 1 if you have the <ncursesw/curses.h> header file. */
/* #undef HAVE_NCURSESW_CURSES_H */

/* Define to 1 if you have the <ncursesw/ncurses.h> header file. */
/* #undef HAVE_NCURSESW_NCURSES_H */

/* Define to 1 if you have the <ncurses.h> header file. */
#define HAVE_NCURSES_H 1

/* Define to 1 if you have the <ncurses/ncurses.h> header file. */
/* #undef HAVE_NCURSES_NCURSES_H */

/* Define to 1 if you have the <pdcurses.h> header file. */
/* #undef HAVE_PDCURSES_H */

/* Has -Wno-pointer-sign */
#define HAVE_PSIGN_OPT 1

/* Define to 1 if you have the `raise' function. */
#define HAVE_RAISE 1

/* Define to 1 if you have the `readlink' function. */
#define HAVE_READLINK 1

/* Define to 1 if you have the `realpath' function. */
#define HAVE_REALPATH 1

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if the system has the type `sig_atomic_t'. */
#define HAVE_SIG_ATOMIC_T 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strcoll' function. */
#define HAVE_STRCOLL 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the `strtol' function. */
#define HAVE_STRTOL 1

/* Define to 1 if you have the `strxfrm' function. */
#define HAVE_STRXFRM 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Has timezone variable */
#define HAVE_TIMEZONE 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* ncurses has use_legacy_coding function */
#define HAVE_USE_LEGACY_CODING 1

/* Define to 1 if you have the <vbisam.h> header file. */
/* #undef HAVE_VBISAM_H */

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Define maximum parameters for CALL */
#define MAX_CALL_FIELD_PARAMS 192

/* Name of package */
#define PACKAGE "gnu-cobol"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "open-cobol-list@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "GnuCOBOL"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "GnuCOBOL 2.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "gnu-cobol"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.opencobol.org"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.0"

/* Define a patch level (numeric, max. 8 digits) */
#define PATCH_LEVEL 0

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Use system dynamic loader */
#define USE_LIBDL 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "2.0"

/* Use CISAM as ISAM handler */
/* #undef WITH_CISAM */

/* curses library for extended SCREEN I/O */
#define WITH_CURSES "ncurses"

/* Use Berkeley DB library as emulation of ISAM handler */
#define WITH_DB 1

/* Use DISAM as ISAM handler */
/* #undef WITH_DISAM */

/* Compile with an external ISAM handler */
/* #undef WITH_INDEX_EXTFH */

/* Compile with an external SEQ/RAN handler */
/* #undef WITH_SEQRA_EXTFH */

/* Define variable sequential file format */
#define WITH_VARSEQ 0

/* Use VBISAM as ISAM handler */
/* #undef WITH_VBISAM */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define to 1 if on HPUX.  */
#ifndef _XOPEN_SOURCE_EXTENDED
/* # undef _XOPEN_SOURCE_EXTENDED */
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
