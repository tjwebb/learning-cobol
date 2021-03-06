/*
   Copyright (C) 2001-2012, 2014-2016 Free Software Foundation, Inc.
   Written by Keisuke Nishida, Roger While, Simon Sobisch, Ron Norman

   This file is part of GnuCOBOL.

   The GnuCOBOL runtime library is free software: you can redistribute it
   and/or modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   GnuCOBOL is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GnuCOBOL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "defaults.h"
#include "tarstamp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>

#include <math.h>
#ifdef HAVE_FINITE_IEEEFP_H
#include <ieeefp.h>
#endif

#include <time.h>

#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef	HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef	_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifdef	HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef	HAVE_LOCALE_H
#include <locale.h>
#endif

#include "lib/gettext.h"

/* Force symbol exports */
#define	COB_LIB_EXPIMP

#include "libcob.h"
#include "coblocal.h"

#include "libcob/cobgetopt.h"

#define	CB_IMSG_SIZE	24
#define	CB_IVAL_SIZE	(80 - CB_IMSG_SIZE - 4)

/* Stringify macros */
#define CB_STRINGIFY(s)			#s
#define CB_XSTRINGIFY(s)		CB_STRINGIFY(s)

/* C version info */
#ifdef	__VERSION__
#define OC_C_VERSION_PRF	""
#define OC_C_VERSION	CB_XSTRINGIFY(__VERSION__)
#elif	defined(__xlc__)
#define OC_C_VERSION_PRF	"(IBM) "
#define OC_C_VERSION	CB_XSTRINGIFY(__xlc__)
#elif	defined(_MSC_VER)
#define OC_C_VERSION_PRF	"(Microsoft) "
#define OC_C_VERSION	CB_XSTRINGIFY(_MSC_VER)
#elif	defined(__BORLANDC__)
#define OC_C_VERSION_PRF	"(Borland) "
#define OC_C_VERSION	CB_XSTRINGIFY(__BORLANDC__)
#elif	defined(__WATCOMC__)
#define OC_C_VERSION_PRF	"(Watcom) "
#define OC_C_VERSION	CB_XSTRINGIFY(__WATCOMC__)
#elif	defined(__INTEL_COMPILER)
#define OC_C_VERSION_PRF	"(Intel) "
#define OC_C_VERSION	CB_XSTRINGIFY(__INTEL_COMPILER)
#else
#define OC_C_VERSION_PRF	""
#define OC_C_VERSION	"unknown"
#endif

struct cob_alloc_cache {
	struct cob_alloc_cache	*next;		/* Pointer to next */
	void			*cob_pointer;	/* Pointer to malloced space */
	size_t			size;		/* Item size */
};

/* EXTERNAL structure */

struct cob_external {
	struct cob_external	*next;		/* Pointer to next */
	void			*ext_alloc;	/* Pointer to malloced space */
	char			*ename;		/* External name */
	int			esize;		/* Item size */
};

#define COB_ERRBUF_SIZE		1024

/* Local variables */

static int			cob_initialized = 0;
static int			cob_argc;
static char			**cob_argv;
static struct cob_alloc_cache	*cob_alloc_base;
static char			*cob_last_sfile;

static cob_global		*cobglobptr = NULL;
static cob_settings		*cobsetptr = NULL;

static char			*runtime_err_str = NULL;

static const cob_field_attr	const_alpha_attr =
				{COB_TYPE_ALPHANUMERIC, 0, 0, 0, NULL};

static char			*cob_local_env = NULL;
static int			current_arg;
static unsigned char		*commlnptr = NULL;
static size_t			commlncnt;
static size_t			cob_local_env_size = 0;

static struct cob_external	*basext = NULL;

static size_t			sort_nkeys = 0;
static cob_file_key		*sort_keys;
static const unsigned char	*sort_collate;

static const char		*cob_current_program_id = NULL;
static const char		*cob_current_section = NULL;
static const char		*cob_current_paragraph = NULL;
static const char		*cob_source_file = NULL;
static const char		*cob_source_statement = NULL;
static FILE			*cob_trace_file = NULL;
static unsigned int		cob_source_line = 0;

static char				*strbuff = NULL;

static int		cob_process_id = 0;
static int		cob_temp_iteration = 0;

static unsigned int conf_runtime_error_displayed = 0;
static unsigned int last_runtime_error_line = 0;
static const char	*last_runtime_error_file = "unknown";

#if	defined(HAVE_SIGNAL_H) && defined(HAVE_SIG_ATOMIC_T)
static volatile sig_atomic_t	sig_is_handled = 0;
#endif

/* Function Pointer for external signal handling */
static void		(*cob_ext_sighdl) (int) = NULL;

#if defined(_MSC_VER) && COB_USE_VC2008_OR_GREATER
static VOID		(WINAPI *time_as_filetime_func) (LPFILETIME) = NULL;
#endif

#undef	COB_EXCEPTION
#define COB_EXCEPTION(code,tag,name,critical)	name,
static const char		* const cob_exception_tab_name[] = {
	"None",		/* COB_EC_ZERO */
#include "exception.def"
	"Invalid"	/* COB_EC_MAX */
};

#undef	COB_EXCEPTION
#define COB_EXCEPTION(code,tag,name,critical)	0x##code,
static const int		cob_exception_tab_code[] = {
	0,		/* COB_EC_ZERO */
#include "exception.def"
	0		/* COB_EC_MAX */
};

#undef	COB_EXCEPTION

#define EXCEPTION_TAB_SIZE	sizeof(cob_exception_tab_code) / sizeof(int)

/* Switches */
#define	COB_SWITCH_MAX	36  /* (must match cobc/tree.h)*/

static int		cob_switch[COB_SWITCH_MAX + 1];

/* Runtime exit handling */
static struct exit_handlerlist {
	struct exit_handlerlist	*next;
	int			(*proc)(void);
} *exit_hdlrs;

/* Runtime error handling */
static struct handlerlist {
	struct handlerlist	*next;
	int			(*proc)(char *s);
} *hdlrs;

static const char *setting_group[] = {" hidden setting ",_("CALL configuration"),
					_("File I/O configuration"),_("Screen I/O configuration"),_("Miscellaneous"),
					_("System configuration")};

static char	not_set[] = "not set";
static struct config_enum lwrupr[] = {{"LOWER","1"},{"UPPER","2"},{not_set,"0"},{NULL,NULL}};
static struct config_enum beepopts[] = {{"FLASH","1"},{"SPEAKER","2"},{"FALSE","9"},{"BEEP","0"},{NULL,NULL}};
static struct config_enum timeopts[] = {{"0","1000"},{"1","100"},{"2","10"},{"3","1"},{NULL,NULL}};
static struct config_enum syncopts[] = {{"P","1"},{NULL,NULL}};
static struct config_enum varseqopts[] = {{"0","0"},{"1","1"},{"2","2"},{"3","3"},{NULL,NULL}};
static char	varseq_dflt[8] = "0";


/*
 * Table of possible environment variables and/or runtime.cfg parameters
   Env Var name, Name used in run-time config file, Default value (NULL for aliases), Table of Alternate values,
   Grouping for display of run-time options, Data type, Location within structure (adds computed length of referenced field),
   Set by which runtime.cfg file, value set by a different keyword,
   optional: Minimum accepted value, Maximum accepted value
 */
static struct config_tbl gc_conf[] = {
	{"COB_LOAD_CASE","load_case",		"0",	lwrupr,GRP_CALL,ENV_INT|ENV_ENUMVAL,SETPOS(name_convert)},
	{"COB_PHYSICAL_CANCEL","physical_cancel",	"0",	NULL,GRP_CALL,ENV_BOOL,SETPOS(cob_physical_cancel)},
	{"default_cancel_mode","default_cancel_mode",	NULL,NULL,GRP_HIDE,ENV_BOOL|ENV_NOT,SETPOS(cob_physical_cancel)},
	{"LOGICAL_CANCELS","logical_cancels",	NULL,NULL,GRP_HIDE,ENV_BOOL|ENV_NOT,SETPOS(cob_physical_cancel)},
	{"COB_PRE_LOAD","pre_load",		NULL,	NULL,GRP_CALL,ENV_STR,SETPOS(cob_preload_str)},
	{"COB_BELL","bell",			"0",	beepopts,GRP_SCREEN,ENV_INT|ENV_ENUMVAL,SETPOS(cob_beep_value)},
	{"COB_DEBUG_LOG","debug_log",		NULL,	NULL,GRP_HIDE,ENV_FILE,SETPOS(cob_debug_log)},
	{"COB_DISABLE_WARNINGS","disable_warnings","0",	NULL,GRP_MISC,ENV_BOOL|ENV_NOT,SETPOS(cob_display_warn)},
	{"COB_ENV_MANGLE","env_mangle",		"0",	NULL,GRP_MISC,ENV_BOOL,SETPOS(cob_env_mangle)},
	{"COB_REDIRECT_DISPLAY","redirect_display","0",	NULL,GRP_SCREEN,ENV_BOOL,SETPOS(cob_disp_to_stderr)},
	{"COB_SCREEN_ESC","screen_esc",		"0",	NULL,GRP_SCREEN,ENV_BOOL,SETPOS(cob_use_esc)},
	{"COB_SCREEN_EXCEPTIONS","screen_exceptions","0",NULL,GRP_SCREEN,ENV_BOOL,SETPOS(cob_extended_status)},
	{"COB_INSERT_MODE","insert_mode","0",NULL,GRP_SCREEN,ENV_BOOL,SETPOS(cob_insert_mode)},
	{"COB_SET_TRACE","set_trace",		"0",	NULL,GRP_MISC,ENV_BOOL,SETPOS(cob_line_trace)},
	{"COB_TIMEOUT_SCALE","timeout_scale",	"0",	timeopts,GRP_SCREEN,ENV_INT,SETPOS(cob_timeout_scale)},
	{"COB_TRACE_FILE","trace_file",		NULL,	NULL,GRP_MISC,ENV_FILE,SETPOS(cob_trace_filename)},
#ifdef  _WIN32
	/* checked before configuration load if set from environment in cob_init() */
	{"COB_UNIX_LF","unix_lf",		"0",	NULL,GRP_FILE,ENV_BOOL,SETPOS(cob_unix_lf)},
#endif
	{"USERNAME","username",			_("unknown"),	NULL,GRP_SYSENV,ENV_STR,SETPOS(cob_user_name)},
	{"LOGNAME","logname",			NULL,	NULL,GRP_HIDE,ENV_STR,SETPOS(cob_user_name)},
#if !defined(_WIN32) || defined (__MINGW32__) /* cygwin does not define _WIN32 */
	{"LANG","lang",				NULL,	NULL,GRP_SYSENV,ENV_STR,SETPOS(cob_sys_lang)},
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__)
	{"OSTYPE","ostype",			NULL,	NULL,GRP_SYSENV,ENV_STR,SETPOS(cob_sys_type)},
#endif
	{"TERM","term",				NULL,	NULL,GRP_SYSENV,ENV_STR,SETPOS(cob_sys_term)},
#endif
#if defined(_WIN32) && !defined(__MINGW32__)
	{"OS","ostype",			NULL,	NULL,GRP_SYSENV,ENV_STR,SETPOS(cob_sys_type)},
#endif
	{"COB_FILE_PATH","file_path",		NULL,	NULL,GRP_FILE,ENV_PATH,SETPOS(cob_file_path)},
	{"COB_LIBRARY_PATH","library_path",	NULL,	NULL,GRP_CALL,ENV_PATH,SETPOS(cob_library_path)}, /* default value set in cob_init_call() */
	{"COB_VARSEQ_FORMAT","varseq_format",	varseq_dflt,varseqopts,GRP_FILE,ENV_INT|ENV_ENUM,SETPOS(cob_varseq_type)},
	{"COB_LS_FIXED","ls_fixed",		"0",	NULL,GRP_FILE,ENV_BOOL,SETPOS(cob_ls_fixed)},
	{"STRIP_TRAILING_SPACES","strip_trailing_spaces",		NULL,	NULL,GRP_HIDE,ENV_BOOL|ENV_NOT,SETPOS(cob_ls_fixed)},
	{"COB_LS_NULLS","ls_nulls",		"0",	NULL,GRP_FILE,ENV_BOOL,SETPOS(cob_ls_nulls)},
	{"COB_SORT_CHUNK","sort_chunk",		"256K",	NULL,GRP_FILE,ENV_SIZE,SETPOS(cob_sort_chunk),(128 * 1024),(16 * 1024 * 1024)},
	{"COB_SORT_MEMORY","sort_memory",	"128M",	NULL,GRP_FILE,ENV_SIZE,SETPOS(cob_sort_memory),(1024*1024),4294967294 /* max. guaranteed - 1 */},
	{"COB_SYNC","sync",			"0",	syncopts,GRP_FILE,ENV_BOOL,SETPOS(cob_do_sync)},
#ifdef  WITH_DB
	{"DB_HOME","db_home",			NULL,	NULL,GRP_FILE,ENV_FILE,SETPOS(bdb_home)},
#endif
	{"COB_LEGACY","legacy",			NULL,	NULL,GRP_SCREEN,ENV_BOOL,SETPOS(cob_legacy)},
	{NULL,NULL,0,0}
};
#define NUM_CONFIG (sizeof(gc_conf)/sizeof(struct config_tbl)-1)
#define FUNC_NAME_IN_DEFAULT NUM_CONFIG + 1

/* Local functions */
static int	set_config_val(char *value, int pos);
static char *	get_config_val(char *value, int pos, char *orgvalue);
void conf_runtime_error_value(char *value, const int conf_pos);
void conf_runtime_error(const int finish_error, const char *fmt, ...);

static void
cob_exit_common (void)
{
	struct cob_external	*p;
	struct cob_external	*q;
	struct cob_alloc_cache	*x;
	struct cob_alloc_cache	*y;
	void 	*data;
	char	*str;
	unsigned int	i;

#ifdef	HAVE_SETLOCALE
	if (cobglobptr->cob_locale_orig) {
		(void) setlocale (LC_ALL, cobglobptr->cob_locale_orig);
		cob_free (cobglobptr->cob_locale_orig);
	}
	if (cobglobptr->cob_locale) {
		cob_free (cobglobptr->cob_locale);
	}
	if (cobglobptr->cob_locale_ctype) {
		cob_free (cobglobptr->cob_locale_ctype);
	}
	if (cobglobptr->cob_locale_collate) {
		cob_free (cobglobptr->cob_locale_collate);
	}
	if (cobglobptr->cob_locale_messages) {
		cob_free (cobglobptr->cob_locale_messages);
	}
	if (cobglobptr->cob_locale_monetary) {
		cob_free (cobglobptr->cob_locale_monetary);
	}
	if (cobglobptr->cob_locale_numeric) {
		cob_free (cobglobptr->cob_locale_numeric);
	}
	if (cobglobptr->cob_locale_time) {
		cob_free (cobglobptr->cob_locale_time);
	}
#endif

	if (commlnptr) {
		cob_free (commlnptr);
	}
	if (cob_local_env) {
		cob_free (cob_local_env);
	}

	/* Free library routine stuff */

	if (cobglobptr->cob_term_buff) {
		cob_free (cobglobptr->cob_term_buff);
	}

	/* Free cached externals */
	for (p = basext; p;) {
		q = p;
		p = p->next;
		if (q->ename) {
			cob_free (q->ename);
		}
		if (q->ext_alloc) {
			cob_free (q->ext_alloc);
		}
		cob_free (q);
	}

	/* Free cached mallocs */
	for (x = cob_alloc_base; x;) {
		y = x;
		x = x->next;
		cob_free (y->cob_pointer);
		cob_free (y);
	}

	/* Free last stuff */
	if (cob_last_sfile) {
		cob_free ((void *)cob_last_sfile);
	}
	if (runtime_err_str) {
		cob_free (runtime_err_str);
	}
	if (cobglobptr) {
		if (cobglobptr->cob_main_argv0) {
			cob_free ((void *)(cobglobptr->cob_main_argv0));
		}
		cob_free (cobglobptr);
		cobglobptr = NULL;
	}
	if (cobsetptr) {
		if (cobsetptr->cob_config_file) {
			for (i = 0; i < cobsetptr->cob_config_num; i++) {
				if (cobsetptr->cob_config_file[i]) {
					cob_free((void*)cobsetptr->cob_config_file[i]);
				}
			}
			cob_free ((void*)cobsetptr->cob_config_file);
		}
		/* Free all strings pointed to by cobsetptr */
		for (i=0; i < NUM_CONFIG; i++) {
			if ((gc_conf[i].data_type & ENV_STR)
			||  (gc_conf[i].data_type & ENV_FILE)
			||  (gc_conf[i].data_type & ENV_PATH)) {	/* String/Path to be stored as a string */
				data = (void*)((char *)cobsetptr + gc_conf[i].data_loc);
				memcpy(&str,data,sizeof(char *));
				if( str != NULL) {
					cob_free((void*)str);
					str = NULL;
					memcpy(data,&str,sizeof(char *));	/* Reset pointer to NULL */
				}
			}
		}
		cob_free (cobsetptr);
		cobsetptr = NULL;
	}
	cob_initialized = 0;
}

static void
cob_terminate_routines (void)
{

	if (!cob_initialized || !cobglobptr) {
		return;
	}
	if (cob_trace_file && cob_trace_file != stderr) {
		fclose (cob_trace_file);
		cob_trace_file = NULL;
	}

	cob_exit_screen ();
	cob_exit_fileio ();
	cob_exit_intrinsic ();
	cob_exit_strings ();
	cob_exit_numeric ();
	cob_exit_call ();
	cob_exit_common ();
}

#ifdef	HAVE_SIGNAL_H
DECLNORET static void COB_A_NORETURN
cob_sig_handler_ex (int sig)
{
	/* call external signal handler if registered */
	if (cob_ext_sighdl != NULL) {
		(*cob_ext_sighdl) (sig);
		cob_ext_sighdl = NULL;
	}
#ifdef	SIGSEGV
	if (sig == SIGSEGV) {
		exit (SIGSEGV);
	}
#endif
#ifdef	HAVE_RAISE
	raise (sig);
#else
	kill (cob_sys_getpid(), sig);
#endif
	exit (sig);
}


DECLNORET static void COB_A_NORETURN
cob_sig_handler (int sig)
{
	const char *signal_name;

#if	defined(HAVE_SIGACTION) && !defined(SA_RESETHAND)
	struct sigaction	sa;
#endif

#ifdef	HAVE_SIG_ATOMIC_T
	if (sig_is_handled) {
		cob_sig_handler_ex(sig);
	}
	sig_is_handled = 1;
#endif

	switch (sig) {
#ifdef	SIGINT
	case SIGINT:
		signal_name = "SIGINT";
		break;
#endif
#ifdef	SIGHUP
	case SIGHUP:
		signal_name = "SIGHUP";
		break;
#endif
#ifdef	SIGQUIT
	case SIGQUIT:
		signal_name = "SIGQUIT";
		break;
#endif
#ifdef	SIGTERM
	case SIGTERM:
		signal_name = "SIGTERM";
		break;
#endif
#ifdef	SIGPIPE
	case SIGPIPE:
		signal_name = "SIGPIPE";
		break;
#endif
#ifdef	SIGSEGV
	case SIGSEGV:
		signal_name = "SIGSEGV";
		break;
#endif
	default:
		signal_name = _("unknown");
		fprintf(stderr, _("cob_sig_handler caught not handled signal: %d"), sig);
		putc('\n', stderr);
		break;
	}

#ifdef	HAVE_SIGACTION
#ifndef	SA_RESETHAND
	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	(void)sigemptyset (&sa.sa_mask);
	(void)sigaction (sig, &sa, NULL);
#endif
#else
	(void)signal (sig, SIG_DFL);
#endif
	cob_exit_screen ();
	putc ('\n', stderr);
	if (cob_source_file) {
		fprintf (stderr, "%s: ", cob_source_file);
		if (cob_source_line) {
			fprintf (stderr, "%u: ", cob_source_line);
		}
	}

#ifdef	SIGSEGV
	if (sig == SIGSEGV) {
		fprintf (stderr, _("attempt to reference unallocated memory"));
	} else {
		fprintf (stderr, _("caught signal"));
	}
#else
	fprintf (stderr, _("caught signal"));
#endif
	fprintf (stderr, " (");
	fprintf (stderr, _("signal %s"), signal_name);
	fprintf (stderr, ")\n");

	if (cob_initialized) {
		cob_terminate_routines ();
		fprintf (stderr, _("abnormal termination - file contents may be incorrect"));
	}
	putc ('\n', stderr);
	fflush (stderr);

	cob_sig_handler_ex(sig);
}
#endif /* HAVE_SIGNAL_H */

static void
cob_set_signal (void)
{
#ifdef	HAVE_SIGNAL_H

#ifdef	HAVE_SIGACTION
	struct sigaction	sa;
	struct sigaction	osa;

	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = cob_sig_handler;
#ifdef	SA_RESETHAND
	sa.sa_flags = SA_RESETHAND;
#else
	sa.sa_flags = 0;
#endif
#ifdef	SA_NOCLDSTOP
	sa.sa_flags |= SA_NOCLDSTOP;
#endif

#ifdef	SIGINT
	(void)sigaction (SIGINT, NULL, &osa);
	if (osa.sa_handler != SIG_IGN) {
		(void)sigemptyset (&sa.sa_mask);
		(void)sigaction (SIGINT, &sa, NULL);
	}
#endif
#ifdef	SIGHUP
	(void)sigaction (SIGHUP, NULL, &osa);
	if (osa.sa_handler != SIG_IGN) {
		(void)sigemptyset (&sa.sa_mask);
		(void)sigaction (SIGHUP, &sa, NULL);
	}
#endif
#ifdef	SIGQUIT
	(void)sigaction (SIGQUIT, NULL, &osa);
	if (osa.sa_handler != SIG_IGN) {
		(void)sigemptyset (&sa.sa_mask);
		(void)sigaction (SIGQUIT, &sa, NULL);
	}
#endif
#ifdef	SIGTERM
	(void)sigaction (SIGTERM, NULL, &osa);
	if (osa.sa_handler != SIG_IGN) {
		(void)sigemptyset (&sa.sa_mask);
		(void)sigaction (SIGTERM, &sa, NULL);
	}
#endif
#ifdef	SIGPIPE
	(void)sigaction (SIGPIPE, NULL, &osa);
	if (osa.sa_handler != SIG_IGN) {
		(void)sigemptyset (&sa.sa_mask);
		(void)sigaction (SIGPIPE, &sa, NULL);
	}
#endif
#ifdef	SIGSEGV
	/* Take direct control of segmentation violation */
	(void)sigemptyset (&sa.sa_mask);
	(void)sigaction (SIGSEGV, &sa, NULL);
#endif

#else

#ifdef	SIGINT
	if (signal (SIGINT, SIG_IGN) != SIG_IGN) {
		(void)signal (SIGINT, cob_sig_handler);
	}
#endif
#ifdef	SIGHUP
	if (signal (SIGHUP, SIG_IGN) != SIG_IGN) {
		(void)signal (SIGHUP, cob_sig_handler);
	}
#endif
#ifdef	SIGQUIT
	if (signal (SIGQUIT, SIG_IGN) != SIG_IGN) {
		(void)signal (SIGQUIT, cob_sig_handler);
	}
#endif
#ifdef	SIGTERM
	if (signal (SIGTERM, SIG_IGN) != SIG_IGN) {
		(void)signal (SIGTERM, cob_sig_handler);
	}
#endif
#ifdef	SIGPIPE
	if (signal (SIGPIPE, SIG_IGN) != SIG_IGN) {
		(void)signal (SIGPIPE, cob_sig_handler);
	}
#endif
#ifdef	SIGSEGV
	/* Take direct control of segmentation violation */
	(void)signal (SIGSEGV, cob_sig_handler);
#endif

#endif
#endif
}

/* ASCII Sign
 * positive: 0123456789
 * negative: pqrstuvwxy
 */

static int
cob_get_sign_ascii (unsigned char *p)
{
#ifdef	COB_EBCDIC_MACHINE
	switch (*p) {
	case 'p':
		*p = (unsigned char)'0';
		return -1;
	case 'q':
		*p = (unsigned char)'1';
		return -1;
	case 'r':
		*p = (unsigned char)'2';
		return -1;
	case 's':
		*p = (unsigned char)'3';
		return -1;
	case 't':
		*p = (unsigned char)'4';
		return -1;
	case 'u':
		*p = (unsigned char)'5';
		return -1;
	case 'v':
		*p = (unsigned char)'6';
		return -1;
	case 'w':
		*p = (unsigned char)'7';
		return -1;
	case 'x':
		*p = (unsigned char)'8';
		return -1;
	case 'y':
		*p = (unsigned char)'9';
		return -1;
	}
	*p = (unsigned char)'0';
	return 1;
#else
	if (*p >= (unsigned char)'p' && *p <= (unsigned char)'y') {
		*p &= ~64U;
		return -1;
	}
	*p = (unsigned char)'0';
	return 1;
#endif
}

static void
cob_put_sign_ascii (unsigned char *p)
{
#ifdef	COB_EBCDIC_MACHINE
	switch (*p) {
	case '0':
		*p = (unsigned char)'p';
		return;
	case '1':
		*p = (unsigned char)'q';
		return;
	case '2':
		*p = (unsigned char)'r';
		return;
	case '3':
		*p = (unsigned char)'s';
		return;
	case '4':
		*p = (unsigned char)'t';
		return;
	case '5':
		*p = (unsigned char)'u';
		return;
	case '6':
		*p = (unsigned char)'v';
		return;
	case '7':
		*p = (unsigned char)'w';
		return;
	case '8':
		*p = (unsigned char)'x';
		return;
	case '9':
		*p = (unsigned char)'y';
		return;
	default:
		*p = (unsigned char)'0';
	}
#else
	*p |= 64U;
#endif
}

/* EBCDIC Sign
 * positive: {ABCDEFGHI
 * negative: }JKLMNOPQR
 */

static int
cob_get_sign_ebcdic (unsigned char *p)
{
	switch (*p) {
	case '{':
		*p = (unsigned char)'0';
		return 1;
	case 'A':
		*p = (unsigned char)'1';
		return 1;
	case 'B':
		*p = (unsigned char)'2';
		return 1;
	case 'C':
		*p = (unsigned char)'3';
		return 1;
	case 'D':
		*p = (unsigned char)'4';
		return 1;
	case 'E':
		*p = (unsigned char)'5';
		return 1;
	case 'F':
		*p = (unsigned char)'6';
		return 1;
	case 'G':
		*p = (unsigned char)'7';
		return 1;
	case 'H':
		*p = (unsigned char)'8';
		return 1;
	case 'I':
		*p = (unsigned char)'9';
		return 1;
	case '}':
		*p = (unsigned char)'0';
		return -1;
	case 'J':
		*p = (unsigned char)'1';
		return -1;
	case 'K':
		*p = (unsigned char)'2';
		return -1;
	case 'L':
		*p = (unsigned char)'3';
		return -1;
	case 'M':
		*p = (unsigned char)'4';
		return -1;
	case 'N':
		*p = (unsigned char)'5';
		return -1;
	case 'O':
		*p = (unsigned char)'6';
		return -1;
	case 'P':
		*p = (unsigned char)'7';
		return -1;
	case 'Q':
		*p = (unsigned char)'8';
		return -1;
	case 'R':
		*p = (unsigned char)'9';
		return -1;
	default:
		/* What to do here */
		*p = (unsigned char)('0' + (*p & 0x0F));
		if (*p > (unsigned char)'9') {
			*p = (unsigned char)'0';
		}
		return 1;
	}
}

static void
cob_put_sign_ebcdic (unsigned char *p, const int sign)
{
	if (sign < 0) {
		switch (*p) {
		case '0':
			*p = (unsigned char)'}';
			return;
		case '1':
			*p = (unsigned char)'J';
			return;
		case '2':
			*p = (unsigned char)'K';
			return;
		case '3':
			*p = (unsigned char)'L';
			return;
		case '4':
			*p = (unsigned char)'M';
			return;
		case '5':
			*p = (unsigned char)'N';
			return;
		case '6':
			*p = (unsigned char)'O';
			return;
		case '7':
			*p = (unsigned char)'P';
			return;
		case '8':
			*p = (unsigned char)'Q';
			return;
		case '9':
			*p = (unsigned char)'R';
			return;
		default:
			/* What to do here */
			*p = (unsigned char)'{';
			return;
		}
	}
	switch (*p) {
	case '0':
		*p = (unsigned char)'{';
		return;
	case '1':
		*p = (unsigned char)'A';
		return;
	case '2':
		*p = (unsigned char)'B';
		return;
	case '3':
		*p = (unsigned char)'C';
		return;
	case '4':
		*p = (unsigned char)'D';
		return;
	case '5':
		*p = (unsigned char)'E';
		return;
	case '6':
		*p = (unsigned char)'F';
		return;
	case '7':
		*p = (unsigned char)'G';
		return;
	case '8':
		*p = (unsigned char)'H';
		return;
	case '9':
		*p = (unsigned char)'I';
		return;
	default:
		/* What to do here */
		*p = (unsigned char)'{';
		return;
	}
}

static int
common_cmpc (const unsigned char *s1, const unsigned int c,
	     const size_t size, const unsigned char *col)
{
	size_t			i;
	int			ret;

	if (unlikely(col)) {
		for (i = 0; i < size; ++i) {
			if ((ret = col[s1[i]] - col[c]) != 0) {
				return ret;
			}
		}
	} else {
		for (i = 0; i < size; ++i) {
			if ((ret = s1[i] - c) != 0) {
				return ret;
			}
		}
	}
	return 0;
}

static int
common_cmps (const unsigned char *s1, const unsigned char *s2,
	     const size_t size, const unsigned char *col)
{
	size_t			i;
	int			ret;

	if (unlikely(col)) {
		for (i = 0; i < size; ++i) {
			if ((ret = col[s1[i]] - col[s2[i]]) != 0) {
				return ret;
			}
		}
	} else {
		for (i = 0; i < size; ++i) {
			if ((ret = s1[i] - s2[i]) != 0) {
				return ret;
			}
		}
	}
	return 0;
}

static int
cob_cmp_all (cob_field *f1, cob_field *f2)
{
	unsigned char		*data;
	const unsigned char	*s;
	size_t			size;
	int			ret;
	int			sign;

	size = f1->size;
	data = f1->data;
	sign = COB_GET_SIGN (f1);
	s = COB_MODULE_PTR->collating_sequence;
	if (f2->size == 1) {
		ret = common_cmpc (data, f2->data[0], size, s);
		goto end;
	}
	ret = 0;
	while (size >= f2->size) {
		if ((ret = common_cmps (data, f2->data, f2->size, s)) != 0) {
			goto end;
		}
		size -= f2->size;
		data += f2->size;
	}
	if (size > 0) {
		ret = common_cmps (data, f2->data, size, s);
	}

end:
	if (COB_FIELD_TYPE (f1) != COB_TYPE_NUMERIC_PACKED) {
		COB_PUT_SIGN (f1, sign);
	}
	return ret;
}

static int
cob_cmp_alnum (cob_field *f1, cob_field *f2)
{
	const unsigned char	*s;
	size_t			min;
	int			ret;
	int			sign1;
	int			sign2;

	sign1 = COB_GET_SIGN (f1);
	sign2 = COB_GET_SIGN (f2);
	min = (f1->size < f2->size) ? f1->size : f2->size;
	s = COB_MODULE_PTR->collating_sequence;

	/* Compare common substring */
	if ((ret = common_cmps (f1->data, f2->data, min, s)) != 0) {
		goto end;
	}

	/* Compare the rest (if any) with spaces */
	if (f1->size > f2->size) {
		ret = common_cmpc (f1->data + min, ' ', f1->size - min, s);
	} else if (f1->size < f2->size) {
		ret = -common_cmpc (f2->data + min, ' ', f2->size - min, s);
	}

end:
	if (COB_FIELD_TYPE (f1) != COB_TYPE_NUMERIC_PACKED) {
		COB_PUT_SIGN (f1, sign1);
	}
	if (COB_FIELD_TYPE (f2) != COB_TYPE_NUMERIC_PACKED) {
		COB_PUT_SIGN (f2, sign2);
	}
	return ret;
}

static int
sort_compare (const void *data1, const void *data2)
{
	size_t		i;
	int		cmp;
	cob_field	f1;
	cob_field	f2;

	for (i = 0; i < sort_nkeys; ++i) {
		f1 = f2 = *sort_keys[i].field;
		f1.data = (unsigned char *)data1 + sort_keys[i].offset;
		f2.data = (unsigned char *)data2 + sort_keys[i].offset;
		if (COB_FIELD_IS_NUMERIC(&f1)) {
			cmp = cob_numeric_cmp (&f1, &f2);
		} else {
			cmp = common_cmps (f1.data, f2.data, f1.size, sort_collate);
		}
		if (cmp != 0) {
			return (sort_keys[i].flag == COB_ASCENDING) ? cmp : -cmp;
		}
	}
	return 0;
}

static void
cob_memcpy (cob_field *dst, const void *src, const size_t size)
{
	cob_field	temp;

	if (!dst->size) {
		return;
	}
	temp.size = size;
	temp.data = (cob_u8_ptr)src;
	temp.attr = &const_alpha_attr;
	cob_move (&temp, dst);
}

static void
cob_check_trace_file (void)
{
	if (!cobsetptr->cob_trace_filename) {
		cob_trace_file = stderr;
		return;
	}
	if (!cobsetptr->cob_unix_lf) {
		cob_trace_file = fopen (cobsetptr->cob_trace_filename, "w");
	} else {
		cob_trace_file = fopen (cobsetptr->cob_trace_filename, "wb");
	}
	if (!cob_trace_file) {
		cob_trace_file = stderr;
	}
}

int
cob_check_env_true (char* s)
{
	if (s) {
		if (strlen(s) == 1 && (*s == 'Y' || *s == 'y' || *s == '1')) return 1;
		if (strcasecmp(s, "YES") == 0 || strcasecmp(s, "ON") == 0 ||
			strcasecmp(s, "TRUE") == 0) {
			return 1;
		}
	}
	return 0;
}

int
cob_check_env_false (char* s)
{
	return s && ((strlen(s) == 1 && (*s == 'N' || *s == 'n' || *s == '0'))
		     || (strcasecmp(s, "NO") == 0 || strcasecmp(s, "NONE") == 0
			 || strcasecmp(s, "OFF") == 0
			 || strcasecmp(s, "FALSE") == 0));
}

static void
cob_rescan_env_vals (void)
{
	int	i;
	int	j;
	int	old_type;
	char	*env;
	char	*save_source_file = (char *) cob_source_file;

	cob_source_file = NULL;
	cob_source_line = 0;

	/* Check for possible environment variables */
	for (i = 0; i < NUM_CONFIG; i++) {
		if (gc_conf[i].env_name
		    && (env = getenv (gc_conf[i].env_name)) != NULL) {
			old_type = gc_conf[i].data_type;
			gc_conf[i].data_type |= STS_ENVSET;

			if (*env != '\0' && set_config_val (env, i)) {
				gc_conf[i].data_type = old_type;

				/* Remove invalid setting */
#if HAVE_SETENV
				(void)unsetenv (gc_conf[i].env_name);
#else
				env = cob_malloc (strlen (gc_conf[i].env_name)+2);
				sprintf (env,"%s=",gc_conf[i].env_name);
				(void)putenv (env);
#endif
			} else if (gc_conf[i].env_group == GRP_HIDE) {
				/* Any alias present? */
				for (j = 0; j < NUM_CONFIG; j++) {
					if (j != i
					    && gc_conf[i].data_loc == gc_conf[j].data_loc) {
						gc_conf[j].data_type |= STS_ENVSET;
						gc_conf[j].set_by = i;
					}
				}
			}
		}
	}
	cob_source_file = save_source_file;

	/* Extended ACCEPT status returns */
	if (cobsetptr->cob_extended_status == 0) {
		cobsetptr->cob_use_esc = 0;
	}
}

static int
one_indexed_day_of_week_from_monday(int zero_indexed_from_sunday)
{
	return ((zero_indexed_from_sunday + 6) % 7) + 1;
}

static void
set_unknown_offset(struct cob_time *time)
{
	time->offset_known = 0;
	time->utc_offset = 0;
}

#if defined(_MSC_VER) && COB_USE_VC2008_OR_GREATER
static void
set_cob_time_ns_from_filetime (const FILETIME filetime, struct cob_time *cb_time)
{
	ULONGLONG	filetime_int;

	filetime_int = (((ULONGLONG) filetime.dwHighDateTime) << 32)
		+ filetime.dwLowDateTime;
	/* FILETIMEs are accurate to 100 nanosecond intervals */
	cb_time->nanosecond = (filetime_int % (ULONGLONG) 10000000) * 100;

}
#endif

#if defined (_WIN32) && !defined (__CYGWIN__)
static void
set_cob_time_offset (struct cob_time *cb_time)
{
	DWORD	time_zone_result;
	TIME_ZONE_INFORMATION	time_zone_info;

	time_zone_result = GetTimeZoneInformation (&time_zone_info);
	if (time_zone_result != TIME_ZONE_ID_INVALID) {
		cb_time->offset_known = 1;
		cb_time->utc_offset = time_zone_info.Bias;
	} else {
		set_unknown_offset (cb_time);
	}
}
#endif

/* Global functions */

COB_INLINE int
cob_min_int (const int x, const int y)
{
	if (x < y) {
		return x;
	}
	return y;
}

COB_INLINE int
cob_max_int (const int x, const int y)
{
	if (x > y) {
		return x;
	}
	return y;
}

int
cob_get_exception_code (void)
{
	return cobglobptr->cob_exception_code;
}

const char *
cob_get_exception_name (void)
{
	size_t	n;

	for (n = 0; n < EXCEPTION_TAB_SIZE; ++n) {
		if (cobglobptr->cob_exception_code == cob_exception_tab_code[n]) {
			return cob_exception_tab_name[n];
		}
	}
	return NULL;
}

void
cob_set_exception (const int id)
{
	cobglobptr->cob_exception_code = cob_exception_tab_code[id];
	if (cobglobptr->cob_exception_code) {
		cobglobptr->cob_got_exception = 1;
		cobglobptr->cob_orig_statement = cob_source_statement;
		cobglobptr->cob_orig_line = cob_source_line;
		cobglobptr->cob_orig_program_id = cob_current_program_id;
		cobglobptr->cob_orig_section = cob_current_section;
		cobglobptr->cob_orig_paragraph = cob_current_paragraph;
	} else {
		cobglobptr->cob_got_exception = 0;
		cobglobptr->cob_orig_statement = NULL;
		cobglobptr->cob_orig_line = 0;
		cobglobptr->cob_orig_program_id = NULL;
		cobglobptr->cob_orig_section = NULL;
		cobglobptr->cob_orig_paragraph = NULL;
	}
}

void
cob_accept_exception_status (cob_field *f)
{
	cob_set_int (f, cobglobptr->cob_exception_code);
}

void
cob_accept_user_name (cob_field *f)
{
	if (cobsetptr->cob_user_name) {
		cob_memcpy (f, cobsetptr->cob_user_name,
			    strlen (cobsetptr->cob_user_name));
	} else {
		cob_memcpy (f, " ", (size_t)1);
	}
}

void *
cob_malloc (const size_t size)
{
	void	*mptr;

	mptr = calloc ((size_t)1, size);
	if (unlikely(!mptr)) {
		cob_fatal_error (COB_FERROR_MEMORY);
	}
	return mptr;
}

void *
cob_realloc (void * optr, const size_t osize, const size_t nsize)
{
	void	*mptr;

	if (unlikely(!optr)) {
		cob_fatal_error (COB_FERROR_FREE);
	}

	if (unlikely(osize <= nsize)) {
		return realloc (optr, nsize);
	}

	mptr = calloc ((size_t)1, nsize);
	if (unlikely(!mptr)) {
		cob_fatal_error (COB_FERROR_MEMORY);
	}
	memcpy (mptr, optr, osize);
	cob_free (optr);
	return mptr;
}

void
cob_free (void * mptr)
{
#ifdef _DEBUG
	if (unlikely(!mptr)) {
		cob_fatal_error (COB_FERROR_FREE);
	}
#endif
	free (mptr);

}

void *
cob_fast_malloc (const size_t size)
{
	void	*mptr;

	mptr = malloc (size);
	if (unlikely(!mptr)) {
		cob_fatal_error (COB_FERROR_MEMORY);
	}
	return mptr;
}

char *
cob_strdup (const char *p)
{
	char	*mptr;
	size_t	len;

	len = strlen (p);
	mptr = (char *) cob_malloc (len + 1U);
	memcpy (mptr, p, len);
	return mptr;
}

/* Caching versions of malloc/free */
void *
cob_cache_malloc (const size_t size)
{
	struct cob_alloc_cache	*cache_ptr;
	void			*mptr;

	cache_ptr = cob_malloc (sizeof (struct cob_alloc_cache));
	mptr = cob_malloc (size);
	cache_ptr->cob_pointer = mptr;
	cache_ptr->size = size;
	cache_ptr->next = cob_alloc_base;
	cob_alloc_base = cache_ptr;
	return mptr;
}

void *
cob_cache_realloc (void *ptr, const size_t size)
{
	struct cob_alloc_cache	*cache_ptr;
	void			*mptr;

	if (!ptr) {
		return cob_cache_malloc (size);
	}
	cache_ptr = cob_alloc_base;
	for (; cache_ptr; cache_ptr = cache_ptr->next) {
		if (ptr == cache_ptr->cob_pointer) {
			if (size <= cache_ptr->size) {
				return ptr;
			}
			mptr = cob_malloc (size);
			memcpy (mptr, cache_ptr->cob_pointer, cache_ptr->size);
			cob_free (cache_ptr->cob_pointer);
			cache_ptr->cob_pointer = mptr;
			cache_ptr->size = size;
			return mptr;
		}
	}
	return ptr;
}

void
cob_cache_free (void *ptr)
{
	struct cob_alloc_cache	*cache_ptr;
	struct cob_alloc_cache	*prev_ptr;

	if (!ptr) {
		return;
	}
	cache_ptr = cob_alloc_base;
	prev_ptr = cob_alloc_base;
	for (; cache_ptr; cache_ptr = cache_ptr->next) {
		if (ptr == cache_ptr->cob_pointer) {
			cob_free (cache_ptr->cob_pointer);
			if (cache_ptr == cob_alloc_base) {
				cob_alloc_base = cache_ptr->next;
			} else {
				prev_ptr->next = cache_ptr->next;
			}
			cob_free (cache_ptr);
			return;
		}
		prev_ptr = cache_ptr;
	}
}

void
cob_set_location (const char *sfile, const unsigned int sline,
		  const char *csect, const char *cpara,
		  const char *cstatement)
{
	const char	*s;

	cob_current_program_id = COB_MODULE_PTR->module_name;
	cob_source_file = sfile;
	cob_source_line = sline;
	cob_current_section = csect;
	cob_current_paragraph = cpara;
	if (cstatement) {
		cob_source_statement = cstatement;
	}
	if (cobsetptr->cob_line_trace) {
		if (!cob_trace_file) {
			cob_check_trace_file ();
			/* silence warnings */
			if (!cob_trace_file) {
				return;
			}
		}
		if (!cob_last_sfile || strcmp (cob_last_sfile, sfile)) {
			if (cob_last_sfile) {
				cob_free((void *)cob_last_sfile);
			}
			cob_last_sfile = cob_strdup (sfile);
			fprintf (cob_trace_file, "Source :    '%s'\n", sfile);
		}
		if (COB_MODULE_PTR->module_name) {
			s = COB_MODULE_PTR->module_name;
		} else {
			s = "Unknown";
		}
		fprintf (cob_trace_file,
			 "Program-Id: %-16s Statement: %-21.21s  Line: %u\n",
			 s, cstatement ? (char *)cstatement : "Unknown",
			 sline);
		fflush (cob_trace_file);
	}
}


void
cob_trace_section (const char *para, const char *source, const int line)
{
	const char	*s;

	if (cobsetptr->cob_line_trace) {
		if (!cob_trace_file) {
			cob_check_trace_file ();
			if (!cob_trace_file) return; /* silence warnings */
		}
		if (source &&
		    (!cob_last_sfile || strcmp (cob_last_sfile, source))) {
			if (cob_last_sfile) {
				cob_free ((void *)cob_last_sfile);
			}
			cob_last_sfile = cob_strdup (source);
			fprintf (cob_trace_file, "Source:     '%s'\n", source);
		}
		if (COB_MODULE_PTR->module_name) {
			s = COB_MODULE_PTR->module_name;
		} else {
			s = "Unknown";
		}
		fprintf (cob_trace_file, "Program-Id: %-16s ", s);
		if (line) {
			fprintf (cob_trace_file, "%-34.34sLine: %d\n", para, line);
		} else {
			fprintf (cob_trace_file, "%s\n", para);
		}
		fflush (cob_trace_file);
	}
}

void
cob_ready_trace (void)
{
	cobsetptr->cob_line_trace = 1;
}

void
cob_reset_trace (void)
{
	cobsetptr->cob_line_trace = 0;
}

unsigned char *
cob_get_pointer (const void *srcptr)
{
	void	*tmptr;

	memcpy (&tmptr, srcptr, sizeof (void *));
	return (cob_u8_ptr)tmptr;
}

void *
cob_get_prog_pointer (const void *srcptr)
{
	void	*tmptr;

	memcpy (&tmptr, srcptr, sizeof (void *));
	return tmptr;
}

void
cob_field_to_string (const cob_field *f, void *str, const size_t maxsize)
{
	unsigned char	*s;
	size_t		count;
	size_t		i;

	count = 0;
	if (unlikely(f->size == 0)) {
		return;
	}
	i = f->size - 1;
	for (; ;) {
		if (f->data[i] && f->data[i] != (unsigned char)' ') {
			count = i + 1;
			break;
		}
		if (!i) {
			break;
		}
		--i;
	}
	if (count > maxsize) {
		count = maxsize;
	}
	s = (unsigned char *)str;
	for (i = 0; i < count; ++i) {
		s[i] = f->data[i];
	}
	s[i] = 0;
}

void
cob_stop_run (const int status)
{
	struct exit_handlerlist	*h;

	if (!cob_initialized) {
		exit (1);
	}
	if (exit_hdlrs != NULL) {
		h = exit_hdlrs;
		while (h != NULL) {
			h->proc ();
			h = h->next;
		}
	}
	cob_terminate_routines ();
	exit (status);
}int
cob_is_initialized (void)
{
	return (cobglobptr != NULL);
}

cob_global *
cob_get_global_ptr (void)
{
	if (unlikely(!cob_initialized)) {
		cob_fatal_error (COB_FERROR_INITIALIZED);
	}
	return cobglobptr;
}

void
cob_module_enter (cob_module **module, cob_global **mglobal,
		  const int auto_init)
{
	/* Check initialized */
	if (unlikely(!cob_initialized)) {
		if (auto_init) {
			cob_init (0, NULL);
		} else {
			cob_fatal_error (COB_FERROR_INITIALIZED);
		}
	}

	/* Set global pointer */
	*mglobal = cobglobptr;

	/* Check module pointer */
	if (!*module) {
		*module = cob_cache_malloc (sizeof(cob_module));
	}

#if	1	/* RXWRXW - Params */
	/* Save parameter count */
	(*module)->module_num_params = cobglobptr->cob_call_params;
#endif

	/* Push module pointer */
	(*module)->next = COB_MODULE_PTR;
	COB_MODULE_PTR = *module;
}

void
cob_module_leave (cob_module *module)
{
	COB_UNUSED (module);
	/* Pop module pointer */
	COB_MODULE_PTR = COB_MODULE_PTR->next;
}

void *
cob_save_func (cob_field **savefld, const int params,
	       const int eparams, ...)
{
	struct cob_func_loc	*fl;
	va_list			args;
	int			numparams;
	int			n;

	if (unlikely(params > eparams)) {
		numparams = eparams;
	} else {
		numparams = params;
	}

	/* Allocate return field */
	*savefld = cob_malloc (sizeof (cob_field));
	/* Allocate save area */
	fl = cob_malloc (sizeof(struct cob_func_loc));
	fl->func_params = cob_malloc (sizeof(void *) * ((size_t)numparams + 1U));
	fl->data = cob_malloc (sizeof(void *) * ((size_t)numparams + 1U));

	/* Save values */
	fl->save_module = COB_MODULE_PTR->next;
 	fl->save_call_params = cobglobptr->cob_call_params;
	fl->save_proc_parms = COB_MODULE_PTR->cob_procedure_params;
	fl->save_num_params = COB_MODULE_PTR->module_num_params;

	/* Set current values */
	COB_MODULE_PTR->cob_procedure_params = fl->func_params;
	cobglobptr->cob_call_params = numparams;
	if (numparams) {
		va_start (args, eparams);
		for (n = 0; n < numparams; ++n) {
			fl->func_params[n] = va_arg (args, cob_field *);
			if (fl->func_params[n]) {
				fl->data[n] = fl->func_params[n]->data;
			}
		}
		va_end (args);
	}
	return fl;
}

void
cob_restore_func (struct cob_func_loc *fl)
{
	/* Restore calling environment */
	cobglobptr->cob_call_params = fl->save_call_params;
#if	0	/* RXWRXW - MODNEXT */
	COB_MODULE_PTR->next = fl->save_module;
#endif
	COB_MODULE_PTR->cob_procedure_params = fl->save_proc_parms;
	COB_MODULE_PTR->module_num_params = fl->save_num_params;
	cob_free (fl->data);
	cob_free (fl->func_params);
	cob_free (fl);
}

void
cob_check_version (const char *prog, const char *packver, const int patchlev)
{
	if (strcmp (packver, PACKAGE_VERSION) || patchlev != PATCH_LEVEL) {
		cob_runtime_error (_("error: version mismatch"));
		cob_runtime_error (_("%s has version/patch level %s/%d"), prog,
				   packver, patchlev);
		cob_runtime_error (_("libcob has version/patch level %s/%d"),
				   PACKAGE_VERSION, PATCH_LEVEL);
		cob_stop_run (1);
	}
}

void
cob_parameter_check (const char *funcname, const int numparms)
{
	if (cobglobptr->cob_call_params < numparms) {
		cob_runtime_error (_("CALL to %s requires %d parameters"),
				   funcname, numparms);
		cob_stop_run (1);
	}
}

void
cob_correct_numeric (cob_field *f)
{
	unsigned char	*p;
	unsigned char	*data;
	size_t		size;
	size_t		i;

	if (!COB_FIELD_IS_NUMDISP(f)) {
		return;
	}
	size = f->size;
	data = f->data;
	if (COB_FIELD_HAVE_SIGN (f)) {
		/* Adjust for sign byte */
		size--;
		if (unlikely(COB_FIELD_SIGN_LEADING (f))) {
			p = f->data;
			data = p + 1;
		} else {
			p = f->data + f->size - 1;
		}
		if (unlikely(COB_FIELD_SIGN_SEPARATE (f))) {
			if (*p != '+' && *p != '-') {
				*p = '+';
			}
		} else if (unlikely(COB_MODULE_PTR->ebcdic_sign)) {
			switch (*p) {
			case '{':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case '}':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
				break;
			case '0':
				*p = '{';
				break;
			case '1':
				*p = 'A';
				break;
			case '2':
				*p = 'B';
				break;
			case '3':
				*p = 'C';
				break;
			case '4':
				*p = 'D';
				break;
			case '5':
				*p = 'E';
				break;
			case '6':
				*p = 'F';
				break;
			case '7':
				*p = 'G';
				break;
			case '8':
				*p = 'H';
				break;
			case '9':
				*p = 'I';
				break;
			case 0:
			case ' ':
				*p = '{';
				break;
			default:
				break;
			}
		} else {
			if(!*p || *p == ' ') {
				*p = '0';
			}
		}
	} else {
		p = f->data + f->size - 1;
		if (unlikely(COB_MODULE_PTR->ebcdic_sign)) {
			switch (*p) {
			case 0:
			case ' ':
			case '{':
			case '}':
				*p = '0';
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
				*p = '1' + (*p - 'A');
				break;
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
				*p = '1' + (*p - 'J');
				break;
			default:
				break;
			}
		} else {
			switch (*p) {
			case 0:
			case ' ':
			case 'p':
				*p = '0';
				break;
			case 'q':
				*p = '1';
				break;
			case 'r':
				*p = '2';
				break;
			case 's':
				*p = '3';
				break;
			case 't':
				*p = '4';
				break;
			case 'u':
				*p = '5';
				break;
			case 'v':
				*p = '6';
				break;
			case 'w':
				*p = '7';
				break;
			case 'x':
				*p = '8';
				break;
			case 'y':
				*p = '9';
				break;
			default:
				break;
			}
		}
	}
	for (i = 0, p = data; i < size; ++i, ++p) {
		switch (*p) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		case 0:
		case ' ':
			*p = '0';
			break;
		default:
			if ((*p & 0x0F) <= 9) {
				*p = (*p & 0x0F) + '0';
			}
			break;
		}
	}
}

static int
cob_check_numdisp (const cob_field *f)
{
	unsigned char	*p;
	unsigned char	*data;
	size_t		size;
	size_t		i;

	size = f->size;
	data = f->data;
	if (COB_FIELD_HAVE_SIGN (f)) {
		/* Adjust for sign byte */
		size--;
		if (unlikely(COB_FIELD_SIGN_LEADING (f))) {
			p = f->data;
			data = p + 1;
		} else {
			p = f->data + f->size - 1;
		}
		if (unlikely(COB_FIELD_SIGN_SEPARATE (f))) {
			if (*p != '+' && *p != '-') {
				return 0;
			}
		} else if (unlikely(COB_MODULE_PTR->ebcdic_sign)) {
			switch (*p) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '{':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case '}':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
				break;
			default:
				return 0;
			}
		} else {
			switch (*p) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'p':
			case 'q':
			case 'r':
			case 's':
			case 't':
			case 'u':
			case 'v':
			case 'w':
			case 'x':
			case 'y':
				break;
			default:
				return 0;
			}
		}
	}
	for (i = 0; i < size; ++i) {
		if (!isdigit (data[i])) {
			return 0;
		}
	}
	return 1;
}

/* Sign */

int
cob_real_get_sign (cob_field *f)
{
	unsigned char	*p;

	switch (COB_FIELD_TYPE (f)) {
	case COB_TYPE_NUMERIC_DISPLAY:
		/* Locate sign */
		if (unlikely(COB_FIELD_SIGN_LEADING (f))) {
			p = f->data;
		} else {
			p = f->data + f->size - 1;
		}

		/* Get sign */
		if (unlikely(COB_FIELD_SIGN_SEPARATE (f))) {
			return (*p == '-') ? -1 : 1;
		}
		if (*p >= (unsigned char)'0' && *p <= (unsigned char)'9') {
			return 1;
		}
		if (*p == ' ') {
#if	0	/* RXWRXW - Space sign */
			*p = (unsigned char)'0';
#endif
			return 1;
		}
		if (unlikely(COB_MODULE_PTR->ebcdic_sign)) {
			return cob_get_sign_ebcdic (p);
		}
		return cob_get_sign_ascii (p);
	case COB_TYPE_NUMERIC_PACKED:
		if (COB_FIELD_NO_SIGN_NIBBLE (f)) {
			return 1;
		}
		p = f->data + f->size - 1;
		return ((*p & 0x0F) == 0x0D) ? -1 : 1;
	}
	return 0;
}

void
cob_real_put_sign (cob_field *f, const int sign)
{
	unsigned char	*p;
	unsigned char	c;

	switch (COB_FIELD_TYPE (f)) {
	case COB_TYPE_NUMERIC_DISPLAY:
		/* Locate sign */
		if (unlikely(COB_FIELD_SIGN_LEADING (f))) {
			p = f->data;
		} else {
			p = f->data + f->size - 1;
		}

		/* Put sign */
		if (unlikely(COB_FIELD_SIGN_SEPARATE (f))) {
			c = (sign < 0) ? (cob_u8_t)'-' : (cob_u8_t)'+';
			if (*p != c) {
				*p = c;
			}
		} else if (unlikely(COB_MODULE_PTR->ebcdic_sign)) {
			cob_put_sign_ebcdic (p, sign);
		} else if (sign < 0) {
			cob_put_sign_ascii (p);
		}
		return;
	case COB_TYPE_NUMERIC_PACKED:
		if (COB_FIELD_NO_SIGN_NIBBLE (f)) {
			return;
		}
		p = f->data + f->size - 1;
		if (sign < 0) {
			*p = (*p & 0xF0) | 0x0D;
		} else {
			*p = (*p & 0xF0) | 0x0C;
		}
		return;
	}
}

/* Registration of external handlers */
void
cob_reg_sighnd	(void (*sighnd) (int))
{
	if (!cob_initialized) {
		cob_set_signal ();
	}
	cob_ext_sighdl = sighnd;
}

/* Switch */

int
cob_get_switch (const int n)
{
	if (n < 0 || n > COB_SWITCH_MAX) {
		return 0;
	}
	return cob_switch[n];
}

void
cob_set_switch (const int n, const int flag)
{
	if (n < 0 || n > COB_SWITCH_MAX) {
		return;
	}
	if (flag == 0) {
		cob_switch[n] = 0;
	} else if (flag == 1) {
		cob_switch[n] = 1;
	}
}

int
cob_cmp (cob_field *f1, cob_field *f2)
{
	cob_field	temp;
	cob_field_attr	attr;
	unsigned char	buff[256];

	if (COB_FIELD_IS_NUMERIC (f1) && COB_FIELD_IS_NUMERIC (f2)) {
		return cob_numeric_cmp (f1, f2);
	}
	if (COB_FIELD_TYPE (f2) == COB_TYPE_ALPHANUMERIC_ALL) {
		if (f2->size == 1 && f2->data[0] == '0' &&
		    COB_FIELD_IS_NUMERIC (f1)) {
			return cob_cmp_int (f1, 0);
		}
		return cob_cmp_all (f1, f2);
	}
	if (COB_FIELD_TYPE (f1) == COB_TYPE_ALPHANUMERIC_ALL) {
		if (f1->size == 1 && f1->data[0] == '0' &&
		    COB_FIELD_IS_NUMERIC (f2)) {
			return -cob_cmp_int (f2, 0);
		}
		return -cob_cmp_all (f2, f1);
	}
	if (COB_FIELD_IS_NUMERIC (f1) &&
	    COB_FIELD_TYPE (f1) != COB_TYPE_NUMERIC_DISPLAY) {
		temp.size = COB_FIELD_DIGITS(f1);
		temp.data = buff;
		temp.attr = &attr;
		attr = *f1->attr;
		attr.type = COB_TYPE_NUMERIC_DISPLAY;
		attr.flags &= ~COB_FLAG_HAVE_SIGN;
		cob_move (f1, &temp);
		f1 = &temp;
	}
	if (COB_FIELD_IS_NUMERIC (f2) &&
	    COB_FIELD_TYPE (f2) != COB_TYPE_NUMERIC_DISPLAY) {
		temp.size = COB_FIELD_DIGITS(f2);
		temp.data = buff;
		temp.attr = &attr;
		attr = *f2->attr;
		attr.type = COB_TYPE_NUMERIC_DISPLAY;
		attr.flags &= ~COB_FLAG_HAVE_SIGN;
		cob_move (f2, &temp);
		f2 = &temp;
	}
	return cob_cmp_alnum (f1, f2);
}

/* Class check */

int
cob_is_omitted (const cob_field *f)
{
	return f->data == NULL;
}

int
cob_is_numeric (const cob_field *f)
{
	size_t		i;
	union {
		float		fpf;
		double		fpd;
	} fval;
	int		sign;

	switch (COB_FIELD_TYPE (f)) {
	case COB_TYPE_NUMERIC_BINARY:
		return 1;
	case COB_TYPE_NUMERIC_FLOAT:
		memcpy (&fval.fpf, f->data, sizeof(float));
		return !finite ((double)fval.fpf);
	case COB_TYPE_NUMERIC_DOUBLE:
		memcpy (&fval.fpd, f->data, sizeof(double));
		return !finite (fval.fpd);
	case COB_TYPE_NUMERIC_PACKED:
		/* Check digits */
		for (i = 0; i < f->size - 1; ++i) {
			if ((f->data[i] & 0xF0) > 0x90 ||
			    (f->data[i] & 0x0F) > 0x09) {
				return 0;
			}
		}
		/* Check high nibble of last byte */
		if ((f->data[i] & 0xF0) > 0x90) {
			return 0;
		}

		if (COB_FIELD_NO_SIGN_NIBBLE (f)) {
			/* COMP-6 - Check last nibble */
			if ((f->data[i] & 0x0F) > 0x09) {
				return 0;
			}
			return 1;
		}

		/* Check sign */
		sign = f->data[i] & 0x0F;
		if (COB_FIELD_HAVE_SIGN (f)) {
			if (sign == 0x0C || sign == 0x0D) {
				return 1;
			}
			if (COB_MODULE_PTR->flag_host_sign &&
			    sign == 0x0F) {
				return 1;
			}
		} else if (sign == 0x0F) {
			return 1;
		}
		return 0;
	case COB_TYPE_NUMERIC_DISPLAY:
		return cob_check_numdisp (f);
	case COB_TYPE_NUMERIC_FP_DEC64:
#ifdef	WORDS_BIGENDIAN
		return (f->data[0] & 0x78U) != 0x78U;
#else
		return (f->data[7] & 0x78U) != 0x78U;
#endif
	case COB_TYPE_NUMERIC_FP_DEC128:
#ifdef	WORDS_BIGENDIAN
		return (f->data[0] & 0x78U) != 0x78U;
#else
		return (f->data[15] & 0x78U) != 0x78U;
#endif
	default:
		for (i = 0; i < f->size; ++i) {
			if (!isdigit (f->data[i])) {
				return 0;
			}
		}
		return 1;
	}
}

int
cob_is_alpha (const cob_field *f)
{
	size_t	i;

	for (i = 0; i < f->size; ++i) {
		if (!isalpha (f->data[i]) && f->data[i] != (unsigned char)' ') {
			return 0;
		}
	}
	return 1;
}

int
cob_is_upper (const cob_field *f)
{
	size_t	i;

	for (i = 0; i < f->size; ++i) {
		if (!isupper (f->data[i]) && f->data[i] != (unsigned char)' ') {
			return 0;
		}
	}
	return 1;
}

int
cob_is_lower (const cob_field *f)
{
	size_t	i;

	for (i = 0; i < f->size; ++i) {
		if (!islower (f->data[i]) && f->data[i] != (unsigned char)' ') {
			return 0;
		}
	}
	return 1;
}

/* Table sort */

void
cob_table_sort_init (const size_t nkeys, const unsigned char *collating_sequence)
{
	sort_nkeys = 0;
	sort_keys = cob_malloc (nkeys * sizeof (cob_file_key));
	if (collating_sequence) {
		sort_collate = collating_sequence;
	} else {
		sort_collate = COB_MODULE_PTR->collating_sequence;
	}
}

void
cob_table_sort_init_key (cob_field *field, const int flag,
			 const unsigned int offset)
{
	sort_keys[sort_nkeys].field = field;
	sort_keys[sort_nkeys].flag = flag;
	sort_keys[sort_nkeys].offset = offset;
	sort_nkeys++;
}

void
cob_table_sort (cob_field *f, const int n)
{
	qsort (f->data, (size_t) n, f->size, sort_compare);
	cob_free (sort_keys);
}

/* Run-time error checking */

void
cob_check_based (const unsigned char *x, const char *name)
{
	if (!x) {
      /* name includes '' already and can be ... 'x' (addressed by 'y'= */
		cob_runtime_error (_("BASED/LINKAGE item %s has NULL address"), name);
		cob_stop_run (1);
	}
}

void
cob_check_linkage (const unsigned char *x, const char *name, const int check_type)
{
	if (!x) {
		/* name includes '' already and can be ... 'x' of 'y' */
		switch(check_type) {
		case 0: /* check for passed items and size on module entry */
			cob_runtime_error (_("LINKAGE item %s not passed by caller"), name);
			break;
		case 1: /* check for passed OPTIONAL items on item use */
			cob_runtime_error (_("LINKAGE item %s not passed by caller"), name);
			break;
		}
		cob_stop_run (1);
	}
}

void
cob_check_numeric (const cob_field *f, const char *name)
{
	unsigned char	*data;
	char		*p;
	char		*buff;
	size_t		i;

	if (!cob_is_numeric (f)) {
		buff = cob_fast_malloc ((size_t)COB_SMALL_BUFF);
		p = buff;
		data = f->data;
		for (i = 0; i < f->size; ++i) {
			if (isprint (data[i])) {
				*p++ = data[i];
			} else {
				p += sprintf (p, "\\%03o", data[i]);
			}
		}
		*p = '\0';
		cob_runtime_error (_("'%s' not numeric: '%s'"), name, buff);
		cob_free (buff);
		cob_stop_run (1);
	}
}

void
cob_check_odo (const int i, const int min,
	       const int max, const char *name)
{
	/* Check OCCURS DEPENDING ON item */
	if (i < min || max < i) {
		cob_set_exception (COB_EC_BOUND_ODO);
		cob_runtime_error (_("OCCURS DEPENDING ON '%s' out of bounds: %d"), name, i);
		cob_stop_run (1);
	}
}

void
cob_check_subscript (const int i, const int min,
		     const int max, const char *name)
{
	/* Check subscript */
	if (i < min || max < i) {
		cob_set_exception (COB_EC_BOUND_SUBSCRIPT);
		cob_runtime_error (_("subscript of '%s' out of bounds: %d"), name, i);
		cob_stop_run (1);
	}
}

void
cob_check_ref_mod (const int offset, const int length,
		   const int size, const char *name)
{
	/* Check offset */
	if (offset < 1 || offset > size) {
		cob_set_exception (COB_EC_BOUND_REF_MOD);
		cob_runtime_error (_("offset of '%s' out of bounds: %d"), name, offset);
		cob_stop_run (1);
	}

	/* Check length */
	if (length < 1 || offset + length - 1 > size) {
		cob_set_exception (COB_EC_BOUND_REF_MOD);
		cob_runtime_error (_("length of '%s' out of bounds: %d"), name, length);
		cob_stop_run (1);
	}
}

void *
cob_external_addr (const char *exname, const int exlength)
{
	struct cob_external *eptr;

	/* Locate or allocate EXTERNAL item */
	for (eptr = basext; eptr; eptr = eptr->next) {
		if (!strcmp (exname, eptr->ename)) {
			if (exlength > eptr->esize) {
				cob_runtime_error (_("EXTERNAL item '%s' previously allocated with size %d, requested size is %d"),
						   exname, eptr->esize, exlength);
				cob_stop_run (1);
			}
			cobglobptr->cob_initial_external = 0;
			return eptr->ext_alloc;
		}
	}
	eptr = cob_malloc (sizeof (struct cob_external));
	eptr->next = basext;
	eptr->esize = exlength;
	eptr->ename = cob_malloc (strlen (exname) + 1U);
	strcpy (eptr->ename, exname);
	eptr->ext_alloc = cob_malloc ((size_t)exlength);
	basext = eptr;
	cobglobptr->cob_initial_external = 1;
	return eptr->ext_alloc;
}

/* Retrieving current date and time */

int
cob_ctoi (const char digit)
{
	return (int) (digit - '0');
}

#if defined (_WIN32) && !defined (__CYGWIN__)
struct cob_time
cob_get_current_date_and_time (void)
{
	SYSTEMTIME	local_time;
#if defined(_MSC_VER) && COB_USE_VC2008_OR_GREATER
	FILETIME	filetime;
	SYSTEMTIME	utc_time;
#endif
	struct cob_time	cb_time;

#if defined(_MSC_VER) && COB_USE_VC2008_OR_GREATER
	(time_as_filetime_func) (&filetime);
	/* use fallback to GetLocalTime if one of the following does not work */
	if (!(FileTimeToSystemTime (&filetime, &utc_time) &&
		SystemTimeToTzSpecificLocalTime (NULL, &utc_time, &local_time))) {
		GetLocalTime (&local_time);
	}
#else
	GetLocalTime (&local_time);
#endif

	cb_time.year = local_time.wYear;
	cb_time.month = local_time.wMonth;
	cb_time.day_of_month = local_time.wDay;
	cb_time.day_of_week = one_indexed_day_of_week_from_monday (local_time.wDayOfWeek);
	cb_time.hour = local_time.wHour;
	cb_time.minute = local_time.wMinute;
	cb_time.second = local_time.wSecond;
	cb_time.nanosecond = local_time.wMilliseconds * 1000000;
	cb_time.offset_known = 0;
	cb_time.utc_offset = 0;

#if defined(_MSC_VER) && COB_USE_VC2008_OR_GREATER
	set_cob_time_ns_from_filetime (filetime, &cb_time);
#endif

	set_cob_time_offset (&cb_time);

	return cb_time;
}
#else
struct cob_time
cob_get_current_date_and_time (void)
{
#if defined (HAVE_CLOCK_GETTIME)
	struct timespec	time_spec;
#elif defined(HAVE_SYS_TIME_H) && defined(HAVE_GETTIMEOFDAY)
	struct timeval	tmv;
#endif
	time_t		curtime;
	struct tm	*tmptr;
	struct cob_time	cb_time;
#if defined(COB_STRFTIME)
	char		iso_timezone[6] = { '\0' };
#endif

	/* Get the current time */
#if defined (HAVE_CLOCK_GETTIME)
	clock_gettime (CLOCK_REALTIME, &time_spec);
	curtime = time_spec.tv_sec;
#elif defined (HAVE_SYS_TIME_H) && defined (HAVE_GETTIMEOFDAY)
	gettimeofday(&tmv, NULL);
	curtime = tmv.tv_sec;
#else
	curtime = time (NULL);
#endif
	tmptr = localtime (&curtime);
	/* Leap seconds ? */
	if (tmptr->tm_sec >= 60) {
		tmptr->tm_sec = 59;
	}

	cb_time.year = tmptr->tm_year + 1900;
	cb_time.month = tmptr->tm_mon + 1;
	cb_time.day_of_month = tmptr->tm_mday;
	cb_time.day_of_week = one_indexed_day_of_week_from_monday (tmptr->tm_wday);
	cb_time.hour = tmptr->tm_hour;
	cb_time.minute = tmptr->tm_min;
	cb_time.second = tmptr->tm_sec;
	cb_time.nanosecond = 0;
	cb_time.offset_known = 0;
	cb_time.utc_offset = 0;

	/* Get nanoseconds or microseconds, if possible */
#if defined (HAVE_CLOCK_GETTIME)
	cb_time.nanosecond = (int) time_spec.tv_nsec;
#elif defined (HAVE_SYS_TIME_H) && defined (HAVE_GETTIMEOFDAY)
	cb_time.nanosecond = tmv.tv_usec * 1000;
#else
	cb_time.nanosecond = 0;
#endif

	/* Get the offset from UTC */
#if defined (COB_STRFTIME)
	strftime (iso_timezone, (size_t) 6, "%z", tmptr);

	if (iso_timezone[0] == '0') {
		set_unknown_offset (&cb_time);
	} else {
		/* Convert the timezone string into minutes from UTC */
		cb_time.offset_known = 1;
		cb_time.utc_offset =
			cob_ctoi (iso_timezone[1]) * 60 * 10
			+ cob_ctoi (iso_timezone[2]) * 60
			+ cob_ctoi (iso_timezone[3]) * 10
			+ cob_ctoi (iso_timezone[4]);
		if (iso_timezone[0] == '-') {
			cb_time.utc_offset *= -1;
		}
	}
#elif defined (HAVE_TIMEZONE)
	cb_time.offset_known = 1;
	cb_time.utc_offset = timezone / 60;
#else
	set_unknown_offset(&cb_time);
#endif

	return cb_time;
}
#endif

/* Extended ACCEPT/DISPLAY */

void
cob_accept_date (cob_field *f)
{
	time_t	t;
	char	s[8];

	t = time (NULL);
	strftime (s, (size_t)7, "%y%m%d", localtime (&t));
	cob_memcpy (f, s, (size_t)6);
}

void
cob_accept_date_yyyymmdd (cob_field *f)
{
	time_t	t;
	char	s[12];

	t = time (NULL);
	strftime (s, (size_t)9, "%Y%m%d", localtime (&t));
	cob_memcpy (f, s, (size_t)8);
}

void
cob_accept_day (cob_field *f)
{
	time_t	t;
	char	s[8];

	t = time (NULL);
	strftime (s, (size_t)6, "%y%j", localtime (&t));
	cob_memcpy (f, s, (size_t)5);
}

void
cob_accept_day_yyyyddd (cob_field *f)
{
	time_t	t;
	char	s[12];

	t = time (NULL);
	strftime (s, (size_t)8, "%Y%j", localtime (&t));
	cob_memcpy (f, s, (size_t)7);
}

void
cob_accept_day_of_week (cob_field *f)
{
	struct tm	*tm;
	time_t		t;
	unsigned char	s[4];

	t = time (NULL);
	tm = localtime (&t);
	if (tm->tm_wday == 0) {
		s[0] = (unsigned char)'7';
	} else {
		s[0] = (unsigned char)(tm->tm_wday + '0');
	}
	cob_memcpy (f, s, (size_t)1);
}

void
cob_accept_time (cob_field *field)
{
	struct cob_time	time;
	char		str[9] = { '\0' };

	time = cob_get_current_date_and_time ();
	snprintf (str, 9, "%2.2d%2.2d%2.2d%2.2d", time.hour, time.minute,
		  time.second, time.nanosecond / 10000000);

	cob_memcpy (field, str, (size_t)8);
}

void
cob_display_command_line (cob_field *f)
{
	if (commlnptr) {
		cob_free (commlnptr);
	}
	commlnptr = cob_malloc (f->size + 1U);
	commlncnt = f->size;
	memcpy (commlnptr, f->data, commlncnt);
}

void
cob_accept_command_line (cob_field *f)
{
	char	*buff;
	size_t	i;
	size_t	size;
	size_t	len;

	if (commlncnt) {
		cob_memcpy (f, commlnptr, commlncnt);
		return;
	}

	if (cob_argc <= 1) {
		cob_memcpy (f, " ", (size_t)1);
		return;
	}

	size = 0;
	for (i = 1; i < (size_t)cob_argc; ++i) {
		size += (strlen (cob_argv[i]) + 1);
		if (size > f->size) {
			break;
		}
	}
	buff = cob_malloc (size);
	buff[0] = ' ';
	size = 0;
	for (i = 1; i < (size_t)cob_argc; ++i) {
		len = strlen (cob_argv[i]);
		memcpy (buff + size, cob_argv[i], len);
		size += len;
		if (i != (size_t)cob_argc - 1U) {
			buff[size++] = ' ';
		}
		if (size > f->size) {
			break;
		}
	}
	cob_memcpy (f, buff, size);
	cob_free (buff);
}

/* Argument number */

void
cob_display_arg_number (cob_field *f)
{
	int		n;
	cob_field_attr	attr;
	cob_field	temp;

	temp.size = 4;
	temp.data = (unsigned char *)&n;
	temp.attr = &attr;
	COB_ATTR_INIT (COB_TYPE_NUMERIC_BINARY, 9, 0, 0, NULL);
	cob_move (f, &temp);
	if (n < 0 || n >= cob_argc) {
		cob_set_exception (COB_EC_IMP_DISPLAY);
		return;
	}
	current_arg = n;
}

void
cob_accept_arg_number (cob_field *f)
{
	int		n;
	cob_field_attr	attr;
	cob_field	temp;

	n = cob_argc - 1;
	temp.size = 4;
	temp.data = (unsigned char *)&n;
	temp.attr = &attr;
	COB_ATTR_INIT (COB_TYPE_NUMERIC_BINARY, 9, 0, 0, NULL);
	cob_move (&temp, f);
}

void
cob_accept_arg_value (cob_field *f)
{
	if (current_arg >= cob_argc) {
		cob_set_exception (COB_EC_IMP_ACCEPT);
		return;
	}
	cob_memcpy (f, cob_argv[current_arg],
		    strlen (cob_argv[current_arg]));
	current_arg++;
}

/* Environment variable */

void
cob_display_environment (const cob_field *f)
{
	size_t	i;

	if (cob_local_env_size < f->size) {
		cob_local_env_size = f->size;
		if (cob_local_env) {
			cob_free (cob_local_env);
		}
		cob_local_env = cob_malloc (cob_local_env_size + 1U);
	}
	cob_field_to_string (f, cob_local_env, cob_local_env_size);
	if (unlikely(cobsetptr->cob_env_mangle)) {
		for (i = 0; i < strlen (cob_local_env); ++i) {
			if (!isalnum ((int)cob_local_env[i])) {
				cob_local_env[i] = '_';
			}
		}
	}
}

void
cob_display_env_value (const cob_field *f)
{
	char	*env2;
#if !HAVE_SETENV
	char	*p;
	size_t	len;
#endif
	int		ret;

	if (!cob_local_env) {
		cob_set_exception (COB_EC_IMP_DISPLAY);
		return;
	}
	if (!*cob_local_env) {
		cob_set_exception (COB_EC_IMP_DISPLAY);
		return;
	}
	env2 = cob_malloc (f->size + 1U);
	cob_field_to_string (f, env2, f->size);
#if HAVE_SETENV
	ret = setenv(cob_local_env, env2, 1);
#else
	len = strlen (cob_local_env) + strlen (env2) + 3U;
	p = cob_fast_malloc (len);
	sprintf (p, "%s=%s", cob_local_env, env2);
	ret = putenv (p);
#endif
	cob_free (env2);
	if (ret != 0) {
		cob_set_exception (COB_EC_IMP_DISPLAY);
		return;
	}
	/* Rescan term/screen variables */
	cob_rescan_env_vals ();
}

void
cob_set_environment (const cob_field *f1, const cob_field *f2)
{
	cob_display_environment (f1);
	cob_display_env_value (f2);
}

void
cob_get_environment (const cob_field *envname, cob_field *envval)
{
	const char	*p;
	char		*buff;
	size_t		size;

	if (envname->size == 0 || envval->size == 0) {
		cob_set_exception (COB_EC_IMP_ACCEPT);
		return;
	}

	buff = cob_malloc (envname->size + 1U);
	cob_field_to_string (envname, buff, envname->size);
	if (unlikely(cobsetptr->cob_env_mangle)) {
		for (size = 0; size < strlen (buff); ++size) {
			if (!isalnum ((int)buff[size])) {
				buff[size] = '_';
			}
		}
	}
	p = getenv (buff);
	if (!p) {
		cob_set_exception (COB_EC_IMP_ACCEPT);
		p = " ";
	}
	cob_memcpy (envval, p, strlen (p));
	cob_free (buff);
}

void
cob_accept_environment (cob_field *f)
{
	const char *p = NULL;

	if (cob_local_env) {
		p = getenv (cob_local_env);
	}
	if (!p) {
		cob_set_exception (COB_EC_IMP_ACCEPT);
		p = " ";
	}
	cob_memcpy (f, p, strlen (p));
}

void
cob_chain_setup (void *data, const size_t parm, const size_t size)
{
	size_t	len;

	memset (data, ' ', size);
	if (parm <= (size_t)cob_argc - 1) {
		len = strlen (cob_argv[parm]);
		if (len <= size) {
			memcpy (data, cob_argv[parm], len);
		} else {
			memcpy (data, cob_argv[parm], size);
		}
	} else {
		memset (data, ' ', size);
	}
	cobglobptr->cob_call_params = cob_argc - 1;
}

void
cob_allocate (unsigned char **dataptr, cob_field *retptr,
	      cob_field *sizefld, cob_field *initialize)
{
	void			*mptr;
	struct cob_alloc_cache	*cache_ptr;
	int			fsize;
	cob_field		temp;

	/* ALLOCATE */
	cobglobptr->cob_exception_code = 0;
	mptr = NULL;
	fsize = cob_get_int (sizefld);
	if (fsize > 0) {
		cache_ptr = cob_malloc (sizeof (struct cob_alloc_cache));
		mptr = malloc ((size_t)fsize);
		if (!mptr) {
			cob_set_exception (COB_EC_STORAGE_NOT_AVAIL);
			cob_free (cache_ptr);
		} else {
			if (initialize) {
				temp.size = (size_t)fsize;
				temp.data = mptr;
				temp.attr = &const_alpha_attr;
				cob_move (initialize, &temp);
			} else {
				memset (mptr, 0, (size_t)fsize);
			}
			cache_ptr->cob_pointer = mptr;
			cache_ptr->size = (size_t)fsize;
			cache_ptr->next = cob_alloc_base;
			cob_alloc_base = cache_ptr;
		}
	}
	if (dataptr) {
		*dataptr = mptr;
	}
	if (retptr) {
		*(void **)(retptr->data) = mptr;
	}
}

void
cob_free_alloc (unsigned char **ptr1, unsigned char *ptr2)
{
	struct cob_alloc_cache	*cache_ptr;
	struct cob_alloc_cache	*prev_ptr;
	void			*vptr1;

	/* FREE */
	cobglobptr->cob_exception_code = 0;
	cache_ptr = cob_alloc_base;
	prev_ptr = cob_alloc_base;
	if (ptr1 && *ptr1) {
		vptr1 = *ptr1;
		for (; cache_ptr; cache_ptr = cache_ptr->next) {
			if (vptr1 == cache_ptr->cob_pointer) {
				cob_free (cache_ptr->cob_pointer);
				if (cache_ptr == cob_alloc_base) {
					cob_alloc_base = cache_ptr->next;
				} else {
					prev_ptr->next = cache_ptr->next;
				}
				cob_free (cache_ptr);
				*ptr1 = NULL;
				return;
			}
			prev_ptr = cache_ptr;
		}
		cob_set_exception (COB_EC_STORAGE_NOT_ALLOC);
		return;
	}
	if (ptr2 && *(void **)ptr2) {
		for (; cache_ptr; cache_ptr = cache_ptr->next) {
			if (*(void **)ptr2 == cache_ptr->cob_pointer) {
				cob_free (cache_ptr->cob_pointer);
				if (cache_ptr == cob_alloc_base) {
					cob_alloc_base = cache_ptr->next;
				} else {
					prev_ptr->next = cache_ptr->next;
				}
				cob_free (cache_ptr);
				*(void **)ptr2 = NULL;
				return;
			}
			prev_ptr = cache_ptr;
		}
		cob_set_exception (COB_EC_STORAGE_NOT_ALLOC);
		return;
	}
}

char *
cob_getenv (const char *name)
{
	char	*p;

	if (name) {
		p = getenv (name);
		if (p) {
			return cob_strdup (p);
		}
	}
	return NULL;
}

int
cob_putenv (char *name)
{
	int	ret;

	if (name && strchr (name, '=')) {
		ret = putenv (cob_strdup(name));
		if (!ret) {
			cob_rescan_env_vals ();
		}
		return ret;
	}
	return -1;
}

static const char *
cob_gettmpdir (void)
{
	char	*tmpdir;
	char	*tmp;
#if !HAVE_SETENV
	char	*put;
#endif

	if ((tmpdir = getenv ("TMPDIR")) == NULL) {
		tmp = NULL;
#ifdef	_WIN32
		if ((tmpdir = getenv ("TEMP")) == NULL &&
		    (tmpdir = getenv ("TMP")) == NULL &&
		    (tmpdir = getenv ("USERPROFILE")) == NULL) {
			tmp = cob_fast_malloc (2U);
			strcpy (tmp, ".");
			tmpdir = tmp;
		}
#else
		if ((tmpdir = getenv ("TMP")) == NULL &&
		    (tmpdir = getenv ("TEMP")) == NULL) {
			tmp = cob_fast_malloc (5U);
		    strcpy (tmp, "/tmp");
			tmpdir = tmp;
		}
#endif
#if HAVE_SETENV
		(void)setenv("TMPDIR", tmpdir, 1);
#else
		put = cob_fast_malloc (strlen (tmpdir) + 10);
		sprintf (put, "TMPDIR=%s", tmpdir);
		(void)putenv (cob_strdup(put));
		cob_free ((void *)put);
#endif
		if (tmp) {
			cob_free ((void *)tmp);
			tmpdir = getenv ("TMPDIR");
		}
	}
	return tmpdir;
}

void
cob_temp_name (char *filename, const char *ext)
{
	/* Set temporary file name */
	if (ext) {
		snprintf (filename, (size_t)COB_FILE_MAX, "%s%ccob%d_%d%s",
			cob_gettmpdir(), SLASH_CHAR, cob_sys_getpid(), cob_temp_iteration, ext);
	} else {
		snprintf (filename, (size_t)COB_FILE_MAX, "%s%ccobsort%d_%d",
			cob_gettmpdir(), SLASH_CHAR, cob_sys_getpid(), cob_temp_iteration);
	}
}

void
cob_incr_temp_iteration (void)
{
	cob_temp_iteration++;
}

int
cob_extern_init (void)
{
	/* can be called multiple times (MF docs say: should be done in all threads) */
	if (!cob_initialized) {
		cob_init (0, NULL);
	}
	return 0;
}

char *
cob_command_line (int flags, int *pargc, char ***pargv,
		  char ***penvp, char **pname)
{
#if	0	/* RXWRXW cob_command_line */
	char		**spenvp;
	char		*spname;
#else
	COB_UNUSED (penvp);
	COB_UNUSED (pname);
#endif

	COB_UNUSED (flags);

	if (!cob_initialized) {
		cob_fatal_error (COB_FERROR_INITIALIZED);
	}
	if (pargc && pargv) {
		cob_argc = *pargc;
		cob_argv = *pargv;
	}

#if	0	/* RXWRXW cob_command_line */
	if (penvp) {
		spenvp = *penvp;
	}
	if (pname) {
		spname = *pname;
	}
#endif

	/* What are we supposed to return here? */
	return NULL;
}

int
cob_tidy (void)
{
	struct exit_handlerlist	*h;

	if (!cob_initialized) {
		exit (1);
	}
	if (exit_hdlrs != NULL) {
		h = exit_hdlrs;
		while (h != NULL) {
			h->proc ();
			h = h->next;
		}
	}
	cob_terminate_routines ();
	return 0;
}

/* System routines */

int
cob_sys_exit_proc (const void *dispo, const void *pptr)
{
	struct exit_handlerlist *hp;
	struct exit_handlerlist *h;
	const unsigned char	*x;
	int			(**p)(void);

	COB_CHK_PARMS (CBL_EXIT_PROC, 2);

	memcpy (&p, &pptr, sizeof (void *));
	if (!p || !*p) {
		return -1;
	}

	hp = NULL;
	h = exit_hdlrs;
	/* Remove handler anyway */
	while (h != NULL) {
		if (h->proc == *p) {
			if (hp != NULL) {
				hp->next = h->next;
			} else {
				exit_hdlrs = h->next;
			}
			if (hp) {
				cob_free (hp);
			}
			break;
		}
		hp = h;
		h = h->next;
	}
	x = dispo;
	if   (*x != 0 && *x != 2 && *x != 3) {
		/* Remove handler */
		return 0;
	}
	h = cob_malloc (sizeof(struct exit_handlerlist));
	h->next = exit_hdlrs;
	h->proc = *p;
	exit_hdlrs = h;
	return 0;
}

int
cob_sys_error_proc (const void *dispo, const void *pptr)
{
	struct handlerlist	*hp = NULL;
	struct handlerlist	*h = hdlrs;
	const unsigned char	*x;
	int			(**p)(char *s);

	COB_CHK_PARMS (CBL_ERROR_PROC, 2);

	memcpy (&p, &pptr, sizeof (void *));
	if (!p || !*p) {
		return -1;
	}

	hp = NULL;
	h = hdlrs;
	/* Remove handler anyway */
	while (h != NULL) {
		if (h->proc == *p) {
			if (hp != NULL) {
				hp->next = h->next;
			} else {
				hdlrs = h->next;
			}
			if (hp) {
				cob_free (hp);
			}
			break;
		}
		hp = h;
		h = h->next;
	}
	x = dispo;
	if (*x != 0) {
		/* Remove handler */
		return 0;
	}
	h = cob_malloc (sizeof(struct handlerlist));
	h->next = hdlrs;
	h->proc = *p;
	hdlrs = h;
	return 0;
}

int
cob_sys_system (const void *cmdline)
{
	const char	*cmd;
	char		*buff;
	int		i;

	COB_CHK_PARMS (SYSTEM, 1);

	if (COB_MODULE_PTR->cob_procedure_params[0]) {
		cmd = cmdline;
		i = (int)COB_MODULE_PTR->cob_procedure_params[0]->size;
		if (unlikely(i > COB_MEDIUM_MAX)) {
			cob_runtime_error (_("parameter to SYSTEM call is larger than %d characters"), COB_MEDIUM_MAX);
			cob_stop_run (1);
		}
		i--;
		for (; i >= 0; --i) {
			if (cmd[i] != ' ' && cmd[i] != 0) {
				break;
			}
		}
		if (i >= 0) {
			buff = cob_malloc ((size_t)(i + 2));
			memcpy (buff, cmd, (size_t)(i + 1));
			if (cobglobptr->cob_screen_initialized) {
				cob_screen_set_mode (0);
			}
			i = system (buff);
			cob_free (buff);
			if (cobglobptr->cob_screen_initialized) {
				cob_screen_set_mode (1U);
			}
			return i;
		}
	}
	return 1;
}

int
cob_sys_hosted (void *p, const void *var)
{
	const char		*name = var;
	cob_u8_ptr		data = p;
	size_t			i;

	COB_CHK_PARMS (CBL_OC_HOSTED, 2);

	if (!data) {
		return 1;
	}

	if (COB_MODULE_PTR->cob_procedure_params[1]) {
		i = (int)COB_MODULE_PTR->cob_procedure_params[1]->size;
		if ((i == 4) && !strncmp (name, "argc", 4)) {
			*((int *)data) = cob_argc;
			return 0;
		}
		if ((i == 4) && !strncmp (name, "argv", 4)) {
			*((char ***)data) = cob_argv;
			return 0;
		}
		if ((i == 5) && !strncmp (name, "stdin", 5)) {
			*((FILE **)data) = stdin;
			return 0;
		}
		if ((i == 6) && !strncmp (name, "stdout", 6)) {
			*((FILE **)data) = stdout;
			return 0;
		}
		if ((i == 6) && !strncmp (name, "stderr", 6)) {
			*((FILE **)data) = stderr;
			return 0;
		}
		if ((i == 5) && !strncmp (name, "errno", 5)) {
			*((int **)data) = &errno;
			return 0;
		}
#if defined(HAVE_TIMEZONE)
		if ((i == 6) && !strncmp (name, "tzname", 6)) {
			*((char ***)data) = tzname;
			return 0;
		}
		if ((i == 8) && !strncmp (name, "timezone", 8)) {
			*((long *)data) = timezone;
			return 0;
		}
		if ((i == 8) && !strncmp (name, "daylight", 8)) {
			*((int *)data) = daylight;
			return 0;
		}
#endif /* HAVE_TIMEZONE */
	}
	return 1;
}

int
cob_sys_and (const void *p1, void *p2, const int length)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_AND, 3);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_2[n] &= data_1[n];
	}
	return 0;
}

int
cob_sys_or (const void *p1, void *p2, const int length)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_OR, 3);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_2[n] |= data_1[n];
	}
	return 0;
}

int
cob_sys_nor (const void *p1, void *p2, const int length)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_NOR, 3);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_2[n] = ~(data_1[n] | data_2[n]);
	}
	return 0;
}

int
cob_sys_xor (const void *p1, void *p2, const int length)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_XOR, 3);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_2[n] ^= data_1[n];
	}
	return 0;
}

int
cob_sys_imp (const void *p1, void *p2, const int length)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_IMP, 3);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_2[n] = (~data_1[n]) | data_2[n];
	}
	return 0;
}

int
cob_sys_nimp (const void *p1, void *p2, const int length)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_NIMP, 3);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_2[n] = data_1[n] & (~data_2[n]);
	}
	return 0;
}

int
cob_sys_eq (const void *p1, void *p2, const int length)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_EQ, 3);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_2[n] = ~(data_1[n] ^ data_2[n]);
	}
	return 0;
}

int
cob_sys_not (void *p1, const int length)
{
	cob_u8_ptr	data_1 = p1;
	size_t		n;

	COB_CHK_PARMS (CBL_NOT, 2);

	if (length <= 0) {
		return 0;
	}
	for (n = 0; n < (size_t)length; ++n) {
		data_1[n] = ~data_1[n];
	}
	return 0;
}

int
cob_sys_xf4 (void *p1, const void *p2)
{
	cob_u8_ptr		data_1 = p1;
	const cob_u8_ptr	data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_XF4, 2);

	*data_1 = 0;
	for (n = 0; n < 8; ++n) {
		*data_1 |= (data_2[n] & 1) << (7 - n);
	}
	return 0;
}

int
cob_sys_xf5 (const void *p1, void *p2)
{
	const cob_u8_ptr	data_1 = p1;
	cob_u8_ptr		data_2 = p2;
	size_t			n;

	COB_CHK_PARMS (CBL_XF5, 2);

	for (n = 0; n < 8; ++n) {
		data_2[n] = (*data_1 & (1 << (7 - n))) ? 1 : 0;
	}
	return 0;
}

int
cob_sys_x91 (void *p1, const void *p2, void *p3)
{
	cob_u8_ptr		result = p1;
	const cob_u8_ptr	func = p2;
	cob_u8_ptr		parm = p3;
	unsigned char		*p;
	size_t			i;

	switch (*func) {
	case 11:
		/* Set switches */
		p = parm;
		for (i = 0; i < 8; ++i, ++p) {
			if (*p == 0) {
				cob_switch[i] = 0;
			} else if (*p == 1) {
				cob_switch[i] = 1;
			}
		}
		*result = 0;
		break;
	case 12:
		/* Get switches */
		p = parm;
		for (i = 0; i < 8; ++i, ++p) {
			*p = (unsigned char)cob_switch[i];
		}
		*result = 0;
		break;
	case 16:
		/* Return number of call parameters */
		*parm = (unsigned char)COB_MODULE_PTR->module_num_params;
		*result = 0;
		break;
	default:
		*result = 1;
		break;
	}
	return 0;
}

int
cob_sys_toupper (void *p1, const int length)
{
	cob_u8_ptr	data = p1;
	size_t		n;

	COB_CHK_PARMS (CBL_TOUPPER, 2);

	if (length > 0) {
		for (n = 0; n < (size_t)length; ++n) {
			if (islower (data[n])) {
				data[n] = (cob_u8_t)toupper (data[n]);
			}
		}
	}
	return 0;
}

int
cob_sys_tolower (void *p1, const int length)
{
	cob_u8_ptr	data = p1;
	size_t		n;

	COB_CHK_PARMS (CBL_TOLOWER, 2);

	if (length > 0) {
		for (n = 0; n < (size_t)length; ++n) {
			if (isupper (data[n])) {
				data[n] = (cob_u8_t)tolower (data[n]);
			}
		}
	}
	return 0;
}

int
cob_sys_oc_nanosleep (const void *data)
{
	cob_s64_t	nsecs;
#if	defined(_WIN32) || defined(__370__) || defined(__OS400__)
	unsigned int	msecs;
#elif	defined(HAVE_NANO_SLEEP)
	struct timespec	tsec;
#else
	unsigned int	msecs;
#endif

	COB_UNUSED (data);

	COB_CHK_PARMS (CBL_OC_NANOSLEEP, 1);

	if (COB_MODULE_PTR->cob_procedure_params[0]) {
		nsecs = cob_get_llint (COB_MODULE_PTR->cob_procedure_params[0]);
		if (nsecs > 0) {
#ifdef	_WIN32
			msecs = (unsigned int)(nsecs / 1000000);
			if (msecs > 0) {
				Sleep (msecs);
			}
#elif	defined(__370__) || defined(__OS400__)
			msecs = (unsigned int)(nsecs / 1000000000);
			if (msecs > 0) {
				sleep (msecs);
			}
#elif	defined(HAVE_NANO_SLEEP)
			tsec.tv_sec = nsecs / 1000000000;
			tsec.tv_nsec = nsecs % 1000000000;
			nanosleep (&tsec, NULL);
#else
			msecs = (unsigned int)(nsecs / 1000000000);
			if (msecs > 0) {
				sleep (msecs);
			}
#endif
		}
	}
	return 0;
}

int
cob_sys_getpid (void)
{
	if (!cob_process_id) {
		cob_process_id = (int)getpid ();
	}
	return cob_process_id;
}

int
cob_sys_return_args (void *data)
{
	COB_UNUSED (data);

	COB_CHK_PARMS (C$NARG, 1);

	if (COB_MODULE_PTR->cob_procedure_params[0]) {
		cob_set_int (COB_MODULE_PTR->cob_procedure_params[0],
			     COB_MODULE_PTR->module_num_params);
	}
	return 0;
}

int
cob_sys_calledby (void *data)
{
	size_t		size;
	size_t		msize;

	COB_CHK_PARMS (C$CALLEDBY, 1);

	if (!COB_MODULE_PTR->cob_procedure_params[0]) {
		/* TO-DO: check what ACU ccbl/runcbl returns,
		   the documentation doesn't say anything about this */
		return -1;
	}
	size = COB_MODULE_PTR->cob_procedure_params[0]->size;
	memset (data, ' ', size);
	if (!COB_MODULE_PTR->next) {
		return 0;
	}
	msize = strlen (COB_MODULE_PTR->next->module_name);
	if (msize > size) {
		msize = size;
	}
	memcpy (data, COB_MODULE_PTR->next->module_name, msize);
	return 1;
}

int
cob_sys_parameter_size (void *data)
{
	int	n;

	COB_UNUSED (data);

	COB_CHK_PARMS (C$PARAMSIZE, 1);

	if (COB_MODULE_PTR->cob_procedure_params[0]) {
		n = cob_get_int (COB_MODULE_PTR->cob_procedure_params[0]);
		if (n > 0 && n <= COB_MODULE_PTR->module_num_params) {
			n--;
			if (COB_MODULE_PTR->next &&
			    COB_MODULE_PTR->next->cob_procedure_params[n]) {
				return (int)COB_MODULE_PTR->next->cob_procedure_params[n]->size;
			}
		}
	}
	return 0;
}

int
cob_sys_getopt_long_long (void* so, void* lo, void* idx, const int long_only, void* return_char, void* opt_val) {
	/*
	 * cob_argc is a static int containing argc from runtime
	 * cob_argv is a static char** containing argv from runtime
	 */

	size_t opt_val_size = 0;
	size_t so_size = 0;
	size_t lo_size = 0;
	size_t optlen;

	unsigned int lo_amount;

	int exit_status;

	char* shortoptions;
	char* temp;

	struct option* longoptions;
	longoption_def* l = NULL;

	int longind = 0;
	unsigned int i;
	int j;

	int return_value;

	COB_UNUSED (idx);
	COB_UNUSED (lo);
	COB_UNUSED (so);

	COB_CHK_PARMS (CBL_OC_GETOPT, 6);

	/*
	 * Read in sizes of some parameters
	 */
	if (COB_MODULE_PTR->cob_procedure_params[1]) {
		lo_size = COB_MODULE_PTR->cob_procedure_params[1]->size;
	}
	if (COB_MODULE_PTR->cob_procedure_params[0]) {
		so_size = COB_MODULE_PTR->cob_procedure_params[0]->size;
	}
	if (COB_MODULE_PTR->cob_procedure_params[5]) {
		opt_val_size = COB_MODULE_PTR->cob_procedure_params[5]->size;
	}

	/*
	 * Buffering longoptions (cobol), target format (struct option)
	 */
	if (lo_size % sizeof(longoption_def) == 0) {
		lo_amount = (int)lo_size / sizeof(longoption_def);
		longoptions = (struct option*) cob_malloc(sizeof(struct option) * (lo_amount + 1U));
	}
	else {
		cob_runtime_error (_("Call to CBL_OC_GETOPT with wrong longoption size."));
		cob_stop_run (1);
	}

	if (!COB_MODULE_PTR->cob_procedure_params[2]) {
		cob_runtime_error (_("Call to CBL_OC_GETOPT with missing longind."));
		cob_stop_run (1);
	}
	longind = cob_get_int (COB_MODULE_PTR->cob_procedure_params[2]);

	/*
	 * Add 0-termination to strings.
	 */
	shortoptions = cob_malloc(so_size + 1U);
	if (COB_MODULE_PTR->cob_procedure_params[0]) {
		cob_field_to_string (COB_MODULE_PTR->cob_procedure_params[0], shortoptions, so_size);
	}

	if (COB_MODULE_PTR->cob_procedure_params[1]) {
		l = (struct longoption_def*) (COB_MODULE_PTR->cob_procedure_params[1]->data);
	}

	for (i = 0; i < lo_amount; i++) {
		j = sizeof(l->name) - 1;
		while (j >= 0 && l->name[j] == 0x20) {
			l->name[j] = 0x00;
			j--;
		}
		longoptions->name = l->name;
		longoptions->has_arg = (int) l->has_option - '0';
		memcpy (&longoptions->flag, l->return_value_pointer, sizeof(l->return_value_pointer));
		memcpy(&longoptions->val, &l->return_value, 4);

		l = l + 1; /* +1 means pointer + 1*sizeof(longoption_def) */
		longoptions = longoptions + 1;
	}

	/*
	 * Appending final record, so getopt can spot the end of longoptions
	 */
	longoptions->name = NULL;
	longoptions->has_arg = 0;
	longoptions->flag = NULL;
	longoptions->val = 0;


	l -= lo_amount; /* Set pointer back to begin of longoptions */
	longoptions -= lo_amount;

	return_value = cob_getopt_long_long(cob_argc, cob_argv, shortoptions, longoptions, &longind, long_only);
	temp = (char*) &return_value;

	/*
	 * Write data back to COBOL
	 */
	if (temp[0] == '?' || temp[0] == ':' || temp[0] == 'W'
		|| temp[0] == -1 || temp[0] == 0) exit_status = return_value;
	else exit_status = 3;

	for(i = 3; i > 0; i--) {
		if(temp[i] == 0x00) temp[i] = 0x20;
		else break;
	}

	cob_set_int (COB_MODULE_PTR->cob_procedure_params[2], longind);
	memcpy (return_char, &return_value, 4);

	if(cob_optarg != NULL) {
		memset (opt_val, 0x00, opt_val_size);

		optlen = strlen (cob_optarg);
		if (optlen > opt_val_size) {
			/* Returncode 2 for "Optionvalue too long => cut" */
			optlen = opt_val_size;
			exit_status = 2;
		}
		memcpy (opt_val, cob_optarg, optlen);
	}


	cob_free (shortoptions);
	cob_free (longoptions);

	return exit_status;

}

int
cob_sys_sleep (const void *data)
{
	int	n;

	COB_UNUSED (data);

	COB_CHK_PARMS (C$SLEEP, 1);

	if (COB_MODULE_PTR->cob_procedure_params[0]) {
		n = cob_get_int (COB_MODULE_PTR->cob_procedure_params[0]);
		if (n > 0 && n < 3600*24*7) {
#ifdef	_WIN32
			Sleep (n*1000);
#else
			sleep ((unsigned int)n);
#endif
		}
	}
	return 0;
}

int
cob_sys_printable (void *p1, ...)
{
	cob_u8_ptr		data;
	unsigned char		*dotptr;
	size_t			datalen;
	size_t			n;
	unsigned char		dotrep;
	va_list			args;

	COB_CHK_PARMS (C$PRINTABLE, 1);

	if (!COB_MODULE_PTR->cob_procedure_params[0]) {
		return 0;
	}
	data = p1;
	datalen = COB_MODULE_PTR->cob_procedure_params[0]->size;
	if (cobglobptr->cob_call_params > 1) {
		va_start (args, p1);
		dotptr = va_arg (args, unsigned char *);
		va_end (args);
		dotrep = *dotptr;
	} else {
		dotrep = (unsigned char)'.';
	}
	for (n = 0; n < datalen; ++n) {
		if (!isprint (data[n])) {
			data[n] = dotrep;
		}
	}
	return 0;
}

int
cob_sys_justify (void *p1, ...)
{
	cob_u8_ptr	data;
	unsigned char	*direction;
	size_t		datalen;
	size_t		left;
	size_t		right;
	size_t		movelen;
	size_t		centrelen;
	size_t		n;
	size_t		shifting;
	va_list		args;

	COB_CHK_PARMS (C$JUSTIFY, 1);

	if (!COB_MODULE_PTR->cob_procedure_params[0]) {
		return 0;
	}
	data = p1;
	datalen = COB_MODULE_PTR->cob_procedure_params[0]->size;
	if (datalen < 2) {
		return 0;
	}
	if (data[0] != ' ' && data[datalen - 1] != ' ') {
		return 0;
	}
	for (left = 0; left < datalen; ++left) {
		if (data[left] != ' ') {
			break;
		}
	}
	if (left == datalen) {
		return 0;
	}
	right = 0;
	for (n = datalen - 1; ; --n, ++right) {
		if (data[n] != ' ') {
			break;
		}
		if (n == 0) {
			break;
		}
	}
	movelen = datalen - left - right;
	shifting = 0;
	if (cobglobptr->cob_call_params > 1) {
		va_start (args, p1);
		direction = va_arg (args, unsigned char *);
		va_end (args);
		if (*direction == 'L') {
			shifting = 1;
		} else if (*direction == 'C') {
			shifting = 2;
		}
	}
	switch (shifting) {
	case 1:
		memmove (data, &data[left], movelen);
		memset (&data[movelen], ' ', datalen - movelen);
		break;
	case 2:
		centrelen = (left + right) / 2;
		memmove (&data[centrelen], &data[left], movelen);
		memset (data, ' ', centrelen);
		if ((left + right) % 2) {
			memset (&data[centrelen + movelen], ' ', centrelen + 1);
		} else {
			memset (&data[centrelen + movelen], ' ', centrelen);
		}
		break;
	default:
		memmove (&data[left + right], &data[left], movelen);
		memset (data, ' ', datalen - movelen);
		break;
	}
	return 0;
}

void
cob_set_locale (cob_field *locale, const int category)
{
#ifdef	HAVE_SETLOCALE
	char	*p;
	char	*buff;

	p = NULL;
	if (locale) {
		if (locale->size == 0) {
			return;
		}
		buff = cob_malloc (locale->size + 1U);
		cob_field_to_string (locale, buff, locale->size);
	} else {
		buff = NULL;
	}

	switch (category) {
	case COB_LC_COLLATE:
		p = setlocale (LC_COLLATE, buff);
		break;
	case COB_LC_CTYPE:
		p = setlocale (LC_CTYPE, buff);
		break;
#ifdef	LC_MESSAGES
	case COB_LC_MESSAGES:
		p = setlocale (LC_MESSAGES, buff);
		break;
#endif
	case COB_LC_MONETARY:
		p = setlocale (LC_MONETARY, buff);
		break;
	case COB_LC_NUMERIC:
		p = setlocale (LC_NUMERIC, buff);
		break;
	case COB_LC_TIME:
		p = setlocale (LC_TIME, buff);
		break;
	case COB_LC_ALL:
		p = setlocale (LC_ALL, buff);
		break;
	case COB_LC_USER:
		if (cobglobptr->cob_locale_orig) {
			p = setlocale (LC_ALL, cobglobptr->cob_locale_orig);
			(void)setlocale (LC_NUMERIC, "C");
		}
		break;
	case COB_LC_CLASS:
		if (cobglobptr->cob_locale_ctype) {
			p = setlocale (LC_CTYPE, cobglobptr->cob_locale_ctype);
		}
		break;
	}
	if (buff) {
		cob_free (buff);
	}
	if (!p) {
		cob_set_exception (COB_EC_LOCALE_MISSING);
		return;
	}
	p = setlocale (LC_ALL, NULL);
	if (p) {
		if (cobglobptr->cob_locale) {
			cob_free (cobglobptr->cob_locale);
		}
		cobglobptr->cob_locale = cob_strdup (p);
	}
#else
	cob_set_exception (COB_EC_LOCALE_MISSING);
#endif
}


#if 0 /* currently not used */
char *
cob_int_to_string (int i, char* number)
{
	if (!number) return NULL;
	sprintf (number, "%i", i);
	return number;
}

char *
cob_int_to_formatted_bytestring (int i, char* number)
{
	double		d;
	char		*strB;

	if (!number) return NULL;

	strB = (char*) cob_fast_malloc(3);

	if (i > (1024 * 1024)) {
		d = i / 1024.0 / 1024.0;
		strB = (char*) "MB";
	} else if (i > 1024) {
		d = i / 1024.0;
		strB = (char*) "kB";
	} else {
		d = 0;
		strB = (char*) "B";
	}
	sprintf (number, "%3.2f %s", d, strB);
	return number;
}
#endif

char *
cob_strcat (char* str1, char* str2)
{
	size_t		l;
	char		*temp1, *temp2;

	l = strlen (str1) + strlen (str2) + 1;

	/*
	 * If one of the parameter is the buffer itself,
	 * we copy the buffer before continuing.
	 */
	if (str1 == strbuff) {
		temp1 = cob_strdup (str1);
	} else {
		temp1 = str1;
	}
	if (str2 == strbuff) {
		temp2 = cob_strdup (str2);
	} else {
		temp2 = str2;
	}

	if (strbuff) {
		cob_free (strbuff);
	}
	strbuff = (char*) cob_fast_malloc(l);

	sprintf (strbuff, "%s%s", temp1, temp2);
	return strbuff;
}

char *
cob_strjoin (char** strarray, int size, char* separator)
{
	char	*result;
	int		i;

	if(!strarray || size <= 0 || !separator) return NULL;

	result = strarray[0];
	for (i = 1; i < size; i++) {
		result = cob_strcat(result, separator);
		result = cob_strcat(result, strarray[i]);
	}

	return result;
}

/* reentrant version of strerror */
static char *
cob_get_strerror (void)
{
	char * msg;
	msg = cob_cache_malloc ((size_t)COB_ERRBUF_SIZE);
#ifdef HAVE_STRERROR
	strncpy (msg, strerror (errno), COB_ERRBUF_SIZE - 1);
#else
	snprintf (msg, COB_ERRBUF_SIZE - 1, _("system error %d"), errno);
#endif
	return msg;
}

char *
cob_save_env_value (char* env_var, char* env_val)
{
	if (!env_val) return NULL;

	if (env_var) cob_free(env_var);
	env_var = (char*) cob_fast_malloc(strlen(env_val) + 1);
	strcpy(env_var, env_val);

	return env_var;
}

static void
var_print (const char *msg, const char *val, const char *default_val,
		const unsigned int format)
{
	char	*p;
	char	*token;
	size_t	n;
	int	lablen;
	int	toklen;

	switch (format) {
	case 0:
		printf("%-*.*s : ", CB_IMSG_SIZE, CB_IMSG_SIZE, msg);
		break;
		case 1: {
		printf("  %s: ", _("env"));
		lablen = CB_IMSG_SIZE - 2 - (int)strlen(_("env")) - 2;
		printf("%-*.*s : ", lablen, lablen, msg);
			break;
		}
	case 2:
		printf("    %-*.*s     : ", CB_IMSG_SIZE, CB_IMSG_SIZE, msg);
		break;
	case 3:
		printf("        %-*.*s : ", CB_IMSG_SIZE, CB_IMSG_SIZE, msg);
		break;
	default:
		printf("%-*.*s : ", CB_IMSG_SIZE, CB_IMSG_SIZE, msg);
		break;
	}

	if (!val && !default_val) {
		putchar('\n');
		return;
	} else if (val && default_val && ((format != 2 && val[0] == 0x30) || strcmp(val, default_val) == 0)) {
		val = cob_strcat((char*) default_val, (char*) _(" (default)"));
	} else if (!val && default_val) {
		val = default_val;
	}

	if (val && strlen(val) <= CB_IVAL_SIZE) {
		printf("%s", val);
		putchar ('\n');

		return;
	}

	p = cob_strdup (val);

	n = 0;
	token = strtok (p, " ");
	for (; token; token = strtok (NULL, " ")) {
		toklen = (int)strlen (token) + 1;
		if ((n + toklen) > CB_IVAL_SIZE) {
			if (n) {
				if (format == 2 || format == 3)
					printf("\n        %*.*s", CB_IMSG_SIZE + 3,
					CB_IMSG_SIZE + 3, " ");
				else
					printf("\n%*.*s", CB_IMSG_SIZE + 3, CB_IMSG_SIZE + 3, " ");
			}
			n = 0;
		}
		printf ("%s%s", (n ? " " : ""), token);
		n += toklen;
	}
	putchar ('\n');
	cob_free (p);

}


/*
 Expand a string with environment variable in it. Return malloced string.
*/
char *
cob_expand_env_string (char *strval)
{
	unsigned int	i;
	unsigned int	j = 0;
	unsigned int	k = 0;
	unsigned int	envlen = 1280;
	char		*env;
	char		*str = strval;
	char		ename[128] = { '\0' };
	char		*penv;

	env = cob_malloc(envlen);
	for (k = 0; strval[k] != 0; k++) {
		/* String almost full?; Expand it */
		if(j >= envlen - 128) {
			env = cob_realloc(env, envlen, envlen+256);
			envlen += 256;
		}

		/* ${envname:default} */
		if (strval[k] == '$' && strval[k + 1] == '{') {
			k += 2;
			for (i = 0; strval[k] != '}'
				     && strval[k] != 0
				     && strval[k] != ':'; k++) {
				ename[i++] = strval[k];
			}
			ename[i++] = 0;
			penv = getenv (ename);
			if (penv == NULL) {
				/* Copy 'default' value */
				if (strval[k] == ':') {
					k++;
					/* ${name:-default} */
					if (strval[k] == '-') {
						k++;
					}
					while (strval[k] != '}' && strval[k] != 0) {
						if (j >= envlen - 50) {
							env = cob_realloc (env, envlen, envlen + 128);
							envlen += 128;
						}
						env[j++] = strval[k++];
					}
				} else if (strcmp (ename, "COB_CONFIG_DIR") == 0) {
					penv = (char*)COB_CONFIG_DIR;
				} else if (strcmp (ename, "COB_COPY_DIR") == 0) {
					penv = (char*)COB_COPY_DIR;
				}
			}
			if (penv != NULL) {
				if ((j + strlen (penv)) > (unsigned int)(envlen - 128)) {
					env = cob_realloc (env, envlen, strlen (penv) + 256);
					envlen = (unsigned int)strlen (penv) + 256;
				}
				j += sprintf (&env[j], "%s", penv);
				penv = NULL;
			}
			while (strval[k] != '}' && strval[k] != 0) {
				k++;
			}
			if (strval[k] == '}') {
				k++;
			}
			k--;
		} else if (!isspace ((unsigned char)strval[k])) {
			env[j++] = strval[k];
		} else {
			env[j++] = ' ';
		}
	}

	env[j] = '\0';
	str = cob_strdup (env);
	cob_free (env);

	return str;
}

/* Store 'integer' value in field for correct length */
static void
set_value (char *data, int len, long val)
{
	if(len == sizeof(int)) {
		*(int*)data = (int)val;
	} else if(len == sizeof(short)) {
		*(short*)data = (short)val;
	} else if(len == sizeof(long)) {
		*(long*)data = val;
	} else {
		*(char*)data = (char)val;
	}
}

/* Get 'integer' value from field */
static long
get_value (char *data, int len)
{
	if(len == sizeof(int)) {
		return (long)*(int*)data;
	} else if(len == sizeof(short)) {
		return (long)*(short*)data;
	} else if(len == sizeof(long)) {
		return (long)*(long*)data;
	} else {
		return (long)*(char*)data;
	}
}

/* Set runtime setting with given value */
static int					/* returns 1 if any error, else 0 */
set_config_val (char *value, int pos)
{
	void 	*data;
	char	*ptr = value,*str;
	unsigned long	numval = 0;
	int	i,data_type,data_loc,data_len,slen;

	data_type = gc_conf[pos].data_type;
	data_loc  = gc_conf[pos].data_loc;
	data_len  = gc_conf[pos].data_len;

	data = (void*)((char *)cobsetptr + data_loc);

	if(gc_conf[pos].enums) {		/* Translate 'word' into alternate 'value' */

		for (i=0; gc_conf[pos].enums[i].match != NULL; i++) {
			if (strcasecmp(value,gc_conf[pos].enums[i].match) == 0) {
				ptr = value = (char*)gc_conf[pos].enums[i].value;
				break;
			}
			if ((data_type & ENV_ENUMVAL) && strcasecmp(value,gc_conf[pos].enums[i].value) == 0) {
				break;
			}
		}
		if((data_type & ENV_ENUM || data_type & ENV_ENUMVAL)	/* Must be one of the 'enum' values */
		&& gc_conf[pos].enums[i].match == NULL) {
			conf_runtime_error_value(ptr, pos);
			fprintf(stderr, _("should be one of the following values: %s"), "");
			for (i = 0; gc_conf[pos].enums[i].match != NULL; i++) {
				if (i != 0) {
					putc (',', stderr);
					putc (' ', stderr);
				}
				fprintf (stderr, "%s", (char*)gc_conf[pos].enums[i].match);
				if (data_type & ENV_ENUMVAL) {
					fprintf (stderr, "(%s)", (char*)gc_conf[pos].enums[i].value);
				}
			}
			putc ('\n', stderr);
			fflush (stderr);
			return 1;
		}
	}

	if((data_type & ENV_INT) 				/* Integer data */
	|| (data_type & ENV_SIZE) ) {				/* Size: integer with K, M, G */
		for (; *ptr != 0 && (isdigit((unsigned char)*ptr) || *ptr == ' '); ptr++) {
			if (*ptr != ' ') {
				numval = (numval * 10) + (*ptr - '0');
			}
		}
		if((data_type & ENV_SIZE)			/* Size: any K, M, G */
		&& *ptr != 0) {
			switch(toupper((unsigned char)*ptr)) {
			case 'K':
				numval = numval * 1024;
				break;
			case 'M':
				if (numval < 4001) {
					numval = numval * 1024 * 1024;
				} else {
					numval = 4294967295; /* max. guaranteed value for unsigned long */
				}
				break;
			case 'G':
				if (numval < 4) {
					numval = numval * 1024 * 1024 * 1024;
				} else {
					numval = 4294967295; /* max. guaranteed value for unsigned long */
				}
				break;
			}
		}
		if(gc_conf[pos].min_value > 0
		&& numval < gc_conf[pos].min_value) {
			conf_runtime_error_value(value, pos);
			conf_runtime_error(1, _("minimum value: %lu"),gc_conf[pos].min_value);
			return 1;
		}
		if(gc_conf[pos].max_value > 0
		&& numval > gc_conf[pos].max_value) {
			conf_runtime_error_value(value, pos);
			conf_runtime_error(1, _("maximum value: %lu"),gc_conf[pos].max_value);
			return 1;
		}
		set_value((char *)data,data_len,numval);

	} else if((data_type & ENV_BOOL)) {	/* Boolean: Yes/No,True/False,... */
		numval = 2;
		if(isdigit((unsigned char)*ptr)) {
			numval = atoi(ptr);		/* 0 or 1 */
		} else
		if(strcasecmp(ptr,"true") == 0
		|| strcasecmp(ptr,"t") == 0
		|| strcasecmp(ptr,"on") == 0
		|| strcasecmp(ptr,"yes") == 0
		|| strcasecmp(ptr,"y") == 0) {
			numval = 1;			/* True value */
		} else
		if(strcasecmp(ptr,"false") == 0
		|| strcasecmp(ptr,"f") == 0
		|| strcasecmp(ptr,"off") == 0
		|| strcasecmp(ptr,"no") == 0
		|| strcasecmp(ptr,"n") == 0) {
			numval = 0;			/* False value */
		}

		if(numval != 1
		&& numval != 0) {
			conf_runtime_error_value(ptr, pos);
			conf_runtime_error(1, _("should be one of the following values: %s"), "true, false");
			return 1;
		} else {
			if ((data_type & ENV_NOT)) {	/* Negate logic for actual setting */
				numval = !numval;
			}
			set_value((char *)data,data_len,numval);
		}

	} else if((data_type & ENV_STR)
		||(data_type & ENV_FILE)
		||(data_type & ENV_PATH)) {	/* String/Path to be stored as a string */
		memcpy(&str,data,sizeof(char *));
		if (str != NULL) {
			cob_free((void*)str);
		}
		str = cob_expand_env_string(value);
			if((data_type & ENV_FILE)
			&& strchr(str,PATHSEP_CHAR) != NULL) {
				conf_runtime_error_value(value, pos);
				conf_runtime_error(1, _("should not contain '%c'"),PATHSEP_CHAR);
				return 1;
			}
		memcpy(data,&str,sizeof(char *));

	} else if((data_type & ENV_CHAR)) {	/* 'char' field inline */
		memset(data,0,data_len);
		slen = (int)strlen(value);
		if (slen > data_len) {
			slen = data_len;
		}
		memcpy(data,value,slen);
	}
	return 0;
}

/* Set runtime setting by name with given value */
static int					/* returns 1 if any error, else 0 */
set_config_val_by_name (char *value, const char *name, const char *func)
{
	int	i;
	int ret = 1;

	for (i = 0; i < NUM_CONFIG; i++) {
		if (!strcmp(gc_conf[i].conf_name,name)) {
			ret = set_config_val(value, i);
			if (func) {
				gc_conf[i].data_type |= STS_FNCSET;
				gc_conf[i].set_by = FUNC_NAME_IN_DEFAULT;
				gc_conf[i].default_val = func;
			}
			break;
		}
	}
	return ret;
}

/* Return setting value as a 'string' */
static char *
get_config_val (char *value, int pos, char *orgvalue)
{
	void 	*data;
	char	*str;
	double	dval;
	long	numval = 0;
	int	i,data_type,data_loc,data_len;

	data_type	= gc_conf[pos].data_type;
	data_loc	= gc_conf[pos].data_loc;
	data_len	= gc_conf[pos].data_len;

	data = (void*)((char *)cobsetptr + data_loc);

	strcpy(value,"Unknown");
	strcpy(orgvalue,"");
	if((data_type & ENV_INT)) {				/* Integer data */
		numval = get_value((char *)data,data_len);
		sprintf(value,"%ld",numval);

	} else if((data_type & ENV_SIZE)) {			/* Size: integer with K, M, G */
		numval = get_value((char *)data,data_len);
		dval = numval;
		if(numval > (1024 * 1024 * 1024)) {
			if((numval % (1024 * 1024 * 1024)) == 0)
				sprintf(value,"%ld GB",numval/(1024 * 1024 * 1024));
			else
				sprintf(value,"%.2f GB",dval/(1024.0 * 1024.0 * 1024.0));
		} else if(numval > (1024 * 1024)) {
			if((numval % (1024 * 1024)) == 0)
				sprintf(value,"%ld MB",numval/(1024 * 1024));
			else
				sprintf(value,"%.2f MB",dval/(1024.0 * 1024.0));
		} else if(numval > 1024) {
			if((numval % 1024) == 0)
				sprintf(value,"%ld KB",numval/1024);
			else
				sprintf(value,"%.2f KB",dval/1024.0);
		} else {
			sprintf(value,"%ld",numval);
		}

	} else if((data_type & ENV_BOOL)) {	/* Boolean: Yes/No,True/False,... */
		numval = get_value((char *)data,data_len);
		if((data_type & ENV_NOT))
			numval = !numval;
		if(numval)
			strcpy(value,"true");
		else
			strcpy(value,"false");

	/* TO-DO: Consolidate copy-and-pasted code! */
	} else if((data_type & ENV_STR)) {	/* String stored as a string */
		memcpy(&str,data,sizeof(char *));
		if(str == NULL)
			sprintf(value,"%s","not set");
		else
			sprintf(value,"'%s'",str);

	} else if((data_type & ENV_FILE)) {	/* File/path stored as a string */
		memcpy(&str,data,sizeof(char *));
		/* TODO: add special cases here on merging rw-branch */
		if(str == NULL)
			sprintf(value,"%s","not set");
		else
			sprintf(value,"%s",str);

	} else if((data_type & ENV_PATH)) {	/* Path stored as a string */
		memcpy(&str,data,sizeof(char *));
		if(str == NULL)
			sprintf(value,"%s","not set");
		else
			sprintf(value,"%s",str);

	} else if((data_type & ENV_CHAR)) {	/* 'char' field inline */
		if(*(char*)data == 0) {
			strcpy(value,"Nul");
		} else if(isprint(*(unsigned char*)data)) {
			sprintf(value,"'%s'",(char*)data);
		} else {
			sprintf(value,"0x%02X",*(char*)data);
		}
	}

	if(gc_conf[pos].enums) {		/* Translate 'word' into alternate 'value' */
		for (i=0; gc_conf[pos].enums[i].match != NULL; i++) {
			if(strcasecmp(value,gc_conf[pos].enums[i].value) == 0) {
				if(strcmp(value,"0") != 0
				&& strcmp(value,gc_conf[pos].default_val) != 0)
					strcpy(orgvalue,value);
				strcpy(value,gc_conf[pos].enums[i].match);
				break;
			}
		}
	}
	return value;
}

static int
cb_lookup_config (char *keyword)
{
	int	i;
	for (i=0; i < NUM_CONFIG; i++) {		/* Set value from config file */
		if(gc_conf[i].conf_name
		&& strcasecmp(keyword,gc_conf[i].conf_name) == 0) {	/* Look for config file name */
			break;
		}
		if(gc_conf[i].env_name
		&& strcasecmp(keyword,gc_conf[i].env_name) == 0) {	/* Catch using env var name */
			break;
		}
	}
	return i;
}

static int
cb_config_entry (char *buf, int line)
{
	int	i,j,k, old_type;
	void	*data;
	char	*env,*str,qt;
	char	keyword[COB_MINI_BUFF],value[COB_SMALL_BUFF],value2[COB_SMALL_BUFF];

	cob_source_line = line;

	for (j=(int)strlen(buf); buf[j-1] == '\r' || buf[j-1] == '\n'; )	/* Remove CR LF */
		buf[--j] = 0;

	for (i=0; isspace((unsigned char)buf[i]); i++);

	for (j=0; buf[i] != 0 && buf[i] != ':' && !isspace((unsigned char)buf[i]) && buf[i] != '=' && buf[i] != '#'; )
		keyword[j++] = buf[i++];
	keyword[j] = 0;

	while (buf[i] != 0 && ( isspace((unsigned char)buf[i]) || buf[i] == ':' || buf[i] == '=')) i++;
	if (buf[i] == '"'
	||  buf[i] == '\'') {
		qt = buf[i++];
		for (j=0; buf[i] != qt && buf[i] != 0; )
			value[j++] = buf[i++];
	} else {
		for (j=0; !isspace((unsigned char)buf[i]) && buf[i] != '#' && buf[i] != 0; )
			value[j++] = buf[i++];
	}

	value[j] = 0;

	if (strcasecmp (keyword,"setenv") == 0 ) {
		/* collect additional value and push into environment */
		strcpy(value2,"");
		/*check for := in value 2 and split, if necessary*/
		k = 0; while (value[k] != '=' && value[k] != ':' && value[k] != '"' && value[k] != '\'' && value[k] != 0) k++;
		if (value[k] == '=' || value[k] == ':') {
			i = i - (int)strlen(value + k);
			value[k] = 0;
		}
		while(isspace((unsigned char)buf[i]) || buf[i] == ':' || buf[i] == '=') i++;
		if(buf[i] == '"'
		|| buf[i] == '\'') {
			qt = buf[i++];
			for (j=0; buf[i] != qt && buf[i] != 0; )
				value2[j++] = buf[i++];
		} else {
			for (j=0; !isspace((unsigned char)buf[i]) && buf[i] != '#' && buf[i] != 0; )
				value2[j++] = buf[i++];
		}
		value2[j] = 0;
		if (strcmp(value2, "") == 0) {
			conf_runtime_error(1, _("WARNING - '%s %s' without a value - ignored!"), keyword, value);
			return 2;
		}
		/* check additional value for inline env vars ${varname:-default} */
		str = cob_expand_env_string(value2);
		env = cob_malloc(strlen(value)+strlen(str)+2);
		sprintf(env,"%s=%s",value,str);
		(void)putenv(env);
		cob_free(str);
		for (i=0; i < NUM_CONFIG; i++) {		/* Set value from config file */
			if(gc_conf[i].env_name
			&& strcasecmp(value,gc_conf[i].env_name) == 0) {/* no longer cleared by runtime.cfg */
				gc_conf[i].data_type &= ~STS_ENVCLR;
				break;
			}
		}
		return 0;
	}

	if (strcasecmp (keyword,"unsetenv") == 0) {
		if (strcmp(value, "") == 0) {
			conf_runtime_error(1, _("WARNING - '%s' without a value - ignored!"), keyword);
			return 2;
		}
		if ( (env = getenv(value)) != NULL ) {
			for (i=0; i < NUM_CONFIG; i++) {		/* Set value from config file */
				if(gc_conf[i].env_name
				&& strcasecmp(value,gc_conf[i].env_name) == 0) {	/* Catch using env var name */
					gc_conf[i].data_type |= STS_ENVCLR;
					break;
				}
			}
#if HAVE_SETENV
			(void)unsetenv(value);
#else
			env = cob_malloc(strlen(value)+2);
			sprintf(env,"%s=",value);
			(void)putenv(env);

#endif
		}
		return 0;
	}

	if (strcasecmp (keyword, "include") == 0 ||
		strcasecmp (keyword, "includeif") == 0) {
		if (strcmp(value, "") == 0) {
			conf_runtime_error(1, _("'%s' without a value!"), keyword);
			return -1;
		}
		str = cob_expand_env_string(value);
		strcpy (buf, str);
		cob_free(str);
		if (strcasecmp (keyword, "include") == 0) {
			return 1;
		} else {
			return 3;
		}
	}

	if (strcasecmp (keyword, "reset") == 0) {
		i = cb_lookup_config(value);
		if(i >= NUM_CONFIG) {
			conf_runtime_error (1,_("unknown configuration tag '%s'"), value);
			return -1;
		}
		gc_conf[i].data_type &= ~(STS_ENVSET|STS_CNFSET|STS_ENVCLR);	/* Clear status */
		gc_conf[i].data_type |= STS_RESET;
		gc_conf[i].set_by = 0;
		gc_conf[i].config_num = cobsetptr->cob_config_cur - 1;
		if(gc_conf[i].default_val) {
			set_config_val((char*)gc_conf[i].default_val,i);
		} else if ((gc_conf[i].data_type & ENV_STR)
			|| (gc_conf[i].data_type & ENV_FILE)
			|| (gc_conf[i].data_type & ENV_PATH)) {	/* String/Path stored as a string */
			data = (void*)((char *)cobsetptr + gc_conf[i].data_loc);
			memcpy(&str,data,sizeof(char *));
			if( str != NULL) {
				cob_free((void*)str);
			}
		} else {
			set_config_val((char*)"0",i);
		}
		return 0;
	}

	i = cb_lookup_config(keyword);

	if (i >= NUM_CONFIG) {
		conf_runtime_error (1,_("unknown configuration tag '%s'"), keyword);
		return -1;
	}
	if (strcmp(value, "") == 0) {
		conf_runtime_error(1, _("WARNING - '%s' without a value - ignored!"), keyword);
		return 2;
	}

	old_type = gc_conf[i].data_type;
	gc_conf[i].data_type |= STS_CNFSET;
	if(!set_config_val(value,i)) {
		gc_conf[i].data_type &= ~STS_RESET;
		gc_conf[i].config_num = cobsetptr->cob_config_cur - 1;

		if(gc_conf[i].env_group == GRP_HIDE) {
			for (j=0; j < NUM_CONFIG; j++) {		/* Any alias present? */
				if(j != i
				&& gc_conf[i].data_loc == gc_conf[j].data_loc) {
					gc_conf[j].data_type |= STS_CNFSET;
					gc_conf[j].data_type &= ~STS_RESET;
					gc_conf[j].config_num = gc_conf[i].config_num;
					gc_conf[j].set_by = i;
				}
			}
		}
	} else {
		gc_conf[i].data_type = old_type;
	}
	return 0;
}

static int
cob_load_config_file (const char *config_file, int isoptional)
{
	char			buff[COB_FILE_BUFF], filename[COB_FILE_BUFF];
	char			*penv;
	int			sub_ret, ret;
	unsigned int	i;
	int			line;
	FILE			*conf_fd;

	for (i=0; config_file[i] != 0 && config_file[i] != SLASH_CHAR; i++);
	if (config_file[i] == 0) {			/* Just a name, No directory */
		if (access(config_file, F_OK) != 0) {	/* and file does not exist */
			/* check for path of previous configuration file (for includes) */
			filename[0] = 0;
			if (cobsetptr->cob_config_cur != 0) {
				strcpy(buff, cobsetptr->cob_config_file[cobsetptr->cob_config_cur - 1]);
				for (i = (int)strlen(buff); i != 0 && buff[i] != SLASH_CHAR; i--);
				if (i != 0) {
					buff[i] = 0;
					snprintf(filename, (size_t)COB_FILE_MAX, "%s%s%s", buff, SLASH_STR, config_file);
					if (access(filename, F_OK) == 0) {	/* and prefixed file exist */
						config_file = filename;		/* Prefix last directory */
					} else {
						filename[0] = 0;
					}
				}
			}
			if (filename[0] == 0) {
				/* check for COB_CONFIG_DIR (use default if not in environment) */
				penv = getenv("COB_CONFIG_DIR");
				if (penv != NULL) {
					snprintf(filename, (size_t)COB_FILE_MAX, "%s%s%s", penv, SLASH_STR, config_file);
				} else {
					snprintf(filename, (size_t)COB_FILE_MAX, "%s%s%s", COB_CONFIG_DIR, SLASH_STR, config_file);
				}
				if (access(filename, F_OK) == 0) {	/* and prefixed file exist */
					config_file = filename;		/* Prefix COB_CONFIG_DIR */
				}
			}
		}
	}

	cob_source_file = config_file;

	/* check for recursion */
	for (i=0; i < cobsetptr->cob_config_num; i++) {
		if (strcmp(cobsetptr->cob_config_file[i], config_file) == 0) {
			cob_source_line = 0;
			conf_runtime_error (1,_("recursive inclusion"));
			return -2;
		}
	}

	/* Open the configuration file */
	conf_fd = fopen (config_file, "r");
	if (conf_fd == NULL && !isoptional) {
		cob_source_line = 0;
		conf_runtime_error (1, cob_get_strerror());
		if (cobsetptr->cob_config_file) {
			cob_source_file = cobsetptr->cob_config_file[cobsetptr->cob_config_num-1];
		}
		return -1;
	}
	if (conf_fd != NULL) {
		if (cobsetptr->cob_config_file == NULL) {
			cobsetptr->cob_config_file = cob_malloc(sizeof(char *));
		} else {
			cobsetptr->cob_config_file = cob_realloc(cobsetptr->cob_config_file,
				sizeof(char *)*(cobsetptr->cob_config_num), sizeof(char *)*(cobsetptr->cob_config_num+1));
		}
		cobsetptr->cob_config_file[cobsetptr->cob_config_num++] = strdup (config_file);	/* Save config file name */
		cobsetptr->cob_config_cur = cobsetptr->cob_config_num;
	}


	/* Read the configuration file */
	ret = 0;
	line = 0;
	while ( (conf_fd != NULL)
	&& 	(fgets (buff, COB_SMALL_BUFF, conf_fd) != NULL) ) {
		line++;
		for (i=0; isspace((unsigned char)buff[i]); i++);
		if (buff[i] == 0
		||  buff[i] == '#'
		||  buff[i] == '\r'
		||  buff[i] == '\n')
			continue;	/* Skip comments and blank lines */

		/* Evaluate config line */
		sub_ret = cb_config_entry (buff, line);

		/* Include another configuration file */
		if (sub_ret == 1 || sub_ret == 3) {
			cob_source_line = line;
			sub_ret = cob_load_config_file (buff, sub_ret == 3);
			cob_source_file = config_file;
			if (sub_ret < 0) {
				ret = -1;
				cob_source_line = line;
				conf_runtime_error (1, _("configuration file was included here"));
				break;
			}
		}
		if (sub_ret < ret) ret = sub_ret;
	}
	if (conf_fd) {
		fclose (conf_fd);
		cobsetptr->cob_config_cur--;
	}
	cob_source_file = NULL;
	conf_fd = NULL;

	return ret;
}

/*
 * Load the GnuCOBOL runtime configuration information
 */
int
cob_load_config (void)
{
	char		*env;
	char		conf_file[COB_MEDIUM_BUFF];
	int		isoptional = 1, sts, i, j;


	/* Get the name for the configuration file */
	if ((env = getenv ("COB_RUNTIME_CONFIG")) != NULL && env[0]) {
		strcpy (conf_file, env);
		isoptional = 0;			/* If declared then it is NOT optional */
		if(strchr(conf_file,PATHSEP_CHAR) != NULL) {
			conf_runtime_error(0, _("invalid value '%s' for configuration tag '%s'"), conf_file, "COB_RUNTIME_CONFIG");
			conf_runtime_error(1, _("should not contain '%c'"),PATHSEP_CHAR);
			return -1;
		}
	} else {
		/* check for COB_CONFIG_DIR (use default if not in environment) */
		if ((env = getenv("COB_CONFIG_DIR")) != NULL && env[0]) {
			snprintf (conf_file, (size_t)COB_MEDIUM_MAX, "%s%s%s", env, SLASH_STR, "runtime.cfg");
		} else {
			snprintf (conf_file, (size_t)COB_MEDIUM_MAX, "%s%s%s", COB_CONFIG_DIR, SLASH_STR, "runtime.cfg");
		}
		conf_file[COB_MEDIUM_MAX] = 0; /* fixing code analyser warning */
		isoptional = 1;			/* If not present, then just use env vars */
		if (strchr(conf_file,PATHSEP_CHAR) != NULL) {
			conf_runtime_error(0, _("invalid value '%s' for configuration tag '%s'"), conf_file, "COB_CONFIG_DIR");
			conf_runtime_error(1, _("should not contain '%c'"),PATHSEP_CHAR);
			return -1;
	}
	}

	sprintf (varseq_dflt, "%d", WITH_VARSEQ);		/* Default comes from config.h */
	for (i=0; i < NUM_CONFIG; i++) {
		gc_conf[i].data_type &= ~(STS_ENVSET|STS_CNFSET|STS_ENVCLR);	/* Clear status */
	}

	sts = cob_load_config_file (conf_file, isoptional);

	cob_rescan_env_vals(); 			/* Check for possible environment variables */

	/* Set with default value if present and not set otherwise */
	for (i=0; i < NUM_CONFIG; i++) {
		if (gc_conf[i].default_val
		&& !(gc_conf[i].data_type & STS_CNFSET)
		&& !(gc_conf[i].data_type & STS_ENVSET)) {
			for (j=0; j < NUM_CONFIG; j++) {	/* Any alias present? */
				if(j != i
				&& gc_conf[i].data_loc == gc_conf[j].data_loc)
					break;
			}
			if(j < NUM_CONFIG) {
				if(!(gc_conf[j].data_type & STS_CNFSET)
				&& !(gc_conf[j].data_type & STS_ENVSET)) {	/* alias not defined? */
					set_config_val((char*)gc_conf[i].default_val,i);
				}
			} else {
				set_config_val((char*)gc_conf[i].default_val,i); /* Set default value */
			}
		}
	}

	return sts;
}

void
cob_runtime_error(const char *fmt, ...)
{
	struct handlerlist	*h;
	struct handlerlist	*hp;
	char			*p;
	va_list			ap;

#if	1	/* RXWRXW - Exit screen */
	/* Exit screen mode early */
	cob_exit_screen();
#endif

	if (hdlrs != NULL) {
		if (runtime_err_str) {
			p = runtime_err_str;
			if (cob_source_file) {
				sprintf(runtime_err_str, "%s: %u: ",
					cob_source_file, cob_source_line);
				p = runtime_err_str + strlen(runtime_err_str);
			}
			va_start(ap, fmt);
			vsprintf(p, fmt, ap);
			va_end(ap);
		}
		h = hdlrs;
		while (h != NULL) {
			if (runtime_err_str) {
				h->proc(runtime_err_str);
			}
			else {
				h->proc((char *)_("malloc error"));
			}
			hp = h;
			h = h->next;
			cob_free(hp);
		}
		hdlrs = NULL;
	}

	/* Prefix */
	fputs("libcob: ", stderr);
	if (cob_source_file) {
		fprintf(stderr, "%s: ", cob_source_file);
		if (cob_source_line) {
			fprintf(stderr, "%u: ", cob_source_line);
		}
	}

	/* Body */
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	/* Postfix */
	putc('\n', stderr);
	fflush(stderr);
}

void
cob_fatal_error(const int fatal_error)
{
	const char	*msg;
	unsigned char	*file_status;
	char		*err_cause;
	int		status;
#ifdef	_WIN32
	char		*p;
#endif

	switch (fatal_error) {
#if 0 /* Currently not in use, should enter unknown error */
	case COB_FERROR_NONE:
		cob_runtime_error(_("attempt to CANCEL active program"));
		break;
#endif
	case COB_FERROR_CANCEL:
		cob_runtime_error(_("attempt to CANCEL active program"));
		break;
	case COB_FERROR_INITIALIZED:
#ifdef	_WIN32
		/* cob_unix_lf needs to be set before any error message is thrown,
		as they would have wrong line endings otherwise */
		p = getenv("COB_UNIX_LF");
		if (p && (*p == 'Y' || *p == 'y' ||
			*p == 'T' || *p == 't' ||
			*p == '1')) {
			(void)_setmode(_fileno(stdin), _O_BINARY);
			(void)_setmode(_fileno(stdout), _O_BINARY);
			(void)_setmode(_fileno(stderr), _O_BINARY);
		}
#endif
		cob_runtime_error(_("cob_init() has not been called"));
		break;
	case COB_FERROR_CODEGEN:
		cob_runtime_error(_("codegen error - Please report this!"));
		break;
	case COB_FERROR_CHAINING:
		cob_runtime_error(_("recursive call of chained program"));
		break;
	case COB_FERROR_STACK:
		cob_runtime_error(_("stack overflow, possible PERFORM depth exceeded"));
		break;
	case COB_FERROR_GLOBAL:
		cob_runtime_error(_("invalid entry/exit in GLOBAL USE procedure"));
		break;
	case COB_FERROR_MEMORY:
		cob_runtime_error(_("unable to allocate memory"));
		break;
	case COB_FERROR_MODULE:
		cob_runtime_error(_("invalid entry into module"));
		break;
	case COB_FERROR_RECURSIVE:
		cob_runtime_error(_("invalid recursive COBOL CALL to '%s'"),
			COB_MODULE_PTR->module_name);
		break;
	case COB_FERROR_FREE:
		cob_runtime_error(_("call to %s with NULL pointer"), "cob_free");
		break;
	case COB_FERROR_FILE:
		file_status = cobglobptr->cob_error_file->file_status;
		status = COB_D2I(file_status[0]) * 10 + COB_D2I(file_status[1]);
		switch (status) {
		case COB_STATUS_10_END_OF_FILE:
			msg = _("end of file");
			break;
		case COB_STATUS_14_OUT_OF_KEY_RANGE:
			msg = _("key out of range");
			break;
		case COB_STATUS_21_KEY_INVALID:
			msg = _("key order not ascending");
			break;
		case COB_STATUS_22_KEY_EXISTS:
			msg = _("record key already exists");
			break;
		case COB_STATUS_23_KEY_NOT_EXISTS:
			msg = _("record key does not exist");
			break;
		case COB_STATUS_30_PERMANENT_ERROR:
			msg = _("permanent file error");
			break;
		case COB_STATUS_35_NOT_EXISTS:
			msg = _("file does not exist");
			break;
		case COB_STATUS_37_PERMISSION_DENIED:
			msg = _("permission denied");
			break;
		case COB_STATUS_41_ALREADY_OPEN:
			msg = _("file already open");
			break;
		case COB_STATUS_42_NOT_OPEN:
			msg = _("file not open");
			break;
		case COB_STATUS_43_READ_NOT_DONE:
			msg = _("READ must be executed first");
			break;
		case COB_STATUS_44_RECORD_OVERFLOW:
			msg = _("record overflow");
			break;
		case COB_STATUS_46_READ_ERROR:
			msg = _("READ after uncessful READ/START");
			break;
		case COB_STATUS_47_INPUT_DENIED:
			msg = _("READ/START not allowed, file not open for input");
			break;
		case COB_STATUS_48_OUTPUT_DENIED:
			msg = _("WRITE not allowed, file not open for output");
			break;
		case COB_STATUS_49_I_O_DENIED:
			msg = _("DELETE/REWRITE not allowed, file not open for I-O");
			break;
		case COB_STATUS_51_RECORD_LOCKED:
			msg = _("record locked by another file connector");
			break;
		case COB_STATUS_57_I_O_LINAGE:
			msg = _("LINAGE values invalid");
			break;
		case COB_STATUS_61_FILE_SHARING:
			msg = _("file sharing conflict");
			break;
		case COB_STATUS_91_NOT_AVAILABLE:
			msg = _("runtime library is not configured for this operation");
			break;
		default:
			msg = _("unknown file error");
			break;
		}
		err_cause = cob_malloc((size_t)COB_FILE_BUFF);
		cob_field_to_string(cobglobptr->cob_error_file->assign,
			err_cause, (size_t)COB_FILE_MAX);
		cob_runtime_error(_("%s (status = %02d) file: '%s'"),
			msg, status, err_cause);
		cob_free(err_cause);
		break;
	case COB_FERROR_FUNCTION:
		cob_runtime_error(_("attempt to use non-implemented function"));
		break;
	default:
		cob_runtime_error(_("unknown failure: %d"), fatal_error);
		break;
	}
	cob_stop_run(1);
}

void
conf_runtime_error_value(char *value, const int pos)
{
	const char *name = NULL;

	if (gc_conf[pos].data_type & STS_CNFSET) {
		name = gc_conf[pos].conf_name;
	}
	else {
		name = gc_conf[pos].env_name;
	}
	conf_runtime_error(0, _("invalid value '%s' for configuration tag '%s'"), value, name);
}

void
conf_runtime_error(const int finish_error, const char *fmt, ...)
{
	va_list args;

	if (!conf_runtime_error_displayed) {
		conf_runtime_error_displayed = 1;
		fputs(_("configuration error:"), stderr);
		putc('\n', stderr);
	}

	/* Prefix */
	if (cob_source_file != last_runtime_error_file
		|| cob_source_line != last_runtime_error_line) {
		last_runtime_error_file = cob_source_file;
		last_runtime_error_line = cob_source_line;
		if (cob_source_file) {
			fprintf(stderr, "%s: ", cob_source_file);
		}
		else {
			fprintf(stderr, "%s", _("environment variables"));
			fprintf(stderr, ": ");
		}
		if (cob_source_line) {
			fprintf(stderr, "%u: ", cob_source_line);
		}
	}

	/* Body */
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	/* Postfix */
	if (!finish_error) {
		putc(';', stderr);
		putc('\n', stderr);
		putc('\t', stderr);
	}
	else {
		putc('\n', stderr);
		fflush(stderr);
	}
}


void
print_version (void)
{
	char	cob_build_stamp[COB_MINI_BUFF];
	char	month[64];
	int		status, day, year;

	/* Set up build time stamp */
	memset (cob_build_stamp, 0, (size_t)COB_MINI_BUFF);
	memset (month, 0, sizeof(month));
	day = 0;
	year = 0;
	status = sscanf (__DATE__, "%s %d %d", month, &day, &year);
	if (status == 3) {
		snprintf (cob_build_stamp, (size_t)COB_MINI_MAX,
			  "%s %2.2d %4.4d %s", month, day, year, __TIME__);
	} else {
		snprintf (cob_build_stamp, (size_t)COB_MINI_MAX,
			  "%s %s", __DATE__, __TIME__);
	}

	printf ("libcob (%s) %s.%d\n",
		PACKAGE_NAME, PACKAGE_VERSION, PATCH_LEVEL);
	puts ("Copyright (C) 2016 Free Software Foundation, Inc.");
	puts (_("License LGPLv3+: GNU LGPL version 3 or later <http://gnu.org/licenses/lgpl.html>"));
	puts (_("This is free software; see the source for copying conditions.  There is NO\n"
	        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."));
	printf (_("Written by %s\n"), "Keisuke Nishida, Roger While, Ron Norman, Simon Sobisch, Edward Hart");
	printf (_("Built     %s"), cob_build_stamp);
	putchar ('\n');
	printf (_("Packaged  %s"), COB_TAR_DATE);
	putchar ('\n');

}

void
print_info (void)
{
	char	buff[16];
	char	*s;

	print_version ();
	putchar ('\n');
	puts (_("build information"));
	var_print (_("build environment"),	COB_BLD_BUILD, "", 0);
	var_print ("CC", COB_BLD_CC, "", 0);
	var_print ("CPPFLAGS", COB_BLD_CPPFLAGS, "", 0);
	var_print ("CFLAGS", COB_BLD_CFLAGS, "", 0);
	var_print ("LD", COB_BLD_LD, "", 0);
	var_print ("LDFLAGS", COB_BLD_LDFLAGS, "", 0);
	putchar ('\n');
	printf (_("C version %s%s"), OC_C_VERSION_PRF, OC_C_VERSION);
	putchar ('\n');

	puts (_("GnuCOBOL information"));

	var_print ("COB_MODULE_EXT", COB_MODULE_EXT, "", 0);
#if 0 /* only relevant for cobc */
	var_print ("COB_OBJECT_EXT", COB_OBJECT_EXT, "", 0);
	var_print ("COB_EXEEXT", COB_EXEEXT, "", 0);
#endif

#if	defined(USE_LIBDL) || defined(_WIN32)
	var_print (_("Dynamic loading"),	"system", "", 0);
#else
	var_print (_("Dynamic loading"),	"libtool", "", 0);
#endif

#if 0 /* Simon: only a marginal performance influence - removed from output */
#ifdef	COB_PARAM_CHECK
	var_print ("\"CBL_\" param check",	_("enabled"), "", 0);
#else
	var_print ("\"CBL_\" param check",	_("disabled"), "", 0);
#endif
#endif

#ifdef COB_64_BIT_POINTER
	var_print ("64bit-mode",	_("yes"), "", 0);
#else
	var_print ("64bit-mode",	_("no"), "", 0);
#endif

#ifdef	COB_LI_IS_LL
	var_print ("BINARY-C-LONG",	_("8 bytes"), "", 0);
#else
	var_print ("BINARY-C-LONG", _("4 bytes"), "", 0);
#endif

	var_print (_("Extended screen I/O"),	WITH_CURSES, "", 0);

	snprintf (buff, sizeof(buff), "%d", WITH_VARSEQ);
	var_print (_("Variable format"), buff, "", 0);
	if ((s = getenv ("COB_VARSEQ_FORMAT")) != NULL) {
		var_print ("COB_VARSEQ_FORMAT", s, "", 1);
	}

#ifdef	WITH_SEQRA_EXTFH
	var_print (_("Sequential handler"),	_("EXTFH"), "", 0);
#else
	var_print (_("Sequential handler"), _("built-in"), "", 0);
#endif

#if defined	(WITH_INDEX_EXTFH)
	var_print (_("ISAM handler"),		_("EXTFH"), "", 0);
#elif defined	(WITH_DB)
	var_print (_("ISAM handler"),		"BDB", "", 0);
#elif defined	(WITH_CISAM)
	var_print (_("ISAM handler"),		"C-ISAM" "", 0);
#elif defined	(WITH_DISAM)
	var_print (_("ISAM handler"),		"D-ISAM", "", 0);
#elif defined	(WITH_VBISAM)
	var_print (_("ISAM handler"),		"VBISAM", "", 0);
#else
	var_print (_("ISAM handler"),		_("disabled"), "", 0);
#endif
}


void
print_runtime_conf()
{
	unsigned int 	i,j,k,vl,dohdg,hdlen,plen;
	char	value[COB_MEDIUM_BUFF],orgvalue[COB_MINI_BUFF];

	printf ("%s %s.%d ", PACKAGE_NAME, PACKAGE_VERSION, PATCH_LEVEL);
	puts (_("runtime configuration"));
	if (cobsetptr->cob_config_file) {
		strcpy(value, _("via"));
		hdlen = (unsigned int)strlen(value) + 3;

		/* output path of main configuration file */
		printf(" %s  ", value);
		plen = 80 - hdlen;
		strcpy(value, cobsetptr->cob_config_file[0]);
		vl = (unsigned int)strlen(value);
		for (k = 0; vl > plen; vl -= plen, k += plen) {
			printf("%.*s\n%-*s", plen, &value[k], hdlen, "");
		}
		printf("%s\n", &value[k]);

		/* output path of additional configuration files */
		for (i = 1; i < cobsetptr->cob_config_num; i++) {
			printf("%*d  ", hdlen - 2, i);
			strcpy(value, cobsetptr->cob_config_file[i]);
			vl = (unsigned int)strlen(value);
			for (k = 0; vl > plen; vl -= plen, k += plen) {
				printf("%.*s\n%-*s", plen, &value[k], hdlen, "");
			}
			printf("%s\n", &value[k]);
		}

	}
	putchar('\n');
	strcpy(value,"todo");
	hdlen = 15;
	for (i=0; i < NUM_CONFIG; i++) {
		j = (unsigned int)strlen(gc_conf[i].env_name);
		if(j > hdlen)
			hdlen = j;
		j = (unsigned int)strlen(gc_conf[i].conf_name);
		if(j > hdlen)
			hdlen = j;
	}

	for (j=1; j < GRP_MAX; j++) {
		dohdg = 1;
		for (i=0; i < NUM_CONFIG; i++) {
			if(gc_conf[i].env_group == j) {
				if(dohdg) {
					dohdg = 0;
					if (j > 1) {
						putchar('\n');
					}
					printf(" %s\n",setting_group[j]);
				}
				/* Convert value back into string and display it */
				get_config_val(value,i,orgvalue);
				if((gc_conf[i].data_type & STS_ENVSET)
				|| (gc_conf[i].data_type & STS_FNCSET)) {
					putchar(' ');
					if (gc_conf[i].data_type & STS_FNCSET) {
						printf("   ");
					} else if ((gc_conf[i].data_type & STS_CNFSET)) {
						printf("Ovr");
					} else {
						printf("env");
					}
					printf(": %-*s : ",hdlen,gc_conf[i].env_name);
				} else if((gc_conf[i].data_type & STS_CNFSET)) {
					if (gc_conf[i].config_num > 0) {
						printf("  %d ", gc_conf[i].config_num);
					} else {
						printf("    ");
					}
					if (gc_conf[i].set_by > 0) {
						printf(": %-*s : ", hdlen, gc_conf[i].env_name);
					} else {
						printf(": %-*s : ", hdlen, gc_conf[i].conf_name);
					}
				} else if(gc_conf[i].env_name) {
					if(gc_conf[i].config_num > 0){
						printf("  %d ",gc_conf[i].config_num);
					} else {
						printf("    ");
					}
					printf(": %-*s : ",hdlen,gc_conf[i].env_name);
				} else {
					printf("    : %-*s : ",hdlen,gc_conf[i].conf_name);
				}
				vl = (unsigned int)strlen(value);
				plen = 71 - hdlen;
				for (k = 0; vl > plen; vl -= plen, k += plen) {
					printf("%.*s\n      %-*s : ", plen, &value[k], hdlen, "");
				}
				printf("%s",&value[k]);
				if (orgvalue[0] > ' ') {
					printf(" (%s)", orgvalue);
				}
				if (gc_conf[i].set_by > 0) {
					putchar(' ');
					if (gc_conf[i].set_by != FUNC_NAME_IN_DEFAULT) {
						printf(_("(set by %s)"), gc_conf[gc_conf[i].set_by].env_name);
					} else {
						printf(_("(set by %s)"), gc_conf[i].default_val);
					}
				}
				if(!(gc_conf[i].data_type & STS_ENVSET)
				&& !(gc_conf[i].data_type & STS_CNFSET)
				&& !(gc_conf[i].data_type & STS_FNCSET)) {
					putchar(' ');
					if ((gc_conf[i].data_type & STS_RESET)) {
						printf(_("(reset)"));
					} else if (strcmp(value, not_set) != 0) {
						printf(_("(default)"));
					}
				}
				putchar('\n');
				if ((gc_conf[i].data_type & STS_ENVCLR)) {
					puts("    :  ");
					puts(_("... removed from environment"));
				}
			}
		}
	}


	/* checkme

	var_print ("resolve_path",
			cob_strjoin(&cobsetptr->cob_library_path, *(cobsetptr->resolve_size),
					(char*) PATHSEPS), not_set, 3);
	*/
	//var_print ("base_preload_ptr",
	//		cobsetptr->cob_preload_str, not_set, 3);


#ifdef	HAVE_SETLOCALE
	printf("    : %-*s : %s\n",hdlen,"LC_CTYPE", (char*) setlocale (LC_CTYPE, NULL));
	printf("    : %-*s : %s\n",hdlen,"LC_NUMERIC", (char*) setlocale (LC_NUMERIC, NULL));
	printf("    : %-*s : %s\n",hdlen,"LC_COLLATE", (char*) setlocale (LC_COLLATE, NULL));
#ifdef	LC_MESSAGES
	printf("    : %-*s : %s\n",hdlen,"LC_MESSAGES", (char*) setlocale (LC_MESSAGES, NULL));
#endif
	printf("    : %-*s : %s\n",hdlen,"LC_MONETARY", (char*) setlocale (LC_MONETARY, NULL));
	printf("    : %-*s : %s\n",hdlen,"LC_TIME", (char*) setlocale (LC_TIME, NULL));
#endif
}

cob_settings *
cob_get_settings_ptr()
{
	return cobsetptr;
}

void
cob_init (const int argc, char **argv)
{
	char		*s;
#if	defined(HAVE_READLINK) || defined(HAVE_GETEXECNAME)
	const char	*path;
#endif
#ifdef	ENABLE_NLS
	const char* localedir;
#endif
#if defined(_MSC_VER) && COB_USE_VC2008_OR_GREATER
	HMODULE		kernel32_handle;
#endif
	int		i;

#if 0	/* Simon: Should not happen - is it neccessary anywhere?
		   We may change this to a runtime warning/error */
	if (cob_initialized) {
		return;
	}
#endif

	cob_set_signal ();

	cob_alloc_base = NULL;
	cob_local_env = NULL;
	cob_last_sfile = NULL;
	commlnptr = NULL;
	basext = NULL;
	sort_keys = NULL;
	sort_collate = NULL;
	cob_current_program_id = NULL;
	cob_current_section = NULL;
	cob_current_paragraph = NULL;
	cob_source_file = NULL;
	cob_source_statement = NULL;
	exit_hdlrs = NULL;
	hdlrs = NULL;
	commlncnt = 0;
	sort_nkeys = 0;
	cob_source_line = 0;
	cob_local_env_size = 0;

	current_arg = 1;

	cob_argc = argc;
	cob_argv = argv;

	/* Get emergency buffer */
	runtime_err_str = cob_fast_malloc ((size_t)COB_ERRBUF_SIZE);

	/* Get global structure */
	cobglobptr = cob_malloc (sizeof(cob_global));

	/* Get settings structure */
	cobsetptr = cob_malloc (sizeof(cob_settings));

	cob_initialized = 1;

#ifdef	HAVE_SETLOCALE
	/* Prime the locale from user settings */
	s = setlocale (LC_ALL, "");
	if (s) {
		/* Save initial values */
		cobglobptr->cob_locale_orig = cob_strdup (s);
		s = setlocale (LC_CTYPE, NULL);
		if (s) {
			cobglobptr->cob_locale_ctype = cob_strdup (s);
		}
		s = setlocale (LC_COLLATE, NULL);
		if (s) {
			cobglobptr->cob_locale_collate = cob_strdup (s);
		}
#ifdef	LC_MESSAGES
		s = setlocale (LC_MESSAGES, NULL);
		if (s) {
			cobglobptr->cob_locale_messages = cob_strdup (s);
		}
#endif
		s = setlocale (LC_MONETARY, NULL);
		if (s) {
			cobglobptr->cob_locale_monetary = cob_strdup (s);
		}
		s = setlocale (LC_NUMERIC, NULL);
		if (s) {
			cobglobptr->cob_locale_numeric = cob_strdup (s);
		}
		s = setlocale (LC_TIME, NULL);
		if (s) {
			cobglobptr->cob_locale_time = cob_strdup (s);
		}
		/* Set to standard "C" locale for COBOL */
		setlocale (LC_NUMERIC, "C");
		setlocale (LC_CTYPE, "C");
		/* Save changed locale */
		s = setlocale (LC_ALL, NULL);
		if (s) {
			cobglobptr->cob_locale = cob_strdup (s);
		}
	}
#endif

#ifdef	ENABLE_NLS
	localedir = getenv("LOCALEDIR");
	if (localedir != NULL) {
		bindtextdomain (PACKAGE, localedir);
	} else {
		bindtextdomain (PACKAGE, LOCALEDIR);
	}
	textdomain (PACKAGE);
#endif

#ifdef	_WIN32
	/* cob_unix_lf needs to be set before configuration load,
	   possible error messages would have wrong line endings otherwise */
	if ((s = getenv("COB_UNIX_LF")) != NULL) {
		set_config_val_by_name (s, "unix_lf", NULL);
	}
	if (cobsetptr->cob_unix_lf) {
		(void)_setmode (_fileno (stdin), _O_BINARY);
		(void)_setmode (_fileno (stdout), _O_BINARY);
		(void)_setmode (_fileno (stderr), _O_BINARY);
	}
#endif

	/* Load runtime configuration file */
	if (unlikely(cob_load_config() < 0)) {
		cob_stop_run (1);
	}

	/* Copy COB_PHYSICAL_CANCEL from settings (internal) to global structure */
	cobglobptr->cob_physical_cancel = cobsetptr->cob_physical_cancel;

	/* Call inits with cobsetptr to get the adresses of all */
	/* Screen-IO might be needed for error outputs */
	cob_init_screenio(cobglobptr, cobsetptr);
	cob_init_numeric(cobglobptr);
	cob_init_strings();
	cob_init_move(cobglobptr, cobsetptr);
	cob_init_intrinsic(cobglobptr);
	cob_init_fileio(cobglobptr, cobsetptr);
	cob_init_call(cobglobptr, cobsetptr);
	cob_init_termio(cobglobptr, cobsetptr);

	/* Set up library routine stuff */
	cobglobptr->cob_term_buff = cob_malloc ((size_t)COB_MEDIUM_BUFF);

	/* Set switches */
	for (i = 0; i <= COB_SWITCH_MAX; ++i) {
		sprintf (runtime_err_str, "COB_SWITCH_%d", i);
		s = getenv (runtime_err_str);
		if (s && (*s == '1' || strcasecmp (s, "ON") == 0)) {
				cob_switch[i] = 1;
		} else {
				cob_switch[i] = 0;
		}
	}

	/* Get user name if not set via environment already */
	if (cobsetptr->cob_user_name == NULL || !strcmp(cobsetptr->cob_user_name, "Unknown")) {
#if defined	(_WIN32) && (!defined(_MSC_VER) || COB_USE_VC2008_OR_GREATER) /*  Needs SDK for earlier versions */
		unsigned long bsiz = COB_ERRBUF_SIZE;
		if (GetUserName (runtime_err_str, &bsiz)) {
			set_config_val_by_name(runtime_err_str, "username", "GetUserName()");
		}
#elif	!defined(__OS400__) && !defined(_MSC_VER)
		s = getlogin ();
		if (s) {
			set_config_val_by_name(s, "username", "getlogin()");
		}
#endif
	}

#if defined(_MSC_VER) && COB_USE_VC2008_OR_GREATER
   /* Get function pointer for most precise time function */
   kernel32_handle = GetModuleHandle (TEXT ("kernel32.dll"));
   if (kernel32_handle != NULL) {
       time_as_filetime_func = (VOID (WINAPI *) (LPFILETIME))
           GetProcAddress (kernel32_handle, "GetSystemTimePreciseAsFileTime");
   }
   if (time_as_filetime_func == NULL) {
       time_as_filetime_func = GetSystemTimeAsFileTime;
   }
#endif

	/* This must be last in this function as we do early return */
	/* from certain ifdef's */

#ifdef	_WIN32
	s = cob_malloc ((size_t)COB_LARGE_BUFF);
	i = GetModuleFileNameA (NULL, s, COB_LARGE_MAX);
	if (i > 0 && i < COB_LARGE_BUFF) {
		cobglobptr->cob_main_argv0 = cob_strdup (s);
		cob_free (s);
		return;
	}
	cob_free (s);
#elif	defined(HAVE_READLINK)
	path = NULL;
	if (!access ("/proc/self/exe", R_OK)) {
		path = "/proc/self/exe";
	} else if (!access ("/proc/curproc/file", R_OK)) {
		path = "/proc/curproc/file";
	} else if (!access ("/proc/self/path/a.out", R_OK)) {
		path = "/proc/self/path/a.out";
	}
	if (path) {
		s = cob_malloc ((size_t)COB_LARGE_BUFF);
		i = (int)readlink (path, s, (size_t)COB_LARGE_MAX);
		if (i > 0 && i < COB_LARGE_BUFF) {
			cobglobptr->cob_main_argv0 = cob_strdup (s);
			cob_free (s);
			return;
		}
		cob_free (s);
	}
#endif

#ifdef	HAVE_GETEXECNAME
	path = getexecname ();
	if (path) {
#ifdef	HAVE_REALPATH
		s = cob_malloc ((size_t)COB_LARGE_BUFF);
		if (realpath (path, s) != NULL) {
			cobglobptr->cob_main_argv0 = cob_strdup (s);
		} else {
			cobglobptr->cob_main_argv0 = cob_strdup (path);
		}
		cob_free (s);
#else
		cobglobptr->cob_main_argv0 = cob_strdup (path);
#endif
		return;
	}
#endif

	if (argc && argv && argv[0]) {
#ifdef	_WIN32
		/* Returns malloced path or NULL */
		cobglobptr->cob_main_argv0 = _fullpath (NULL, argv[0], 1);
#elif	defined(HAVE_CANONICALIZE_FILE_NAME)
		/* Returns malloced path or NULL */
		cobglobptr->cob_main_argv0 = canonicalize_file_name (argv[0]);
#elif	defined(HAVE_REALPATH)
		s = cob_malloc ((size_t)COB_LARGE_BUFF);
		if (realpath (argv[0], s) != NULL) {
			cobglobptr->cob_main_argv0 = cob_strdup (s);
		}
		cob_free (s);
#endif
		if (!cobglobptr->cob_main_argv0) {
			cobglobptr->cob_main_argv0 = cob_strdup (argv[0]);
		}
	} else {
		cobglobptr->cob_main_argv0 = cob_strdup (_("unknown"));
	}
	/* The above must be last in this function as we do early return */
	/* from certain ifdef's */
}
