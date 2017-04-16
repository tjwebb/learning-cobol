/*
   Copyright (C) 2001-2016 Free Software Foundation, Inc.

   Authors:
   Keisuke Nishida, Roger While, Ron Norman, Simon Sobisch, Brian Tiffin,
   Edward Hart, Dave Pitts

   This file is part of GnuCOBOL.

   The GnuCOBOL compiler is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   GnuCOBOL is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GnuCOBOL.  If not, see <http://www.gnu.org/licenses/>.
*/

/*#define DEBUG_REPLACE*/

#include "config.h"
#include "defaults.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef	HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef	_WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifdef	HAVE_LOCALE_H
#include <locale.h>
#endif

#include "tarstamp.h"

#include "cobc.h"
#include "tree.h"

#include "lib/gettext.h"

#include "libcob/cobgetopt.h"

struct strcache {
	struct strcache	*next;
	void		*val;
};

/* Compile level */
#define	CB_LEVEL_PREPROCESS	1
#define	CB_LEVEL_TRANSLATE	2
#define	CB_LEVEL_COMPILE	3
#define	CB_LEVEL_ASSEMBLE	4
#define	CB_LEVEL_MODULE		5
#define	CB_LEVEL_LIBRARY	6
#define	CB_LEVEL_EXECUTABLE	7

/* Info display limits */
#define	CB_IMSG_SIZE		24
#define	CB_IVAL_SIZE		(74 - CB_IMSG_SIZE - 4)

#define	COBC_ADD_STR(v,x,y,z)	cobc_add_str (&v, &v##_size, x, y, z);
#define	COBC_INV_PAR		_("invalid parameter: %s")

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
#define OC_C_VERSION	_("unknown")
#endif

#define	CB_TEXT_LIST_ADD(y,z)	y = cb_text_list_add (y, z)
#define	CB_TEXT_LIST_CHK(y,z)	y = cb_text_list_chk (y, z)

#ifdef	_MSC_VER
#define	CB_COPT_1	" /O1"
#define	CB_COPT_2	" /O2"
#define	CB_COPT_S	" /Os"
#elif   defined(__BORLANDC__)
#define	CB_COPT_1	" -O"
#define	CB_COPT_2	" -O2"
#define	CB_COPT_S	" -O1"
#elif defined(__hpux) && !defined(__GNUC__)
#define	CB_COPT_1	" -O"
#define	CB_COPT_2	" +O2"
#define	CB_COPT_S	" +O2 +Osize"
#elif   defined(__WATCOMC__)
#define	CB_COPT_1	" -ot"
#define	CB_COPT_2	" -ox"
#define	CB_COPT_S	" -os"
#elif   defined(__SUNPRO_C)
#define	CB_COPT_1	" -xO1"
#define	CB_COPT_2	" -xO2"
#define	CB_COPT_S	" -xO1 -xspace"
#elif	defined(__xlc__)
#define	CB_COPT_1	" -O"
#define	CB_COPT_2	" -O2"
#define	CB_COPT_S	" -O"
#else
#define	CB_COPT_1	" -O"
#define	CB_COPT_2	" -O2"
#define	CB_COPT_S	" -Os"
#endif


/* Global variables */

const char		*cb_source_file = NULL;
char			*cb_oc_build_stamp = NULL;
const char		*demangle_name = NULL;
const char		*cb_storage_file_name = NULL;
struct cb_text_list	*cb_include_list = NULL;
struct cb_text_list	*cb_intrinsic_list = NULL;
struct cb_text_list	*cb_extension_list = NULL;
struct cb_text_list	*cb_static_call_list = NULL;
struct cb_text_list	*cb_early_exit_list = NULL;
char			**cb_saveargv = NULL;
const char		*cob_config_dir = NULL;
FILE			*cb_storage_file = NULL;
FILE			*cb_listing_file = NULL;

FILE			*cb_src_list_file = NULL;
int			cb_listing_page = 0;
int			cb_listing_wide = 0;
int			cb_lines_per_page = CB_MAX_LINES;
int			cb_no_symbols = 0;
#define		CB_LISTING_DATE_BUFF	48
#define		CB_LISTING_DATE_MAX		(CB_LISTING_DATE_BUFF - 1)
char			cb_listing_date[48]; /* Date/Time buffer for listing */
struct list_files	*cb_listing_files = NULL;
struct list_files	*cb_current_file = NULL;

#if	0	/* RXWRXW - source format */
char			*source_name = NULL;
#endif

int			cb_source_format = CB_FORMAT_FIXED;
int			cb_text_column;
int			cb_id = 0;
int			cb_pic_id = 0;
int			cb_attr_id = 0;
int			cb_literal_id = 0;
int			cb_field_id = 0;
int			cobc_flag_main = 0;
int			cb_flag_main = 0;
int			cobc_wants_debug = 0;
int			cb_flag_functions_all = 0;
int			cobc_seen_stdin = 0;
int			cb_unix_lf = 0;

int			errorcount = 0;
int			warningcount = 0;
int			exit_option = 0;
int			warningopt = 0;
int			no_physical_cancel = 0;
int			cb_source_line = 0;
int			cb_saveargc = 0;
unsigned int		cobc_gen_listing = 0;

cob_u32_t		optimize_defs[COB_OPTIM_MAX] = { 0 };

#define	COB_EXCEPTION(code,tag,name,critical) {name, 0x##code, 0},
struct cb_exception cb_exception_table[] = {
	{NULL, 0, 0},		/* CB_EC_ZERO */
#include "libcob/exception.def"
	{NULL, 0, 0}		/* CB_EC_MAX */
};
#undef	COB_EXCEPTION

#define	CB_FLAG(var,pdok,name,doc)	int var = 0;
#define	CB_FLAG_ON(var,pdok,name,doc)	int var = 1;
#define	CB_FLAG_RQ(var,pdok,name,def,opt,doc,vdoc,ddoc)	int var = def;
#define	CB_FLAG_NQ(pdok,name,opt,doc,vdoc)
#include "flag.def"
#undef	CB_FLAG
#undef	CB_FLAG_ON
#undef	CB_FLAG_RQ
#undef	CB_FLAG_NQ

#define	CB_WARNDEF(var,name,doc)	int var = 0;
#define	CB_ONWARNDEF(var,name,doc)	int var = 1;
#define	CB_NOWARNDEF(var,name,doc)	int var = 0;
#include "warning.def"
#undef	CB_WARNDEF
#undef	CB_ONWARNDEF
#undef	CB_NOWARNDEF

/* Local variables */

static struct cb_define_struct	*cb_define_list = NULL;

static struct cobc_mem_struct	*cobc_mainmem_base = NULL;
static struct cobc_mem_struct	*cobc_parsemem_base = NULL;
static struct cobc_mem_struct	*cobc_plexmem_base = NULL;

static const char	*cobc_cc;		/* C compiler */
static char		*cobc_cflags;		/* C compiler flags */
static char		*cobc_libs;		/* -l... */
static char		*cobc_lib_paths;	/* -L... */
static char		*cobc_include;		/* -I... */
static char		*cobc_ldflags;		/* -Q / COB_LDFLAGS */

static size_t		cobc_cflags_size;
static size_t		cobc_libs_size;
static size_t		cobc_lib_paths_size;
static size_t		cobc_include_size;
static size_t		cobc_ldflags_size;

static size_t		cobc_cc_len;
static size_t		cobc_cflags_len;
static size_t		cobc_libs_len;
static size_t		cobc_lib_paths_len;
static size_t		cobc_include_len;
static size_t		cobc_ldflags_len;
static size_t		cobc_export_dyn_len;
static size_t		cobc_shared_opt_len;
static size_t		cobc_pic_flags_len;

static char		*save_temps_dir;
static struct strcache	*base_string;

static char		*cobc_list_dir;
static char		*cobc_list_file;

static char		*output_name;
static char		*cobc_buffer;
static char		*cobc_objects_buffer;
static char		*output_name_buff;
static char		*basename_buffer;

static size_t		cobc_objects_len;
static size_t		basename_len;
static size_t		cobc_buffer_size;

static struct filename	*file_list;

static unsigned int	cb_compile_level = 0;

static int		iargs;

static size_t		wants_nonfinal = 0;
static size_t		cobc_flag_module = 0;
static size_t		cobc_flag_library = 0;
static size_t		cobc_flag_run = 0;
static char		*cobc_run_args;
static size_t		save_temps = 0;
static size_t		save_all_src = 0;
static size_t		save_c_src = 0;
static size_t		verbose_output = 0;
static size_t		cob_optimize = 0;

static int		cb_listing_linecount;
static int		cb_listing_eject = 0;
static char		cb_listing_filename[FILENAME_MAX];
static char		*cb_listing_outputfile = NULL;
static char		cb_listing_title[133];	/* Listing subtitle */

#ifdef	_MSC_VER
static const char	*manicmd;
static const char	*manilink;
static size_t		manilink_len;
#define PATTERN_DELIM '|'
#endif

static size_t		strip_output = 0;
static size_t		gflag_set = 0;
static size_t		aflag_set = 0;

static const char	*const cob_csyns[] = {
#ifndef	COB_EBCDIC_MACHINE
	"NULL",
	"P_cancel",
	"P_initialize",
	"P_ret_initialize",
	"P_switch",
#endif
#ifdef	COB_EBCDIC_MACHINE
	"_float128",
#endif
	"_Bool",
	"_Complex",
	"_Imaginary",
#ifndef	COB_EBCDIC_MACHINE
	"_float128",
#endif
	"alignof",
	"asm",
	"auto",
	"bool",
	"break",
	"case",
	"catch",
	"char",
	"class",
	"const",
	"const_cast",
	"continue",
	"default",
	"delete",
	"do",
	"double",
	"dynamic_cast",
	"else",
	"enum",
	"exit_program",
	"explicit",
	"extern",
	"false",
	"float",
	"for",
	"frame_pointer",
	"frame_stack",
	"friend",
	"goto",
	"if",
	"inline",
	"int",
	"long",
	"mutable",
	"namespace",
	"new",
	"offsetof",
	"operator",
	"private",
	"protected",
	"public",
	"register",
	"reinterpret_cast",
	"restrict",
	"return",
	"short",
	"signed",
	"sizeof",
	"static",
	"static_cast",
	"struct",
	"switch",
	"template",
	"this",
	"throw",
	"true",
	"try",
	"typedef",
	"typeid",
	"typename",
	"typeof",
	"union",
	"unsigned",
	"using",
	"virtual",
	"void",
	"volatile",
#ifndef	COB_EBCDIC_MACHINE
	"wchar_t"
#else
	"wchar_t",
	"NULL",
	"P_cancel",
	"P_initialize",
	"P_ret_initialize",
	"P_switch"
#endif
};

#define COB_NUM_CSYNS	sizeof(cob_csyns) / sizeof(char *)

static const char short_options[] = "hVivqECScbmxjdFROPgwo:t:T:I:L:l:D:K:k:";

#define	CB_NO_ARG	no_argument
#define	CB_RQ_ARG	required_argument
#define	CB_OP_ARG	optional_argument

static const struct option long_options[] = {
	{"help",		CB_NO_ARG, NULL, 'h'},
	{"version",		CB_NO_ARG, NULL, 'V'},
	{"verbose",		CB_NO_ARG, NULL, 'v'},
	{"brief",		CB_NO_ARG, NULL, 'q'},
	{"info",		CB_NO_ARG, NULL, 'i'},
	{"list-reserved",	CB_NO_ARG, NULL, '5'},
	{"list-intrinsics",	CB_NO_ARG, NULL, '6'},
	{"list-mnemonics",	CB_NO_ARG, NULL, '7'},
	{"list-system",		CB_NO_ARG, NULL, '8'},
	{"O2",			CB_NO_ARG, NULL, '9'},
	{"Os",			CB_NO_ARG, NULL, 's'},
	{"save-temps",		CB_OP_ARG, NULL, '_'},
	{"std",			CB_RQ_ARG, NULL, '$'},
	{"conf",		CB_RQ_ARG, NULL, '&'},
	{"debug",		CB_NO_ARG, NULL, 'd'},
	{"ext",			CB_RQ_ARG, NULL, 'e'},
	{"free",		CB_NO_ARG, &cb_source_format, CB_FORMAT_FREE},
	{"fixed",		CB_NO_ARG, &cb_source_format, CB_FORMAT_FIXED},
	{"static",		CB_NO_ARG, &cb_flag_static_call, 1},
	{"dynamic",		CB_NO_ARG, &cb_flag_static_call, 0},
	{"job",			CB_OP_ARG, NULL, 'j'},
	{"j",			CB_OP_ARG, NULL, 'j'},
	{"Q",			CB_RQ_ARG, NULL, 'Q'},
	{"A",			CB_RQ_ARG, NULL, 'A'},
	{"P",			CB_OP_ARG, NULL, 'P'},
	{"Xref",		CB_NO_ARG, NULL, 'X'},
	{"Wall",		CB_NO_ARG, NULL, 'W'},
	{"W",			CB_NO_ARG, NULL, 'Z'},
	{"tlines", 		CB_RQ_ARG, NULL, '#'},
	{"no-symbols", 		CB_NO_ARG, NULL, '@'},

#define	CB_FLAG(var,pdok,name,doc)			\
	{"f"name,		CB_NO_ARG, &var, 1},	\
	{"fno-"name,		CB_NO_ARG, &var, 0},
#define	CB_FLAG_ON(var,pdok,name,doc)		\
	{"f"name,		CB_NO_ARG, &var, 1},	\
	{"fno-"name,		CB_NO_ARG, &var, 0},
#define	CB_FLAG_RQ(var,pdok,name,def,opt,doc,vdoc,ddoc)		\
	{"f"name,		CB_RQ_ARG, NULL, opt},
#define	CB_FLAG_NQ(pdok,name,opt,doc,vdoc)			\
	{"f"name,		CB_RQ_ARG, NULL, opt},
#include "flag.def"
#undef	CB_FLAG
#undef	CB_FLAG_ON
#undef	CB_FLAG_RQ
#undef	CB_FLAG_NQ

#define	CB_CONFIG_ANY(type,var,name,doc)	\
	{"f"name,		CB_RQ_ARG, NULL, '%'},
#define	CB_CONFIG_INT(var,name,min,max,odoc,doc)	\
	{"f"name,		CB_RQ_ARG, NULL, '%'},
#define	CB_CONFIG_STRING(var,name,doc)		\
	{"f"name,		CB_RQ_ARG, NULL, '%'},
#define	CB_CONFIG_BOOLEAN(var,name,doc)		\
	{"f"name,		CB_NO_ARG, &var, 1},	\
	{"fno-"name,		CB_NO_ARG, &var, 0},
#define	CB_CONFIG_SUPPORT(var,name,doc)		\
	{"f"name,		CB_RQ_ARG, NULL, '%'},
#include "config.def"
	{"freserved",	CB_RQ_ARG, NULL, '%'},
	{"fnot-reserved",	CB_RQ_ARG, NULL, '%'},
#undef	CB_CONFIG_ANY
#undef	CB_CONFIG_INT
#undef	CB_CONFIG_STRING
#undef	CB_CONFIG_BOOLEAN
#undef	CB_CONFIG_SUPPORT

#define	CB_WARNDEF(var,name,doc)			\
	{"W"name,		CB_NO_ARG, &var, 1},	\
	{"Wno-"name,		CB_NO_ARG, &var, 0},
#define	CB_ONWARNDEF(var,name,doc)			\
	{"W"name,		CB_NO_ARG, &var, 1},	\
	{"Wno-"name,		CB_NO_ARG, &var, 0},
#define	CB_NOWARNDEF(var,name,doc)			\
	{"W"name,		CB_NO_ARG, &var, 1},	\
	{"Wno-"name,		CB_NO_ARG, &var, 0},
#include "warning.def"
#undef	CB_WARNDEF
#undef	CB_ONWARNDEF
#undef	CB_NOWARNDEF

	{NULL,			0, NULL, 0}
};

#undef	CB_ARG_NO
#undef	CB_ARG_RQ
#undef	CB_ARG_NQ
#undef	CB_ARG_OP

/* Prototype */
DECLNORET static void COB_A_NORETURN	cobc_abort_terminate (void);
static void	print_program_code (struct list_files *, int);
static void	print_program_header (void);
static void	print_program_trailer (void);

/* cobc functions */

static void
cobc_free_mem (void)
{
	struct cobc_mem_struct	*reps;
	struct cobc_mem_struct	*repsl;

	if (save_temps_dir) {
		cobc_free (save_temps_dir);
		save_temps_dir = NULL;
	}
	if (cobc_list_dir) {
		cobc_free (cobc_list_dir);
		cobc_list_dir = NULL;
	}
	if (cobc_list_file) {
		cobc_free (cobc_list_file);
		cobc_list_file = NULL;
	}
	if (cobc_run_args) {
		cobc_free (cobc_run_args);
		cobc_run_args = NULL;
	}
	for (reps = cobc_plexmem_base; reps; ) {
		repsl = reps;
		reps = reps->next;
		cobc_free (repsl);
	}
	cobc_plexmem_base = NULL;
	for (reps = cobc_parsemem_base; reps; ) {
		repsl = reps;
		reps = reps->next;
		cobc_free (repsl);
	}
	cobc_parsemem_base = NULL;
	for (reps = cobc_mainmem_base; reps; ) {
		repsl = reps;
		reps = reps->next;
		cobc_free (repsl);
	}
	cobc_mainmem_base = NULL;
}

static const char *
cobc_enum_explain (const enum cb_tag tag)
{
	switch (tag) {
	case CB_TAG_CONST:
		return "CONSTANT";
	case CB_TAG_INTEGER:
		return "INTEGER";
	case CB_TAG_STRING:
		return "STRING";
	case CB_TAG_ALPHABET_NAME:
		return "ALPHABET";
	case CB_TAG_CLASS_NAME:
		return "CLASS";
	case CB_TAG_LOCALE_NAME:
		return "LOCALE";
	case CB_TAG_SYSTEM_NAME:
		return "SYSTEM";
	case CB_TAG_LITERAL:
		return "LITERAL";
	case CB_TAG_DECIMAL:
		return "DECIMAL";
	case CB_TAG_FIELD:
		return "FIELD";
	case CB_TAG_FILE:
		return "FILE";
	case CB_TAG_REPORT:
		return "REPORT";
	case CB_TAG_REFERENCE:
		return "REFERENCE";
	case CB_TAG_BINARY_OP:
		return "BINARY OP";
	case CB_TAG_FUNCALL:
		return "FUNCTION CALL";
	case CB_TAG_CAST:
		return "CAST";
	case CB_TAG_INTRINSIC:
		return "INTRINSIC";
	case CB_TAG_LABEL:
		return "LABEL";
	case CB_TAG_ASSIGN:
		return "ASSIGN";
	case CB_TAG_INITIALIZE:
		return "INITIALIZE";
	case CB_TAG_SEARCH:
		return "SEARCH";
	case CB_TAG_CALL:
		return "CALL";
	case CB_TAG_GOTO:
		return "GO TO";
	case CB_TAG_IF:
		return "IF";
	case CB_TAG_PERFORM:
		return "PERFORM";
	case CB_TAG_STATEMENT:
		return "STATEMENT";
	case CB_TAG_CONTINUE:
		return "CONTINUE";
	case CB_TAG_CANCEL:
		return "CANCEL";
	case CB_TAG_ALTER:
		return "ALTER";
	case CB_TAG_SET_ATTR:
		return "SET ATTRIBUTE";
	case CB_TAG_PERFORM_VARYING:
		return "PERFORM";
	case CB_TAG_PICTURE:
		return "PICTURE";
	case CB_TAG_LIST:
		return "LIST";
	case CB_TAG_DIRECT:
		return "DIRECT";
	case CB_TAG_DEBUG:
		return "DEBUG";
	case CB_TAG_DEBUG_CALL:
		return "DEBUG CALL";
	default:
		break;
	}
	return "UNKNOWN";
}

/* Global functions */

/* Output a formatted message to stderr */
void
cobc_err_msg (const char *fmt, ...)
{
	va_list		ap;

	fprintf (stderr, "cobc: ");
	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);
	putc ('\n', stderr);
	fflush (stderr);
}

void
cobc_too_many_errors (void)
{
	cobc_err_msg (_("too many errors"));
	cobc_abort_terminate ();
}

/* Output cobc source/line where an internal error occurs and exit */
void
cobc_abort (const char *filename, const int line_num)
{
	cobc_err_msg (_("%s: %d: internal compiler error"), filename, line_num);
	cobc_abort_terminate ();
}

/* Output cobc source/line where a tree cast error occurs and exit */
void
cobc_tree_cast_error (const cb_tree x, const char *filename, const int line_num,
		      const enum cb_tag tagnum)
{
	cobc_err_msg (_("%s: %d: invalid cast from '%s' type %s to type %s"),
		filename, line_num,
		x ? cb_name (x) : "NULL",
		x ? cobc_enum_explain (CB_TREE_TAG(x)) : "None",
		cobc_enum_explain (tagnum));
	cobc_abort_terminate ();
}

#if	!defined(__GNUC__) && defined(COB_TREE_DEBUG)
cb_tree
cobc_tree_cast_check (const cb_tree x, const char *file,
		      const int line, const enum cb_tag tag)
{
	if (!x || x == cb_error_node || CB_TREE_TAG (x) != tag) {
		cobc_tree_cast_error (x, file, line, tag);
	}
	return x;
}
#endif

void *
cobc_malloc (const size_t size)
{
	void	*mptr;

	mptr = calloc ((size_t)1, size);
	if (unlikely(!mptr)) {
		cobc_err_msg (_("cannot allocate %d bytes of memory"),
				(int)size);
		cobc_abort_terminate ();
	}
	return mptr;
}

void
cobc_free(void * mptr)
{
#ifdef	COB_TREE_DEBUG
	if (unlikely(!mptr)) {
		cobc_err_msg (_("call to %s with NULL pointer"), "cobc_free");
		cobc_abort_terminate ();
	}
#endif
	free(mptr);
}

void *
cobc_strdup (const char *dupstr)
{
	void	*p;
	size_t	n;

#ifdef	COB_TREE_DEBUG
	if (unlikely(!dupstr)) {
		cobc_err_msg (_("call to %s with NULL pointer"), "cobc_strdup");
		cobc_abort_terminate ();
	}
#endif
	n = strlen (dupstr);
	p = cobc_malloc (n + 1);
	memcpy (p, dupstr, n);
	return p;
}

void *
cobc_realloc (void *prevptr, const size_t size)
{
	void	*mptr;

	mptr = realloc (prevptr, size);
	if (unlikely(!mptr)) {
		cobc_err_msg (_("cannot reallocate %d bytes of memory"),
				(int)size);
		cobc_abort_terminate ();
	}
	return mptr;
}

/* Memory allocate/strdup/reallocate/free for complete execution */
void *
cobc_main_malloc (const size_t size)
{
	struct cobc_mem_struct	*m;

	m = calloc ((size_t)1, sizeof(struct cobc_mem_struct) + size);
	if (unlikely(!m)) {
		cobc_err_msg (_("cannot allocate %d bytes of memory"),
				(int)size);
		cobc_abort_terminate ();
	}
	m->next = cobc_mainmem_base;
	m->memptr = (char *)m + sizeof(struct cobc_mem_struct);
	m->memlen = size;
	cobc_mainmem_base = m;
	return m->memptr;
}

void *
cobc_main_strdup (const char *dupstr)
{
	void	*p;
	size_t	n;

#ifdef	COB_TREE_DEBUG
	if (unlikely(!dupstr)) {
		cobc_err_msg (_("call to %s with NULL pointer"), "cobc_main_strdup");
		cobc_abort_terminate ();
	}
#endif
	n = strlen (dupstr);
	p = cobc_main_malloc (n + 1);
	memcpy (p, dupstr, n);
	return p;
}

void *
cobc_main_realloc (void *prevptr, const size_t size)
{
	struct cobc_mem_struct	*m;
	struct cobc_mem_struct	*curr;
	struct cobc_mem_struct	*prev;

	m = calloc ((size_t)1, sizeof(struct cobc_mem_struct) + size);
	if (unlikely(!m)) {
		cobc_err_msg (_("cannot allocate %d bytes of memory"),
				(int)size);
		cobc_abort_terminate ();
	}
	m->memptr = (char *)m + sizeof(struct cobc_mem_struct);
	m->memlen = size;

	prev = NULL;
	for (curr = cobc_mainmem_base; curr; curr = curr->next) {
		if (curr->memptr == prevptr) {
			break;
		}
		prev = curr;
	}
	if (unlikely(!curr)) {
		cobc_err_msg (_("attempt to reallocate non-allocated memory"));
		cobc_abort_terminate ();
	}
	m->next = curr->next;
	if (prev) {
		prev->next = m;
	} else {
		/* At mainmem_base */
		cobc_mainmem_base = m;
	}
	memcpy (m->memptr, curr->memptr, curr->memlen);
	cobc_free (curr);

	return m->memptr;
}

void
cobc_main_free (void *prevptr)
{
	struct cobc_mem_struct	*curr;
	struct cobc_mem_struct	*prev;

	prev = NULL;
	for (curr = cobc_mainmem_base; curr; curr = curr->next) {
		if (curr->memptr == prevptr) {
			break;
		}
		prev = curr;
	}
	if (unlikely(!curr)) {
#ifdef	COB_TREE_DEBUG
		cobc_err_msg (_("call to %s with invalid pointer, as it is missing in list"),
			"cobc_main_free");
		cobc_abort_terminate ();
#else
		return;
#endif
	}
	if (prev) {
		prev->next = curr->next;
	} else {
		/* At mainmem_base */
		cobc_mainmem_base = curr->next;
	}
	cobc_free (curr);
}

/* Memory allocate/strdup/reallocate/free for parser */
void *
cobc_parse_malloc (const size_t size)
{
	struct cobc_mem_struct	*m;

	m = calloc ((size_t)1, sizeof(struct cobc_mem_struct) + size);
	if (unlikely(!m)) {
		cobc_err_msg (_("cannot allocate %d bytes of memory"),
				(int)size);
		cobc_abort_terminate ();
	}
	m->next = cobc_parsemem_base;
	m->memptr = (char *)m + sizeof(struct cobc_mem_struct);
	m->memlen = size;
	cobc_parsemem_base = m;
	return m->memptr;
}

void *
cobc_parse_strdup (const char *dupstr)
{
	void	*p;
	size_t	n;

#ifdef	COB_TREE_DEBUG
	if (unlikely(!dupstr)) {
		cobc_err_msg (_("call to %s with NULL pointer"), "cobc_parse_strdup");
		cobc_abort_terminate ();
	}
#endif
	n = strlen (dupstr);
	p = cobc_parse_malloc (n + 1);
	memcpy (p, dupstr, n);
	return p;
}

void *
cobc_parse_realloc (void *prevptr, const size_t size)
{
	struct cobc_mem_struct	*m;
	struct cobc_mem_struct	*curr;
	struct cobc_mem_struct	*prev;

	m = calloc ((size_t)1, sizeof(struct cobc_mem_struct) + size);
	if (unlikely(!m)) {
		cobc_err_msg (_("cannot allocate %d bytes of memory"),
				(int)size);
		cobc_abort_terminate ();
	}
	m->memptr = (char *)m + sizeof(struct cobc_mem_struct);
	m->memlen = size;

	prev = NULL;
	for (curr = cobc_parsemem_base; curr; curr = curr->next) {
		if (curr->memptr == prevptr) {
			break;
		}
		prev = curr;
	}
	if (unlikely(!curr)) {
		cobc_err_msg (_("attempt to reallocate non-allocated memory"));
		cobc_abort_terminate ();
	}
	m->next = curr->next;
	if (prev) {
		prev->next = m;
	} else {
		/* At parsemem_base */
		cobc_parsemem_base = m;
	}
	memcpy (m->memptr, curr->memptr, curr->memlen);
	cobc_free (curr);

	return m->memptr;
}

void
cobc_parse_free (void *prevptr)
{
	struct cobc_mem_struct	*curr;
	struct cobc_mem_struct	*prev;

	prev = NULL;
	for (curr = cobc_parsemem_base; curr; curr = curr->next) {
		if (curr->memptr == prevptr) {
			break;
		}
		prev = curr;
	}
	if (unlikely(!curr)) {
#ifdef	COB_TREE_DEBUG
		cobc_err_msg (_("call to %s with invalid pointer, as it is missing in list"),
			"cobc_parse_free");
		cobc_abort_terminate ();
#else
		return;
#endif
	}
	if (prev) {
		prev->next = curr->next;
	} else {
		/* At parsemem_base */
		cobc_parsemem_base = curr->next;
	}
	cobc_free (curr);
}

/* Memory allocate/strdup/reallocate/free for preprocessor */
void *
cobc_plex_malloc (const size_t size)
{
	struct cobc_mem_struct	*m;

	m = calloc ((size_t)1, sizeof(struct cobc_mem_struct) + size);
	if (unlikely(!m)) {
		cobc_err_msg (_("cannot allocate %d bytes of memory"),
				(int)size);
		cobc_abort_terminate ();
	}
	m->memptr = (char *)m + sizeof(struct cobc_mem_struct);
	m->next = cobc_plexmem_base;
	cobc_plexmem_base = m;
	return m->memptr;
}

void *
cobc_plex_strdup (const char *dupstr)
{
	void	*p;
	size_t	n;

#ifdef	COB_TREE_DEBUG
	if (unlikely(!dupstr)) {
		cobc_err_msg (_("call to %s with NULL pointer"), "cobc_plex_strdup");
		cobc_abort_terminate ();
	}
#endif
	n = strlen (dupstr);
	p = cobc_plex_malloc (n + 1);
	memcpy (p, dupstr, n);
	return p;
}

void *
cobc_check_string (const char *dupstr)
{
	struct strcache	*s;

#ifdef	COB_TREE_DEBUG
	if (unlikely(!dupstr)) {
		cobc_err_msg (_("call to %s with NULL pointer"), "cobc_check_string");
		cobc_abort_terminate ();
	}
#endif
	for (s = base_string; s; s = s->next) {
		if (!strcmp (dupstr, (const char *)s->val)) {
			return s->val;
		}
	}
	s = cobc_main_malloc (sizeof(struct strcache));
	s->next = base_string;
	s->val = cobc_main_strdup (dupstr);
	base_string = s;
	return s->val;
}

static struct cb_text_list *
cb_text_list_add (struct cb_text_list *list, const char *text)
{
	struct cb_text_list	*p;

	p = cobc_main_malloc (sizeof (struct cb_text_list));
	p->text = cobc_main_strdup (text);
	if (!list) {
		p->last = p;
		return p;
	}
	list->last->next = p;
	list->last = p;
	return list;
}

static struct cb_text_list *
cb_text_list_chk (struct cb_text_list *list, const char *text)
{
	struct cb_text_list	*p;

	for (p = list; p; p = p->next) {
		if (!strcmp (text, p->text)) {
			return list;
		}
	}
	return cb_text_list_add (list, text);
}

static unsigned int
cobc_set_value (struct cb_define_struct *p, const char *value)
{
	const char	*s;
	size_t		size;
	unsigned int	dot_seen;
	unsigned int	sign_seen;

	if (!value) {
		p->deftype = PLEX_DEF_NONE;
		p->value = NULL;
		p->sign = 0;
		p->int_part = 0;
		p->dec_part = 0;
		return 0;
	}

	/* Quoted value */
	if (*value == '"' || *value == '\'') {
		size = strlen (value) - 1U;
		if (value[0] != value[size]) {
			p->value = NULL;
			p->deftype = PLEX_DEF_NONE;
			return 1;
		}
		p->value = cobc_main_strdup (value);

		p->deftype = PLEX_DEF_LIT;
		p->sign = 0;
		p->int_part = 0;
		p->dec_part = 0;
		return 0;
	}

	/* Non-quoted value - Check if possible numeric */
	dot_seen = 0;
	sign_seen = 0;
	size = 0;
	s = value;
	if (*s == '+' || *s == '-') {
		sign_seen = 1;
		size++;
		s++;
	}
	for (; *s; ++s) {
		if (*s == '.') {
			if (dot_seen) {
				break;
			}
			dot_seen = 1;
			size++;
			continue;
		}
		if (*s > '9' || *s < '0') {
			break;
		}
		size++;
	}

	if (*s || size <= (dot_seen + sign_seen)) {
		/* Not numeric */
#if	0	/* RXWRXW - Lit warn */
		cb_warning (_("assuming literal for unquoted '%s'"),
				value);
#endif
		size = strlen (value);
		p->value = cobc_main_malloc (size + 4U);
		sprintf (p->value, "'%s'", value);
		p->deftype = PLEX_DEF_LIT;
		p->sign = 0;
		p->int_part = 0;
		p->dec_part = 0;
		return 0;
	}

	p->value = cobc_main_strdup (value);
	p->deftype = PLEX_DEF_NUM;
	p->sign = 0;
	p->int_part = 0;
	p->dec_part = 0;
	return 0;
}

static int
cobc_bcompare (const void *p1, const void *p2)
{
	const void	**tptr;

	tptr = (const void **)p2;
	return strcmp (p1, *tptr);
}

enum name_error_reason {
	INVALID_LENGTH = 1,
	SPACE_UNDERSCORE_FIRST_CHAR,
	GNUCOBOL_PREFIX,
	C_KEYWORD,
	CONTAINS_DIRECTORY_SEPARATOR
};

static void
cobc_error_name (const char *name, const enum cobc_name_type type,
		 const enum name_error_reason reason)
{
	const char	*s;

	switch (reason) {
	case INVALID_LENGTH:
		s = _(" - length is < 1 or > 31");
		break;
	case SPACE_UNDERSCORE_FIRST_CHAR:
		s = _(" - name cannot begin with space or underscore");
		break;
	case GNUCOBOL_PREFIX:
		s = _(" - name cannot begin with 'cob_' or 'COB_'");
		break;
	case C_KEYWORD:
		s = _(" - name duplicates a 'C' keyword");
		break;
	case CONTAINS_DIRECTORY_SEPARATOR:
		s = _(" - name cannot contain a directory separator");
		break;
	default:
		s = "";
		break;
	}
	
	switch (type) {
	case FILE_BASE_NAME:
		cobc_err_msg (_("invalid file base name '%s'%s"),
			      name, s);
		break;
	case ENTRY_NAME:
		cb_error (_("invalid ENTRY '%s'%s"), name, s);
		break;
	case PROGRAM_ID_NAME:
		cb_error (_("invalid PROGRAM-ID '%s'%s"), name, s);
		break;
	default:
		cobc_err_msg (_("unknown name error '%s'%s"),
			      name, s);
		break;
	}
}

size_t
cobc_check_valid_name (const char *name, const enum cobc_name_type prechk)
{
	const char	*p;
	size_t		len;

	/* Check name doesn't contain path separator. */
	for (p = name, len = 0; *p; p++, len++) {
		if (*p == '/' || *p == '\\') {
			cobc_error_name (name, prechk,
					 CONTAINS_DIRECTORY_SEPARATOR);
			return 1;
		}
	}

	/* Check name is of valid length. */
	if (len < 1) {
		cobc_error_name (name, prechk, INVALID_LENGTH);
		return 1;
	}
	if (!cb_relaxed_syntax_checks && len > 31) {
		cobc_error_name (name, prechk, INVALID_LENGTH);
		return 1;
	}
	
	if (*name == '_' || *name == ' ') {
		cobc_error_name (name, prechk, SPACE_UNDERSCORE_FIRST_CHAR);
		return 1;
	}

	/* Check name does not begin with the libcob prefixes cob_ and COB_. */
	if (prechk && len > 3 &&
	    (!memcmp (name, "cob_", (size_t)4) ||
	     !memcmp (name, "COB_", (size_t)4))) {
		cobc_error_name (name, prechk, GNUCOBOL_PREFIX);
		return 1;
	}
	
	/* Check name is not a C keyword. */
	if (bsearch (name, cob_csyns, COB_NUM_CSYNS,
		     sizeof (char *), cobc_bcompare)) {
		cobc_error_name (name, prechk, C_KEYWORD);
		return 1;
	}
	
	return 0;
}

/* Local functions */

static void
cobc_chk_buff_size (const size_t bufflen)
{
	if (bufflen >= cobc_buffer_size) {
		cobc_buffer_size = bufflen + 32;
		cobc_buffer = cobc_main_realloc (cobc_buffer, cobc_buffer_size);
	}
}

static int
cobc_deciph_optarg (const char *p, const int allow_quote)
{
	const unsigned char	*s;
	size_t			len;
	size_t			i;
	int			n;

	len = strlen (p);
	if (!len) {
		return -1;
	}
	s = (const unsigned char *)p;
	if (allow_quote) {
		if (*s == '"' || *s == '\'') {
			if (len != 3 || *(s + 2) != *s) {
				return -1;
			}
			return (int)(*(s + 1));
		}
		if (*s < '0' || *s > '9') {
			if (len != 1) {
				return -1;
			}
			return (int)*s;
		}
	}
	n = 0;
	for (i = 0; i < len; ++i) {
		if (s[i] < '0' || s[i] > '9') {
			return -1;
		}
		n *= 10;
		n += (s[i] & 0x0F);
	}
	return n;
}

DECLNORET static void COB_A_NORETURN
cobc_err_exit (const char *fmt, ...)
{
	va_list		ap;

	fputs ("cobc: ", stderr);
	fputs (_("error: "), stderr);
	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);
	putc ('\n', stderr);
	fflush (stderr);
	cobc_free_mem ();
	exit (1);
}

static struct cb_define_struct *
cb_define_list_add (struct cb_define_struct *list, const char *text)
{
	struct cb_define_struct	*p;
	struct cb_define_struct	*l;
	char			*s;
	char			*x;

	x = cobc_strdup (text);
	s = strtok (x, "=");

	/* Check duplicate */
	for (l = list; l; l = l->next) {
		if (!strcasecmp (s, l->name)) {
			cobc_err_msg (_("duplicate define '%s' - ignored"), s);
			cobc_free (x);
			return list;
		}
	}

	p = cobc_main_malloc (sizeof (struct cb_define_struct));
	p->next = NULL;
	p->name = cobc_check_string (s);
	p->deftype = PLEX_DEF_NONE;
	s = strtok (NULL, "");
	if (cobc_set_value (p, s)) {
		cobc_free (x);
		return NULL;
	}

	cobc_free (x);

	if (!list) {
		p->last = p;
		return p;
	}
	list->last->next = p;
	list->last = p;
	return list;
}

static char *
cobc_stradd_dup (const char *str1, const char *str2)
{
	char	*p;
	size_t	m, n;

#ifdef	COB_TREE_DEBUG
	if (unlikely(!str1 || !str2)) {
		cobc_err_msg (_("call to %s with NULL pointer"), "cobc_stradd_dup");
		cobc_abort_terminate ();
	}
#endif
	m = strlen (str1);
	n = strlen (str2);
	p = cobc_main_malloc (m + n + 1);
	memcpy (p, str1, m);
	memcpy (p + m, str2, n);
	return p;
}

static char *
cobc_getenv (const char *env)
{
	char	*p;

	p = getenv (env);
	if (!p || *p == 0 || *p == ' ') {
		return NULL;
	}
	return cobc_main_strdup (p);
}

/*
 * Like cobc_getenv, except value is not allowed to hold any PATHSEP_CHAR
 */
static char *
cobc_getenv_path (const char *env)
{
	char	*p;

	p = getenv (env);
	if (!p || *p == 0 || *p == ' ') {
		return NULL;
	}
	if (strchr (p, PATHSEP_CHAR) != NULL) {
		cobc_err_msg (_("environment variable '%s' is '%s'; should not contain '%c'"), env, p, PATHSEP_CHAR);
		cobc_abort_terminate ();
	}
	return cobc_main_strdup (p);
}

static void
cobc_add_str (char **var, size_t *cursize, const char *s1, const char *s2,
	      const char *s3)
{
	size_t	calcsize;

	if (!s1) {
		return;
	}

	calcsize = strlen (*var);
	calcsize += strlen (s1);
	if (s2) {
		calcsize += strlen (s2);
	}
	if (s3) {
		calcsize += strlen (s3);
	}
	if (calcsize >= 131072) {
		/* Arbitrary limit */
		cobc_err_exit (_("parameter buffer size exceeded"));
	}
	if (calcsize >= *cursize) {
		while (*cursize <= calcsize) {
			*cursize *= 2;
		}
		*var = cobc_main_realloc (*var, *cursize);
	}
	strcat (*var, s1);
	if (s2) {
		strcat (*var, s2);
	}
	if (s3) {
		strcat (*var, s3);
	}
}

static void
cobc_check_action (const char *name)
{
	if (!name || access (name, F_OK)) {
		return;
	}
	if (!save_temps) {
		(void)unlink (name);
		return;
	}
	if (save_temps_dir) {
		char	temp_buff[COB_MEDIUM_BUFF];

		snprintf (temp_buff, (size_t)COB_MEDIUM_MAX,
			  "%s%s%s", save_temps_dir, SLASH_STR, name);
		temp_buff[COB_MEDIUM_MAX] = 0;
		/* Remove possible target file - ignore return */
		(void)unlink (temp_buff);
		if (rename (name, temp_buff)) {
			cobc_err_msg (_("warning: could not move temporary file to %s"),
					temp_buff);
		}
	}
}

static void
cobc_clean_up (const int status)
{
	struct filename		*fn;
	struct local_filename	*lf;
	cob_u32_t		i;

	if (cb_src_list_file) {
		fclose (cb_src_list_file);
		cb_src_list_file = NULL;
	}
	if (cb_listing_file) {
		fclose (cb_listing_file);
		cb_listing_file = NULL;
	}
	if (cb_storage_file) {
		fclose (cb_storage_file);
		cb_storage_file = NULL;
	}
	if (ppin) {
		fclose (ppin);
		ppin = NULL;
	}

	if (ppout) {
		fclose (ppout);
		ppout = NULL;
	}
	if (yyin) {
		fclose (yyin);
		yyin = NULL;
	}

	if (yyout) {
		fclose (yyout);
		yyout = NULL;
	}

	for (fn = file_list; fn; fn = fn->next) {
		for (lf = fn->localfile; lf; lf = lf->next) {
			if (unlikely(lf->local_fp)) {
				fclose (lf->local_fp);
				lf->local_fp = NULL;
			}
		}
		if (fn->need_assemble &&
		    (status || cb_compile_level > CB_LEVEL_ASSEMBLE ||
		     (cb_compile_level == CB_LEVEL_ASSEMBLE && save_temps))) {
			cobc_check_action (fn->object);
		}
		if (save_all_src) {
			continue;
		}
		if (fn->need_preprocess &&
		    (status || cb_compile_level > CB_LEVEL_PREPROCESS ||
		     (cb_compile_level == CB_LEVEL_PREPROCESS && save_temps))) {
			cobc_check_action (fn->preprocess);
		}
		if (save_c_src) {
			continue;
		}
		if (fn->need_translate &&
		    (status || cb_compile_level > CB_LEVEL_TRANSLATE ||
		     (cb_compile_level == CB_LEVEL_TRANSLATE && save_temps))) {
			cobc_check_action (fn->translate);
			cobc_check_action (fn->trstorage);
			if (fn->localfile) {
				for (lf = fn->localfile; lf; lf = lf->next) {
					cobc_check_action (lf->local_name);
				}
			} else if (fn->translate) {
				/* If we get syntax errors, we do not
				   know the number of local include files */
				snprintf (cobc_buffer, cobc_buffer_size,
					 "%s.l.h", fn->translate);
				cobc_buffer[cobc_buffer_size] = 0;
				for (i = 0; i < 30U; ++i) {
					if (i) {
						snprintf (cobc_buffer, cobc_buffer_size,
							 "%s.l%u.h", fn->translate, i);
						cobc_buffer[cobc_buffer_size] = 0;
					}
					if (!access (cobc_buffer, F_OK)) {
						unlink (cobc_buffer);
					} else if (i) {
						break;
					}
				}
			}
		}
	}
	cobc_free_mem ();
	file_list = NULL;
}

DECLNORET static void COB_A_NORETURN
cobc_terminate (const char *str)
{
	if (cb_src_list_file) {
		cb_listing_linecount = cb_lines_per_page;
		strcpy (cb_listing_filename, str);
		print_program_header();
	}
	cb_perror (0, "cobc: %s: %s", str, cb_get_strerror ());
	if (cb_src_list_file) {
		print_program_trailer();
	}
	cobc_clean_up (1);
	exit (1);
}

static void
cobc_abort_msg (void)
{
	if (cb_source_file) {
		cobc_err_msg (_("aborting compile of %s at line %d"),
			 cb_source_file, cb_source_line);
	} else {
		cobc_err_msg (_("aborting"));
	}
}

DECLNORET static void COB_A_NORETURN
cobc_abort_terminate (void)
{
	cobc_abort_msg ();
	cobc_clean_up (99);
	exit (99);
}

/* TO-DO: Fix */
static void
cobc_sig_handler (int sig)
{
	cobc_abort_msg ();
	save_temps = 0;
	cobc_clean_up (1);
}

/* Command line */

static void
cobc_print_version (void)
{
	printf ("cobc (%s) %s.%d\n",
		PACKAGE_NAME, PACKAGE_VERSION, PATCH_LEVEL);
	puts ("Copyright (C) 2016 Free Software Foundation, Inc.");
	puts (_("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>"));
	puts (_("This is free software; see the source for copying conditions.  There is NO\n"
	        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."));
	printf (_("Written by %s\n"), "Keisuke Nishida, Roger While, Ron Norman, Simon Sobisch, Edward Hart");
	printf (_("Built     %s"), cb_oc_build_stamp);
	putchar ('\n');
	printf (_("Packaged  %s"), COB_TAR_DATE);
	putchar ('\n');
	printf (_("C version %s%s"), OC_C_VERSION_PRF, OC_C_VERSION);
	putchar ('\n');
}

static void
cobc_cmd_print (const char *cmd)
{
	char	*p;
	char	*token;
	size_t	n;
	size_t	toklen;

	fputs (_("executing:"), stderr);
	/* Check if it fits in 80 characters */
	if (strlen (cmd) < 64) {
		fprintf (stderr, "\t%s\n", (char *)cmd);
		fflush (stderr);
		return;
	}
	putc ('\t', stderr);
	p = cobc_strdup (cmd);
	n = 0;
	token = strtok (p, " ");
	for (; token; token = strtok (NULL, " ")) {
		toklen = strlen (token) + 1;
		if ((n + toklen) > 63) {
			fprintf(stderr, "\n\t\t");
			n = 0;
		}
		fprintf (stderr, "%s%s", (n ? " " : ""), token);
		n += toklen;
	}
	cobc_free (p);
	putc ('\n', stderr);
	fflush (stderr);
}

static void
cobc_var_print (const char *msg, const char *val, const unsigned int env)
{
	char	*p;
	char	*token;
	size_t	n;
	int 	lablen;
	size_t	toklen;

	if (!env) {
		printf ("%-*.*s : ", CB_IMSG_SIZE, CB_IMSG_SIZE, msg);
	} else {
		printf ("  %s: ", _("env"));
		lablen = CB_IMSG_SIZE - 2 - (int)strlen (_("env")) - 2;
		printf ("%-*.*s : ", lablen, lablen, msg);
	}
	if (strlen (val) <= CB_IVAL_SIZE) {
		printf("%s\n", val);
		return;
	}
	p = cobc_strdup (val);
	n = 0;
	token = strtok (p, " ");
	for (; token; token = strtok (NULL, " ")) {
		toklen = (int)strlen (token) + 1;
		if ((n + toklen) > CB_IVAL_SIZE) {
			if (n) {
				printf ("\n%*.*s", CB_IMSG_SIZE + 3,
					CB_IMSG_SIZE + 3, " ");
			}
			n = 0;
		}
		printf ("%s%s", (n ? " " : ""), token);
		n += toklen;
	}
	putchar ('\n');
	cobc_free (p);
}

static void
cobc_print_info (void)
{
	char	buff[16];
	char	*s;

	cobc_print_version ();
	putchar ('\n');
	puts (_("build information"));
	cobc_var_print (_("build environment"),	COB_BLD_BUILD, 0);
	cobc_var_print ("CC",			COB_BLD_CC, 0);
	cobc_var_print ("CPPFLAGS",		COB_BLD_CPPFLAGS, 0);
	cobc_var_print ("CFLAGS",		COB_BLD_CFLAGS, 0);
	cobc_var_print ("LD",			COB_BLD_LD, 0);
	cobc_var_print ("LDFLAGS",		COB_BLD_LDFLAGS, 0);
	putchar ('\n');
	puts (_("GnuCOBOL information"));
	cobc_var_print ("COB_CC",		COB_CC, 0);
	if ((s = getenv ("COB_CC")) != NULL) {
		cobc_var_print ("COB_CC",	s, 1);
	}
	cobc_var_print ("COB_CFLAGS",		COB_CFLAGS, 0);
	if ((s = getenv ("COB_CFLAGS")) != NULL) {
		cobc_var_print ("COB_CFLAGS",	s, 1);
	}
	cobc_var_print ("COB_LDFLAGS",		COB_LDFLAGS, 0);
	if ((s = getenv ("COB_LDFLAGS")) != NULL) {
		cobc_var_print ("COB_LDFLAGS",	s, 1);
	}
	cobc_var_print ("COB_LIBS",		COB_LIBS, 0);
	if ((s = getenv ("COB_LIBS")) != NULL) {
		cobc_var_print ("COB_LIBS",	s, 1);
	}
	cobc_var_print ("COB_CONFIG_DIR",	COB_CONFIG_DIR, 0);
	if ((s = getenv ("COB_CONFIG_DIR")) != NULL) {
		cobc_var_print ("COB_CONFIG_DIR",	s, 1);
	}
	cobc_var_print ("COB_COPY_DIR",		COB_COPY_DIR, 0);
	if ((s = getenv ("COB_COPY_DIR")) != NULL) {
		cobc_var_print ("COB_COPY_DIR",	s, 1);
	}
	if ((s = getenv ("COBCPY")) != NULL) {
		cobc_var_print ("COBCPY",	s, 1);
	}
	if (cb_msg_style == CB_MSG_STYLE_MSC) {
		cobc_var_print ("COB_MSG_FORMAT",	"MSC", 0);
	} else {
		cobc_var_print ("COB_MSG_FORMAT",	"GCC", 0);
	}
	if ((s = getenv ("COB_MSG_FORMAT")) != NULL) {
		cobc_var_print ("COB_MSG_FORMAT",	s, 1);
	}
	cobc_var_print ("COB_MODULE_EXT",	COB_MODULE_EXT, 0);
	cobc_var_print ("COB_EXEEXT",		COB_EXEEXT, 0);

#ifdef COB_64_BIT_POINTER
	cobc_var_print ("64bit-mode",	_("yes"), 0);
#else
	cobc_var_print ("64bit-mode",	_("no"), 0);
#endif

#ifdef	COB_LI_IS_LL
	cobc_var_print ("BINARY-C-LONG",	_("8 bytes"), 0);
#else
	cobc_var_print ("BINARY-C-LONG",	_("4 bytes"), 0);
#endif

	cobc_var_print (_("extended screen I/O"),	WITH_CURSES, 0);

	snprintf (buff, sizeof(buff), "%d", WITH_VARSEQ);
	cobc_var_print (_("variable format"),	buff, 0);
	if ((s = getenv ("COB_VARSEQ_FORMAT")) != NULL) {
		cobc_var_print ("COB_VARSEQ_FORMAT", s, 1);
	}

#ifdef	WITH_SEQRA_EXTFH
	cobc_var_print (_("sequential handler"),	"EXTFH", 0);
#else
	cobc_var_print (_("sequential handler"),	_("built-in"), 0);
#endif

#if defined	(WITH_INDEX_EXTFH)
	cobc_var_print (_("ISAM handler"),		"EXTFH", 0);
#elif defined	(WITH_DB)
	cobc_var_print (_("ISAM handler"),		"BDB", 0);
#elif defined	(WITH_CISAM)
	cobc_var_print (_("ISAM handler"),		"C-ISAM", 0);
#elif defined	(WITH_DISAM)
	cobc_var_print (_("ISAM handler"),		"D-ISAM", 0);
#elif defined	(WITH_VBISAM)
	cobc_var_print (_("ISAM handler"),		"VBISAM", 0);
#else
	cobc_var_print (_("ISAM handler"),		_("disabled"), 0);
#endif
}

static void
cobc_print_warn (const char *name, const char *doc, const int warnopt)
{
	switch (warnopt) {
	case 0:
		printf ("  -W%-19s\t%s\n", name, doc);
		fputs ("\t\t\t", stdout);
		fputs (_("- NOT set with -Wall"), stdout);
		putchar ('\n');
		break;
	case 1:
		printf ("  -W%-19s\t%s\n", name, doc);
		break;
	case 2:
		printf ("  -Wno-%-16s\t%s\n", name, doc);
		fputs ("\t\t\t", stdout);	
		fputs (_("- ALWAYS active"), stdout);
		putchar ('\n');
		break;
	default:
		cobc_err_msg (_("call to '%s' with invalid parameter '%s'"),
			"cobc_print_warn", "warnopt");
		COBC_ABORT ();
		break;
	}
}

static void
cobc_print_flag (const char *name, const char *doc,
		 const int pdok, const char *odoc, const char *def)
{
	char		buff[78];

	if (!pdok || !doc) {
		return;
	}
	if (!odoc) {
		snprintf (buff, sizeof (buff) - 1, "-f%s", name);
	} else if (!strcmp(odoc, "no")) {
		snprintf (buff, sizeof (buff) - 1, "-fno-%s", name);
	} else {
		snprintf (buff, sizeof (buff) - 1, "-f%s=%s", name, odoc);
	}
	printf ("  %-21s\t%s\n", buff, doc);
	if (def) {
		printf ("\t\t\t- %s: %s\n", _("default"), def);
	}
}

static void
cobc_print_usage (char * prog)
{
	puts (_("GnuCOBOL compiler for most COBOL dialects with lots of extensions"));
	putchar ('\n');
	printf (_("Usage: %s [options]... file..."), prog);
	putchar ('\n');
	putchar ('\n');
	puts (_("Options:"));
	puts (_("  -h, -help             display this help and exit"));
	puts (_("  -V, -version          display compiler version and exit"));
	puts (_("  -i, -info             display compiler information (build/environment)"));
	puts (_("  -v, -verbose          display the commands invoked by the compiler"));
	puts (_("  -vv                   display compiler version and the commands\n" \
	        "                        invoked by the compiler"));
	puts (_("  -q, -brief            reduced displays, commands invoked not shown"));
	puts (_("  -x                    build an executable program"));
	puts (_("  -m                    build a dynamically loadable module (default)"));
	puts (_("  -j [<args>], -job[=<args>]\trun program after build, passing <args>"));
	puts (_("  -std=<dialect>        warnings/features for a specific dialect\n"
			"                        <dialect> can be one of:\n"
			"                        cobol2014, cobol2002, cobol85, default,\n"
			"                        ibm, mvs, bs2000, mf, acu;\n"
			"                        see configuration files in directory config"));
	puts (_("  -F, -free             use free source format"));
	puts (_("  -fixed                use fixed source format (default)"));
	puts (_("  -O, -O2, -Os          enable optimization"));
	puts (_("  -g                    enable C compiler debug / stack check / trace"));
	puts (_("  -d, -debug            enable all run-time error checking"));
	puts (_("  -o <file>             place the output into <file>"));
	puts (_("  -b                    combine all input files into a single\n"
			"                        dynamically loadable module"));
	puts (_("  -E                    preprocess only; do not compile or link"));
	puts (_("  -C                    translation only; convert COBOL to C"));
	puts (_("  -S                    compile only; output assembly file"));
	puts (_("  -c                    compile and assemble, but do not link"));
	puts (_("  -T <file>             generate and place a wide program listing into <file>"));
	puts (_("  -t <file>             generate and place a program listing into <file>"));
	puts (_("  --tlines=<lines>      specify lines per page in listing, default = 55"));
	puts (_("  --no-symbols          specify no symbols in listing"));
	puts (_("  -P[=<dir or file>]    generate preprocessed program listing (.lst)"));
	puts (_("  -Xref                 generate cross reference through 'cobxref'\n"
			"                        (V. Coen's 'cobxref' must be in path)"));
	puts (_("  -I <directory>        add <directory> to copy/include search path"));
	puts (_("  -L <directory>        add <directory> to library search path"));
	puts (_("  -l <lib>              link the library <lib>"));
	puts (_("  -A <options>          add <options> to the C compile phase"));
	puts (_("  -Q <options>          add <options> to the C link phase"));
	puts (_("  -D <define>           define <define> for COBOL compilation"));
	puts (_("  -K <entry>            generate CALL to <entry> as static"));
	puts (_("  -conf=<file>          user-defined dialect configuration; see -std"));
	puts (_("  -list-reserved        display reserved words"));
	puts (_("  -list-intrinsics      display intrinsic functions"));
	puts (_("  -list-mnemonics       display mnemonic names"));
	puts (_("  -list-system          display system routines"));
	puts (_("  -save-temps[=<dir>]   save intermediate files\n"
			"                        - default: current directory"));
	puts (_("  -ext <extension>      add file extension for resolving COPY"));

	putchar ('\n');

	puts (_("  -W                    enable all warnings"));
	puts (_("  -Wall                 enable most warnings (all except as noted below)"));
	puts (_("  -Wno-<warning>        disable warning enabled by -W or -Wall"));
#define	CB_WARNDEF(var,name,doc)		\
	cobc_print_warn (name, doc, 1);
#define	CB_ONWARNDEF(var,name,doc)		\
	cobc_print_warn (name, doc, 2);
#define	CB_NOWARNDEF(var,name,doc)		\
	cobc_print_warn (name, doc, 0);
#include "warning.def"
#undef	CB_WARNDEF
#undef	CB_ONWARNDEF
#undef	CB_NOWARNDEF

	putchar ('\n');

#define	CB_FLAG(var,pdok,name,doc)		\
	cobc_print_flag (name, doc, pdok, NULL, NULL);
#define	CB_FLAG_ON(var,pdok,name,doc)		\
	cobc_print_flag (name, doc, pdok, "no", NULL);
#define	CB_FLAG_RQ(var,pdok,name,def,opt,doc,vdoc,ddoc)	\
	cobc_print_flag (name, doc, pdok, vdoc, ddoc);
#define	CB_FLAG_NQ(pdok,name,opt,doc,vdoc)		\
	cobc_print_flag (name, doc, pdok, vdoc, NULL);
#include "flag.def"
#undef	CB_FLAG
#undef	CB_FLAG_ON
#undef	CB_FLAG_RQ
#undef	CB_FLAG_NQ

	putchar ('\n');

#define	CB_CONFIG_STRING(var,name,doc)		\
	cobc_print_flag (name, doc, 1, _("<value>"), NULL);
#define	CB_CONFIG_INT(var,name,min,max,odoc,doc)		\
	cobc_print_flag (name, doc, 1, odoc, NULL);
#define	CB_CONFIG_ANY(type,var,name,doc)		\
	cobc_print_flag (name, doc, 1, _("<value>"), NULL);
#define	CB_CONFIG_BOOLEAN(var,name,doc)		\
	cobc_print_flag (name, doc, 1, NULL, NULL);
#define	CB_CONFIG_SUPPORT(var,name,doc)		\
	cobc_print_flag (name, doc, 1, _("<support>"), NULL);
#include "config.def"
#undef	CB_CONFIG_ANY
#undef	CB_CONFIG_INT
#undef	CB_CONFIG_STRING
#undef	CB_CONFIG_BOOLEAN
#undef	CB_CONFIG_SUPPORT
	putchar ('\t');
	puts(_("where <support> is one of the following:"));
	puts("\t'ok', 'warning', 'archaic', 'obsolete', 'skip', 'ignore', 'error', 'unconformable'");
	cobc_print_flag ("not-reserved", _("word to be taken out of the reserved words list"), 1, _("<word>"), NULL);
	cobc_print_flag ("reserved", _("word to be added to reserved words list"), 1, _("<word>"), NULL);
	cobc_print_flag ("reserved", _("word to be added to reserved words list as alias"), 1, _("<word>:<alias>"), NULL);

	putchar ('\n');

	putchar ('\n');
	printf (_("Report bugs to: %s\n" 
			  "or (preferably) use the issue tracker via the home page."), "bug-gnucobol@gnu.org");
	putchar ('\n');
	puts (_("GnuCOBOL home page: <http://www.gnu.org/software/gnucobol/>"));
	puts (_("General help using GNU software: <http://www.gnu.org/gethelp/>"));

}

static void
cobc_options_error_nonfinal (void)
{
	cobc_err_exit (_("only one of options 'E', 'S', 'C', 'c' may be specified"));
}

static void
cobc_options_error_build (void)
{
	cobc_err_exit (_("only one of options 'm', 'x', 'b' may be specified"));
}

static void
cobc_deciph_funcs (const char *opt)
{
	char	*p;
	char	*q;

	if (!strcasecmp (opt, "ALL")) {
		cb_flag_functions_all = 1;
		return;
	}

	p = cobc_strdup (opt);
	q = strtok (p, ",");
	while (q) {
		if (!lookup_intrinsic (q, 1)) {
			cobc_err_exit (_("'%s' is not an intrinsic function"), q);
		}
		CB_TEXT_LIST_ADD (cb_intrinsic_list, q);
		q = strtok (NULL, ",");
	}
	cobc_free (p);
}

static int
process_command_line (const int argc, char **argv)
{
	struct cb_define_struct	*p;
	size_t			osize;
	int			c;
	int			idx;
	int			n;
	int			list_reserved = 0;
#ifdef _WIN32
	int 			argnum;
#endif
	enum cob_exception_id	i;
	struct stat		st;
	char			ext[COB_MINI_BUFF];
	char			*conf_label;	/* we want a dynamic address for erroc.c, not a  static one */
	char			*conf_entry;
	time_t			curtime;		/* Compile time */


	int			conf_ret = 0;

#ifdef _WIN32
	/* Translate command line arguments from WIN to UNIX style */
	argnum = 1;
	while (++argnum <= argc) {
		if (strrchr(argv[argnum - 1], '/') == argv[argnum - 1]) {
			argv[argnum - 1][0] = '-';
		}
	}
#endif

	/* First run of getopt: handle std/conf and all listing options
	   We need to postpone single configuration flags as we need
	   a full configuration to be loaded before */
	cob_optind = 1;
	while ((c = cob_getopt_long_long (argc, argv, short_options,
					  long_options, &idx, 1)) >= 0) {
		switch (c) {

		case '?':
			/* Unknown option or ambiguous */
			cobc_free_mem ();
			exit (1);

		case 'h':
			/* --help */
			cobc_print_usage (argv[0]);
			cobc_free_mem ();
			exit (0);

		case 'V':
			/* --version */
			cobc_print_version ();
			cobc_free_mem ();
			exit (0);

		case 'i':
			/* --info */
			cobc_print_info ();
			exit_option = 1;
			break;

		case '5':
			/* --list-reserved */
			/*
			  This must be postponed until after the configuration
			  is loaded.
			*/
			list_reserved = 1;
			exit_option = 1;
			break;

		case '6':
			/* --list-intrinsics */
			cb_list_intrinsics ();
			exit_option = 1;
			break;

		case '7':
			/* --list-mnemonics */
			cb_list_mnemonics ();
			exit_option = 1;
			break;

		case '8':
			/* --list-system */
			cb_list_system ();
			exit_option = 1;
			break;

		case 'q':
			/* --brief : reduced reporting */
			/* resets -verbose and removes the path to cobc in argv[0] */
			verbose_output = 0;
			strcpy (argv[0], "cobc");	/* set for simple compare in test suite
										   and other static output */
			break;

		case 'v':
			/* --verbose : Verbose reporting */
			/* VERY special case as we set different level by mutliple calls */
			/* Output version information when running very verbose */
			if (++verbose_output == 2) {
				cobc_print_version ();
			}
			break;

		case '$':
			/* -std=<xx> : Specify dialect */
			if (!exit_option) {
				snprintf (ext, (size_t)COB_MINI_MAX, "%s.conf", cob_optarg);
				if (cb_load_std (ext) != 0) {
					cobc_err_exit (_("invalid option -std=%s"),
							   cob_optarg);
				}
			}
			break;

		case '&':
			/* -conf=<xx> : Specify dialect configuration file */
			if (!exit_option) {
				if (strlen (cob_optarg) > COB_SMALL_MAX) {
					cobc_err_exit (COBC_INV_PAR, "-conf");
				}
				conf_ret |= cb_load_conf (cob_optarg, 0);
			}
			break;

		default:
			/* as we postpone most options simply skip everything other here */
			break;
		}
	}

	/* Exit if list options were specified */
	if (exit_option && !list_reserved) {
		cobc_free_mem ();
		exit (0);
	}

	/* Load default configuration file if necessary */
	if (cb_config_name == NULL) {
		if (verbose_output) {
			fputs (_("loading standard configuration file 'default.conf'"), stderr);
			fputc ('\n', stderr);
		}
		conf_ret |= cb_load_std ("default.conf");
	}

	/* Exit for configuration errors resulting from -std/-conf/default.conf */
	if (conf_ret != 0) {
		cobc_free_mem ();
		exit (1);
	}

	cob_optind = 1;
	while ((c = cob_getopt_long_long (argc, argv, short_options,
					  long_options, &idx, 1)) >= 0) {
		switch (c) {
		case 0:
			/* Defined flag */
			break;

		case 'h':
			/* --help */
		case 'V':
			/* --version */
		case 'i':
			/* --info */
		case '5':
			/* --list-reserved */
		case '6':
			/* --list-intrinsics */
		case '7':
			/* --list-mnemonics */
		case '8':
			/* --list-system */
			/* These options were all processed in the first getopt-run */
			break;

		case 'E':
			/* -E : Preprocess */
			if (wants_nonfinal) {
				cobc_options_error_nonfinal ();
			}
			wants_nonfinal = 1;
			cb_compile_level = CB_LEVEL_PREPROCESS;
			break;

		case 'C':
			/* -C : Generate C code */
			if (wants_nonfinal) {
				cobc_options_error_nonfinal ();
			}
			save_c_src = 1;
			wants_nonfinal = 1;
			cb_compile_level = CB_LEVEL_TRANSLATE;
			break;

		case 'S':
			/* -S : Generate assembler code */
			if (wants_nonfinal) {
				cobc_options_error_nonfinal ();
			}
			wants_nonfinal = 1;
			cb_compile_level = CB_LEVEL_COMPILE;
			break;

		case 'c':
			/* -c : Generate C object code */
			if (wants_nonfinal) {
				cobc_options_error_nonfinal ();
			}
			wants_nonfinal = 1;
			cb_compile_level = CB_LEVEL_ASSEMBLE;
			break;

		case 'b':
			/* -b : Generate combined library module */
			if (cobc_flag_main || cobc_flag_module) {
				cobc_options_error_build ();
			}
			cobc_flag_library = 1;
			no_physical_cancel = 1;
			cb_flag_implicit_init = 1;
			break;

		case 'm':
			/* -m : Generate loadable module (default) */
			if (cobc_flag_main || cobc_flag_library) {
				cobc_options_error_build ();
			}
			cobc_flag_module = 1;
			break;

		case 'x':
			/* -x : Generate executable */
			if (cobc_flag_module || cobc_flag_library) {
				cobc_options_error_build ();
			}
			cobc_flag_main = 1;
			cb_flag_main = 1;
			no_physical_cancel = 1;
			break;

		case 'j':
			/* -j : Run job; compile, link and go, either by ./ or cobcrun */
			/* allows optional arguments, passed to program */
			cobc_flag_run = 1;
			if (cob_optarg) {
				cobc_run_args = cobc_strdup (cob_optarg);
			}
			break;

		case 'F':
			/* -F : short option for -free */
			cb_source_format = CB_FORMAT_FREE;
			break;

		case 'q':
			/* --brief : reduced reporting */
		case 'v':
			/* --verbose : Verbose reporting */
			/* These option were processed in the first getopt-run */
			break;

		case 'o':
			/* -o : Output file */
			osize = strlen (cob_optarg);
			if (osize > COB_SMALL_MAX) {
				cobc_err_exit (_("invalid output file name"));
			}
			output_name = cobc_main_strdup (cob_optarg);
			/* Allocate buffer plus extension reserve */
			output_name_buff = cobc_main_malloc (osize + 32U);
			break;

		case 'O':
			/* -O : Optimize */
			cob_optimize = 1;
			COBC_ADD_STR (cobc_cflags, CB_COPT_1, NULL, NULL);
			break;

		case '9':
			/* -O2 : Optimize */
			cob_optimize = 1;
			strip_output = 1;
			COBC_ADD_STR (cobc_cflags, CB_COPT_2, NULL, NULL);
			break;

		case 's':
			/* -Os : Optimize */
			cob_optimize = 1;
			strip_output = 1;
			COBC_ADD_STR (cobc_cflags, CB_COPT_S, NULL, NULL);
			break;

		case 'g':
			/* -g : Generate C debug code */
			save_all_src = 1;
			gflag_set = 1;
			cb_flag_stack_check = 1;
			cb_flag_source_location = 1;
#ifndef	_MSC_VER
			COBC_ADD_STR (cobc_cflags, " -g", NULL, NULL);
#endif
			break;

		case '$':
			/* -std=<xx> : Specify dialect */
		case '&':
			/* -conf=<xx> : Specify dialect configuration file */
			/* These options were all processed in the first getopt-run */
			break;

		case '%':
			/* -f<tag>=<value> : Override configuration entry */
			/* hint: -f[no-]<tag> sets the var directly */
			conf_label = cobc_main_malloc (COB_MINI_BUFF);
			conf_entry = cobc_malloc (COB_MINI_BUFF - 2);
			snprintf (conf_label, COB_MINI_MAX, "-%s=%s",
				long_options[idx].name, cob_optarg);
			strncpy(conf_entry, conf_label + 2, COB_MINI_MAX - 2);
			conf_ret |= cb_config_entry (conf_entry, conf_label, 0);
			cobc_free (conf_entry);
			break;

		case 'd':
			/* -debug : Turn on all runtime checks */
			cb_flag_source_location = 1;
			cb_flag_trace = 1;
			cb_flag_stack_check = 1;
			cobc_wants_debug = 1;
			break;

		case '_':
			/* --save-temps : Save intermediary files */
			save_temps = 1;
			if (cob_optarg) {
				if (stat (cob_optarg, &st) != 0 ||
				    !(S_ISDIR (st.st_mode))) {
					cobc_err_msg (_("warning: '%s' is not a directory, defaulting to current directory"),
						cob_optarg);
				} else {
					save_temps_dir = cobc_strdup (cob_optarg);
				}
			}
			break;

		case 'T':
			/* -T : Generate wide listing */
			cb_listing_wide = 1;
			/* fall through */

		case 't':
			/* -t : Generate listing */
			if (!cb_listing_outputfile) {
				cb_listing_outputfile = cobc_strdup (cob_optarg);
				curtime = time (NULL);
				strcpy (cb_listing_date, ctime(&curtime));
				*strchr (cb_listing_date, '\n') = '\0';
			}
			break;

		case '#':
			/* --tlines=nn : Lines per page */
			cb_lines_per_page = atoi(cob_optarg);
			break;
		case '@':
			/* --no-symbols : No symbols in listing */
			cb_no_symbols = 1;
			break;

		case 'P':
			/* -P : Generate preproc listing */
			if (cob_optarg) {
				if (!stat (cob_optarg, &st) && S_ISDIR (st.st_mode)) {
					cobc_list_dir = cobc_strdup (cob_optarg);
				} else {
					cobc_list_file = cobc_strdup (cob_optarg);
				}
			}
			if (!cobc_gen_listing) {
				cobc_gen_listing = 1;
			}
			break;

		case 'X':
			/* -Xref : Generate listing through 'cobxref' */
			cobc_gen_listing = 2;
			break;

		case 'D':
			/* -D xx(=yy) : Define variables */
			if (strlen (cob_optarg) > 64U) {
				cobc_err_exit (COBC_INV_PAR, "-D");
			}
			if (!strcasecmp (cob_optarg, "ebug")) {
				cobc_err_msg (_("warning: assuming '%s' is a DEFINE - did you intend to use -debug?"),
						cob_optarg);
			}
			p = cb_define_list_add (cb_define_list, cob_optarg);
			if (!p) {
				cobc_err_exit (COBC_INV_PAR, "-D");
			}
			cb_define_list = p;
			break;

		case 'I':
			/* -I <xx> : Include/copy directory */
			if (strlen (cob_optarg) > COB_SMALL_MAX) {
				cobc_err_exit (COBC_INV_PAR, "-I");
			}
			if (stat (cob_optarg, &st) != 0 ||
			    !(S_ISDIR (st.st_mode))) {
				break;
			}
#ifdef	_MSC_VER
			COBC_ADD_STR (cobc_include, " /I \"", cob_optarg, "\"");
#elif	defined (__WATCOMC__)
			COBC_ADD_STR (cobc_include, " -i\"", cob_optarg, "\"");
#else
			COBC_ADD_STR (cobc_include, " -I\"", cob_optarg, "\"");
#endif
			CB_TEXT_LIST_ADD (cb_include_list, cob_optarg);
			break;

		case 'L':
			/* -L <xx> : Directory for library search */
			if (strlen (cob_optarg) > COB_SMALL_MAX) {
				cobc_err_exit (COBC_INV_PAR, "-L");
			}
			if (stat (cob_optarg, &st) != 0 ||
			    !(S_ISDIR (st.st_mode))) {
				break;
			}
#ifdef	_MSC_VER
			COBC_ADD_STR (cobc_lib_paths, " /LIBPATH:\"", cob_optarg, "\"");
#else
			COBC_ADD_STR (cobc_lib_paths, " -L\"", cob_optarg, "\"");
#endif
			break;

		case 'l':
			/* -l <xx> : Add library to link phase */
			if (strlen (cob_optarg) > COB_SMALL_MAX) {
				cobc_err_exit (COBC_INV_PAR, "-l");
			}
#ifdef	_MSC_VER
			COBC_ADD_STR (cobc_libs, " \"", cob_optarg, ".lib\"");
#else
			COBC_ADD_STR (cobc_libs, " -l\"", cob_optarg, "\"");
#endif
			break;

		case 'e':
			/* -e <xx> : Add an extension suffix */
			if (strlen (cob_optarg) > 15U) {
				cobc_err_exit (COBC_INV_PAR, "--ext");
			}
			snprintf (ext, (size_t)COB_MINI_MAX, ".%s", cob_optarg);
			CB_TEXT_LIST_ADD (cb_extension_list, ext);
			break;

		case 'K':
			/* -K <xx> : Define literal CALL to xx as static */
			if (strlen (cob_optarg) > 32U) {
				cobc_err_exit (COBC_INV_PAR, "-K");
			}
			CB_TEXT_LIST_ADD (cb_static_call_list, cob_optarg);
			break;

		case 'k':
			/* -k <xx> : Check for exit after CALL to xx  */
			/* This is to cater for legacy German DIN standard */
			/* Check after CALL if an exit program required */
			/* Not in --help as subject to change and highly specific */
			if (strlen (cob_optarg) > 32U) {
				cobc_err_exit (COBC_INV_PAR, "-k");
			}
			CB_TEXT_LIST_ADD (cb_early_exit_list, cob_optarg);
			break;

		case 1:
			/* -fstack-size=<xx> : Specify stack (perform) size */
			n = cobc_deciph_optarg (cob_optarg, 0);
			if (n < 16 || n > 512) {
				cobc_err_exit (COBC_INV_PAR, "-fstack-size");
			}
			cb_stack_size = n;
			break;

		case 2:
			/* -fif-cutoff=<xx> : Specify IF cutoff level */
			n = cobc_deciph_optarg (cob_optarg, 0);
			if (n < 1 || n > 512) {
				cobc_err_exit (COBC_INV_PAR, "-fif-cutoff");
			}
			cb_if_cutoff = n;
			break;

		case 3:
			/* -fsign=<ASCII/EBCDIC> : Specify display sign */
			if (!strcasecmp (cob_optarg, "EBCDIC")) {
				cb_ebcdic_sign = 1;
			} else if (!strcasecmp (cob_optarg, "ASCII")) {
				cb_ebcdic_sign = 0;
			} else {
				cobc_err_exit (COBC_INV_PAR, "-fsign");
			}
			break;

		case 4:
			/* -ffold-copy=<UPPER/LOWER> : COPY fold case */
			if (!strcasecmp (cob_optarg, "UPPER")) {
				cb_fold_copy = COB_FOLD_UPPER;
			} else if (!strcasecmp (cob_optarg, "LOWER")) {
				cb_fold_copy = COB_FOLD_LOWER;
			} else {
				cobc_err_exit (COBC_INV_PAR, "-ffold-copy");
			}
			break;

		case 5:
			/* -ffold-call=<UPPER/LOWER> : CALL/PROG-ID fold case */
			if (!strcasecmp (cob_optarg, "UPPER")) {
				cb_fold_call = COB_FOLD_UPPER;
			} else if (!strcasecmp (cob_optarg, "LOWER")) {
				cb_fold_call = COB_FOLD_LOWER;
			} else {
				cobc_err_exit (COBC_INV_PAR, "-ffold-call");
			}
			break;

		case 6:
			/* -fdefaultbyte=<xx> : Default initialization byte */
			n = cobc_deciph_optarg (cob_optarg, 1);
			if (n < 0 || n > 255) {
				cobc_err_exit (COBC_INV_PAR, "-fdefaultbyte");
			}
			cb_default_byte = n;
			break;

		case 10:
			/* -fintrinsics=<xx> : Intrinsic name or ALL */
			cobc_deciph_funcs (cob_optarg);
			break;

		case 'A':
			/* -A <xx> : Add options to C compile phase */
			COBC_ADD_STR (cobc_cflags, " ", cob_optarg, NULL);
			aflag_set = 1;
			break;

		case 'Q':
			/* -Q <xx> : Add options to C link phase */
			COBC_ADD_STR (cobc_ldflags, " ", cob_optarg, NULL);
			break;

		case 'w':
			/* -w : Turn off all warnings (disables -W/-Wall if passed later) */
			warningopt = 0;
#define	CB_WARNDEF(var,name,doc)	var = 0;
#define	CB_ONWARNDEF(var,name,doc)	var = 0;
#define	CB_NOWARNDEF(var,name,doc)	var = 0;
#include "warning.def"
#undef	CB_WARNDEF
#undef	CB_ONWARNDEF
#undef	CB_NOWARNDEF
			break;

		case 'W':
			/* -Wall : Turn on most warnings */
			warningopt = 1;
#define	CB_WARNDEF(var,name,doc)	var = 1;
#define	CB_ONWARNDEF(var,name,doc)
#define	CB_NOWARNDEF(var,name,doc)
#include "warning.def"
#undef	CB_WARNDEF
#undef	CB_ONWARNDEF
#undef	CB_NOWARNDEF
			break;

		case 'Z':
			/* -W : Turn on every warning */
			warningopt = 1;
#define	CB_WARNDEF(var,name,doc)	var = 1;
#define	CB_ONWARNDEF(var,name,doc)
#define	CB_NOWARNDEF(var,name,doc)	var = 1;
#include "warning.def"
#undef	CB_WARNDEF
#undef	CB_ONWARNDEF
#undef	CB_NOWARNDEF
			break;

		default:
			cobc_err_exit (_("invalid option detected"));
		}
	}

	/* Exit for configuration errors resulting from -f<conf-tag>[=<value>] */
	if (conf_ret != 0) {
		cobc_free_mem ();
		exit (1);
	}

	/* Set relaxed syntax configuration options if requested */
	/* part 1: relaxed syntax compiler configuration option */ 
	if (cb_relaxed_syntax_checks) {
		if (cb_reference_out_of_declaratives > CB_WARNING) {
			cb_reference_out_of_declaratives = CB_WARNING;
		}
	}
#if 0 /* deactivated as -frelaxed-syntax-checks and other compiler configurations
		 are available at command line - maybe re-add with another name */
	/* 2: relaxed syntax group option from command line */ 
	if (cb_flag_relaxed_syntax_group) {
		cb_relaxed_syntax_checks = 1;
		cb_larger_redefines_ok = 1;
		cb_relax_level_hierarchy = 1;
		cb_top_level_occurs_clause = CB_OK;
	}
#endif

	if (list_reserved) {
		cb_list_reserved ();
	}

	/* Exit if list options were specified */
	if (exit_option) {
		cobc_free_mem ();
		exit (0);
	}

	/* debug: Turn on all exception conditions */
	if (cobc_wants_debug) {
		for (i = (enum cob_exception_id)1; i < COB_EC_MAX; ++i) {
			CB_EXCEPTION_ENABLE (i) = 1;
		}
		if (verbose_output > 1) {
			fputs (_("all runtime checks are enabled"), stderr);
			fputc ('\n', stderr);
		}
	}

	/* If C debug, do not strip output */
	if (gflag_set) {
		strip_output = 0;
	}

	if (cb_flag_traceall) {
		cb_flag_trace = 1;
		cb_flag_source_location = 1;
	}

	return cob_optind;
}

/* Reverse the list of programs */
static struct cb_program *
program_list_reverse (struct cb_program *p)
{
	struct cb_program	*next;
	struct cb_program	*last;

	last = NULL;
	for (; p; p = next) {
		next = p->next_program;
		p->next_program = last;
		last = p;
	}
	return last;
}

static void
process_env_copy_path (const char *p)
{
	char		*value;
	char		*token;
	struct stat	st;

	if (p == NULL || !*p || *p == ' ') {
		return;
	}

	/* Clone value for destructive strtok */
	value = cobc_strdup (p);

	/* Tokenize for path sep. */
	token = strtok (value, PATHSEP_STR);
	while (token) {
		if (!stat (token, &st) && (S_ISDIR (st.st_mode))) {
			CB_TEXT_LIST_CHK (cb_include_list, token);
		}
		token = strtok (NULL, PATHSEP_STR);
	}

	cobc_free (value);
	return;
}

#if	defined (_MSC_VER) || defined(__OS400__) || defined(__WATCOMC__) || defined(__BORLANDC__)
static void
file_stripext (char *buff)
{
	char	*endp;

	endp = buff + strlen (buff) - 1U;
	while (endp > buff) {
		if (*endp == '/' || *endp == '\\') {
			break;
		}
		if (*endp == '.') {
			*endp = 0;
		}
		--endp;
	}
}
#endif

static char *
file_basename (const char *filename)
{
	const char	*p;
	const char	*startp;
	const char	*endp;
	size_t		len;

	if (!filename) {
		cobc_err_msg (_("call to '%s' with invalid parameter '%s'"),
			"file_basename", "filename");
		COBC_ABORT ();
	}

	/* Remove directory name */
	startp = NULL;
	for (p = filename; *p; p++) {
		if (*p == '/' || *p == '\\') {
			startp = p;
		}
	}
	if (startp) {
		startp++;
	} else {
		startp = filename;
	}

	/* Remove extension */
	endp = strrchr (filename, '.');
	if (endp > startp) {
		len = endp - startp;
	} else {
		len = strlen (startp);
	}

	if (len >= basename_len) {
		basename_len = len + 16;
		basename_buffer = cobc_main_realloc (basename_buffer, basename_len);
	}
	/* Copy base name */
	memcpy (basename_buffer, startp, len);
	basename_buffer[len] = 0;
	return basename_buffer;
}

static const char *
file_extension (const char *filename)
{
	const char *p;

	p = strrchr (filename, '.');
	if (p) {
		return p + 1;
	}
	return "";
}

static struct filename *
process_filename (const char *filename)
{
	const char	*extension;
	struct filename	*fn;
	struct filename	*ffn;
	char		*fbasename;
	char		*listptr;
	size_t		fsize;
	int		file_is_stdin;

	if (strcmp(filename, COB_DASH) == 0) {
		if (cobc_seen_stdin == 0) {
			cobc_seen_stdin = 1;
			file_is_stdin = 1;
			filename = COB_DASH_NAME;
		} else {
			cobc_err_msg (_("only one stdin input allowed"));
			return NULL;
		}
	} else {
		file_is_stdin = 0;
	}

	fsize = strlen (filename);
	if (fsize > COB_NORMAL_MAX) {
		cobc_err_msg (_("invalid file name parameter (length > %d)"), COB_NORMAL_MAX);
		return NULL;
	}

#ifdef	__OS400__
	if (strchr (filename, '.') != NULL) {
#endif

	if (!file_is_stdin && access (filename, R_OK) != 0) {
		cobc_terminate (filename);
	}

#ifdef	__OS400__
	}
#endif

	fbasename = file_basename (filename);
	extension = file_extension (filename);
	if (strcmp(extension, "lib") && strcmp(extension, "a") &&
		strcmp(extension, COB_OBJECT_EXT)) {
		if (cobc_check_valid_name (fbasename, 0)) {
			return NULL;
		}
	}
	fn = cobc_main_malloc (sizeof (struct filename));
	fn->need_preprocess = 1;
	fn->need_translate = 1;
	fn->need_assemble = 1;
	fn->file_is_stdin = file_is_stdin;
	fn->next = NULL;

	if (!file_list) {
		file_list = fn;
	} else {
		for (ffn = file_list; ffn->next; ffn = ffn->next)
			;
		ffn->next = fn;
	}

	fn->demangle_source = cb_encode_program_id (fbasename);

	/* Check input file type */
	if (strcmp (extension, "i") == 0) {
		/* Already preprocessed */
		fn->need_preprocess = 0;
	} else if (strcmp (extension, "c") == 0 ||
		   strcmp (extension, "s") == 0) {
		/* Already compiled */
		fn->need_preprocess = 0;
		fn->need_translate = 0;
	} else if (
#if	defined(__OS400__)
			extension[0] == 0
#else
			strcmp (extension, COB_OBJECT_EXT) == 0
#if	defined(_WIN32)
			|| strcmp(extension, "lib") == 0
#endif
#if	!defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
		    || strcmp(extension, "a") == 0
		    || strcmp(extension, "so") == 0
		    || strcmp(extension, "dylib") == 0
			|| strcmp(extension, "sl") == 0
#endif
#endif
	) {
		/* Already assembled */
		fn->need_preprocess = 0;
		fn->need_translate = 0;
		fn->need_assemble = 0;
	}

	/* Set source filename */
	fn->source = cobc_main_strdup (filename);

	/* Set preprocess filename */
	if (!fn->need_preprocess) {
		fn->preprocess = cobc_main_strdup (fn->source);
	} else if (output_name && cb_compile_level == CB_LEVEL_PREPROCESS) {
		fn->preprocess = cobc_main_strdup (output_name);
	} else if (save_all_src || save_temps ||
		   cb_compile_level == CB_LEVEL_PREPROCESS) {
		fn->preprocess = cobc_stradd_dup (fbasename, ".i");
	} else {
		fn->preprocess = cobc_main_malloc(COB_FILE_MAX);
		cob_temp_name ((char *)fn->preprocess, ".cob");
	}

	/* Set translate filename */
	if (!fn->need_translate) {
		fn->translate = cobc_main_strdup (fn->source);
	} else if (output_name && cb_compile_level == CB_LEVEL_TRANSLATE) {
		fn->translate = cobc_main_strdup (output_name);
	} else if (save_all_src || save_temps || save_c_src ||
		   cb_compile_level == CB_LEVEL_TRANSLATE) {
		fn->translate = cobc_stradd_dup (fbasename, ".c");
	} else {
		fn->translate = cobc_main_malloc(COB_FILE_MAX);
		cob_temp_name ((char *)fn->translate, ".c");
	}
	fn->translate_len = strlen (fn->translate);

	/* Set storage filename */
	if (fn->need_translate) {
		fn->trstorage = cobc_stradd_dup (fn->translate, ".h");
	}

	/* Set object filename */
	if (!fn->need_assemble) {
		fn->object = cobc_main_strdup (fn->source);
	} else if (output_name && cb_compile_level == CB_LEVEL_ASSEMBLE) {
		fn->object = cobc_main_strdup (output_name);
	} else if (save_temps || cb_compile_level == CB_LEVEL_ASSEMBLE) {
		fn->object = cobc_stradd_dup(fbasename, "." COB_OBJECT_EXT);
	} else {
		fn->object = cobc_main_malloc(COB_FILE_MAX);
		cob_temp_name ((char *)fn->object, "." COB_OBJECT_EXT);
	}
	fn->object_len = strlen (fn->object);
	cobc_objects_len += fn->object_len + 8U;

	/* Set listing filename */
	if (cobc_gen_listing == 1) {
		if (cobc_list_file) {
			fn->listing_file = cobc_list_file;
		} else if (cobc_list_dir) {
			fsize = strlen (cobc_list_dir) + strlen (fbasename) + 8U;
			listptr = cobc_main_malloc (fsize);
			snprintf (listptr, fsize, "%s%c%s.lst",
				  cobc_list_dir, SLASH_CHAR, fbasename);
			fn->listing_file = listptr;
		} else {
			fn->listing_file = cobc_stradd_dup (fbasename, ".lst");
		}
	} else if (cobc_gen_listing > 1) {
		fn->listing_file = cobc_stradd_dup (fbasename, ".xrf");
	}

	cob_incr_temp_iteration();
	return fn;
}

#ifdef _MSC_VER
/*
 * search_pattern can contain one or more search strings separated by '|'
 * search_patterns must have a final '|'
 */
static int
line_contains (char* line_start, char* line_end, char* search_patterns) {
	int pattern_end, pattern_start, pattern_length, full_length;
	char* line_pos;

	if (search_patterns == NULL) return 0;

	pattern_start = 0;
	full_length = (int)strlen (search_patterns) - 1;
	for (pattern_end = 0; pattern_end < (int)strlen (search_patterns); pattern_end++) {
		if (search_patterns[pattern_end] == PATTERN_DELIM) {
			pattern_length = pattern_end - pattern_start;
			for (line_pos = line_start; line_pos + pattern_length <= line_end; line_pos++) {
				/* Find matching substring */
				if (memcmp (line_pos, search_patterns + pattern_start, pattern_length) == 0) {
					/* Exit if all patterns found, skip to next pattern otherwise */
					if (pattern_start + pattern_length == full_length) {
					return 1;
					} else {
						break;
				}
			}
			}
			pattern_start = pattern_end + 1;
		}
	}

	return 0;
}
#endif

/** -j run job after build */
static int
process_run (const char *name) {
	int ret, status;

	if (cb_compile_level < CB_LEVEL_MODULE) {
		fputs (_("nothing for -j to run"), stderr);
		fflush (stderr);
		return 0;
	}

	if (cb_compile_level == CB_LEVEL_MODULE ||
	    cb_compile_level == CB_LEVEL_LIBRARY) {
		if (cobc_run_args) {
			snprintf (cobc_buffer, cobc_buffer_size, "cobcrun%s %s %s",
				COB_EXEEXT, file_basename(name), cobc_run_args);
		} else {
			snprintf (cobc_buffer, cobc_buffer_size, "cobcrun%s %s",
				COB_EXEEXT, file_basename(name));
		}
	} else {  /* executable */
		if (cobc_run_args) {
			snprintf (cobc_buffer, cobc_buffer_size, ".%c%s%s %s",
				SLASH_CHAR, name, COB_EXEEXT, cobc_run_args);
		} else {
			snprintf (cobc_buffer, cobc_buffer_size, ".%c%s%s",
				SLASH_CHAR, name, COB_EXEEXT);
		}
	}
	cobc_buffer[cobc_buffer_size] = 0;
	if (verbose_output) {
		cobc_cmd_print (cobc_buffer);
	}
	status = system (cobc_buffer);
#ifdef WEXITSTATUS
	if (WIFEXITED(status)) {
		ret = WEXITSTATUS(status);
	} else {
		ret = status;
	}
#else
	ret = status;
#endif
	if (verbose_output) {
		fputs (_("return status:"), stderr);
		fprintf (stderr, "\t%d\n", ret);
		fflush (stderr);
	}
	return ret;
}

#ifdef	__OS400__
static int
process (char *cmd)
{
	char	*buffptr;
	char	*name = NULL;
	char	*objname = NULL;
	char	*cobjname = NULL;
	char	*token;
	char	*incl[100];
	char	*defs[100];
	char	*objs[100];
	char	*libs[100];
	char	*optc[100];
	char	*optl[100];
	int	nincl = 0;
	int	ndefs = 0;
	int	nobjs = 0;
	int	nlibs = 0;
	int	noptc = 0;
	int	noptl = 0;
	int	dbg = 0;
	int	comp_only = 0;
	int	shared = 0;
	int	optimize = 0;
	int	i;
	int	len;
	int	ret;

	if (verbose_output) {
		cobc_cmd_print (cmd);
	}
	if (gflag_set) {
		dbg = 1;
	}
	token = strtok (cmd, " ");
	if (token != NULL) {
		/* Skip C compiler */
		token = strtok (NULL, " ");
	}
	for (; token; token = strtok (NULL, " ")) {
		if (*token != '-') {
			len = strlen (token);
			if (*token == '"') {
				len -= 2;
				++token;
				token[len] = 0;
			}
			if (token[len-2] == '.' && token[len - 1] == 'c') {
				/* C source */
				name = token;
				continue;
			}
			/* Assuming module */
			objs[nobjs++] = token;
			continue;
		}
		++token;
		switch (*token) {
		case 'c':
			comp_only = 1;
			break;
		case 'I':
			++token;
			if (*token == 0) {
				token = strtok (NULL, " ");
			}
			if (*token == '"') {
				++token;
				token[strlen (token) - 1] = 0;
			}
			incl[nincl++] = token;
			break;
		case 'D':
			++token;
			if (*token == 0) {
				token = strtok (NULL, " ");
			}
			if (*token == '"') {
				++token;
				token[strlen (token) - 1] = 0;
			}
			defs[ndefs++] = token;
			break;
		case 'A':
			++token;
			optc[noptc++] = token;
			break;
		case 'Q':
			++token;
			optl[noptl++] = token;
			break;
		case 'o':
			++token;
			if (*token == 0) {
				token = strtok (NULL, " ");
			}
			if (*token == '"') {
				++token;
				token[strlen (token) - 1] = 0;
			}
			objname = token;
			break;
		case 'l':
			++token;
			if (*token == 0) {
				token = strtok (NULL, " ");
			}
			libs[nlibs++] = token;
			break;
		case 'G':
			shared = 1;
			break;
		case 'g':
			dbg = 1;
			break;
		case 'O':
			optimize = 1;
			break;
		default:
			cobc_err_msg (_("unknown option ignored:\t%s"),
				 token - 1);
		}
	}

	buffptr = cobc_malloc (COB_LARGE_BUFF);
	if (name != NULL) {
		/* Requires compilation */
		if (objname == NULL) {
			cobjname = file_basename (name);
		} else {
			cobjname = objname;
		}
		sprintf(buffptr, "CRTCMOD MODULE(%s) SRCSTMF('%s') ",
			cobjname, name);
		if (nincl > 0) {
			strcat (buffptr, "INCDIR(");
			for (i = 0; i < nincl; ++i) {
				if (i != 0) {
					strcat (buffptr, " ");
				}
				strcat (buffptr, "'");
				strcat (buffptr, incl[i]);
				strcat (buffptr, "' ");
			}
			strcat (buffptr, ") ");
		}
		if (ndefs > 0) {
			strcat (buffptr, "DEFINE(");
			for (i = 0; i < ndefs; ++i) {
				if (i != 0) {
					strcat (buffptr, " ");
				}
				strcat (buffptr, "'");
				strcat (buffptr, defs[i]);
				strcat (buffptr, "' ");
			}
			strcat (buffptr, ") ");
		}
		strcat (buffptr, "SYSIFCOPT(*IFSIO)");
		for (i = 0; i < noptc; ++i) {
			strcat (buffptr, " ");
			strcat (buffptr, optc[i]);
		}
		if (optimize) {
			strcat (buffptr, " OPTIMIZE(40)");
		}
		if (dbg) {
			strcat (buffptr, " DBGVIEW(*ALL)");
		}
		if (cobc_gen_listing) {
			strcat (buffptr, " OUTPUT(*PRINT)");
		}
		if (verbose_output) {
			cobc_cmd_print (buffptr);
		}
		ret = system (buffptr);
		if (verbose_output) {
			fputs (_("return status:"), stderr);
			fprintf (stderr, "\t%d\n", ret);
			fflush (stderr);
		}
		if (comp_only || ret != 0) {
			cobc_free (buffptr);
			return ret;
		}
	}
	if (objname == NULL) {
		if (name != NULL) {
			objname = cobjname;
		} else if (nobjs > 0) {
			objname = objs[0];
		} else {
			objname = (char *)"AOUT";
		}
	}
	if (shared) {
		sprintf(buffptr, "CRTSRVPGM SRVPGM(%s) MODULE(", objname);
	} else {
		sprintf(buffptr, "CRTPGM PGM(%s) MODULE(", objname);
	}
	if (name != NULL) {
		strcat (buffptr, cobjname);
	}
	for (i = 0; i < nobjs; ++i) {
		if (i != 0 || name != NULL) {
			strcat (buffptr, " ");
		}
		strcat (buffptr, objs[i]);
	}
	strcat (buffptr, ")");
	if (nlibs > 0) {
		strcat (buffptr, " BNDSRVPGM(");
		for (i = 0; i < nlibs; ++i) {
			if (i != 0) {
				strcat (buffptr, " ");
			}
			strcat (buffptr, libs[i]);
		}
		strcat (buffptr, ")");
	}
	for (i = 0; i < noptl; ++i) {
		strcat (buffptr, " ");
		strcat (buffptr, optl[i]);
	}
	if (shared) {
		strcat (buffptr, " EXPORT(*ALL)");
	}
	if (verbose_output) {
		cobc_cmd_print (buffptr);
	}
	ret = system (buffptr);
	if (verbose_output) {
		fputs (_("return status:"), stderr);
		fprintf (stderr, "\t%d\n", ret);
		fflush (stderr);
	}
	cobc_free (buffptr);

	if ((ret == 0) && cobc_flag_run) {
		ret = process_run (name);
	}
	return ret;
}

#elif defined(_MSC_VER)
static int
process (const char *cmd)
{
	int ret;

	if (verbose_output) {
		cobc_cmd_print (cmd);
	}
	ret = system (cmd);
	if (verbose_output) {
		fputs (_("return status:"), stderr);
		fprintf (stderr, "\t%d\n", ret);
		fflush (stderr);
	}
	return !!ret;
}

static int
process_filtered (const char *cmd, struct filename *fn)
{
	FILE* pipe;
	char* read_buffer;
	char *line_start, *line_end;
	char* search_pattern, *search_pattern2 = NULL;
	char* output_name_temp;
	int i;

	/* Open pipe to catch output of cl.exe */
	pipe = _popen(cmd, "r");

	if (!pipe) {
		return 1; /* checkme */
	}

	/* building search_patterns */
	if (output_name) {
		output_name_temp = file_basename(output_name);
	} else {
		/* demangle_source is encoded and cannot be used
		   -> set to file.something and strip at point
		*/
		output_name_temp = cobc_strdup (fn->source);
		file_stripext(output_name_temp);
	}

	/* check for last path seperator as we only need the file name */
	for (i = fn->translate_len; i > 0; i--) {
		if (fn->translate[i - 1] == '\\' || fn->translate[i - 1] == '/') break;
	}

	search_pattern = (char*)cobc_malloc((fn->translate_len - i + 2) + 1);
	sprintf(search_pattern, "%s\n%c", fn->translate + i, PATTERN_DELIM);
	if (cb_compile_level > CB_LEVEL_ASSEMBLE) {
		search_pattern2 = (char*)cobc_malloc (2 * (strlen (output_name_temp) + 5) + 1);
		sprintf (search_pattern2, "%s.lib%c%s.exp%c", file_basename(output_name_temp), PATTERN_DELIM,
			file_basename(output_name_temp), PATTERN_DELIM);
	}

	/* prepare buffer and read from pipe */
	read_buffer = (char*) cobc_malloc(COB_FILE_BUFF);
	line_start = fgets(read_buffer, COB_FILE_BUFF - 1, pipe);

	while (line_start != NULL) {
		/* read one line from buffer, returning line end position */
		line_end = line_start + strlen (line_start);

		/* if non of the patterns was found, print line */
		if (line_start == line_end
			|| (!line_contains(line_start, line_end, search_pattern)
				&& !line_contains(line_start, line_end, search_pattern2)))
		{
			fprintf(stdout, "%*s", (int)(line_end - line_start + 2), line_start);
		}
		line_start = fgets(read_buffer, COB_FILE_BUFF - 1, pipe);
	}
	fflush (stdout);

	cobc_free (read_buffer);
	cobc_free (search_pattern);
	cobc_free (search_pattern2);

	if (!output_name) cobc_free (output_name_temp);

	/* close pipe and get return code of cl.exe */
	return !!_pclose (pipe);
}

#else
static int
process (const char *cmd)
{
	char	*p;
	char	*buffptr;
	size_t	clen;
	int	ret;

	if (likely(strchr (cmd, '$') == NULL)) {
		buffptr = (char *)cmd;
	} else {
		clen = strlen (cmd) + 64U;
		clen = clen + 6U;
		buffptr = (char *)cobc_malloc (clen);
		p = buffptr;
		/* Quote '$' */
		for (; *cmd; ++cmd) {
			if (*cmd == '$') {
				p += sprintf (p, "\\$");
			} else {
				*p++ = *cmd;
			}
		}
		*p = 0;
	}

	if (verbose_output) {
		cobc_cmd_print (buffptr);
	}

	ret = system (buffptr);

	if (unlikely(buffptr != cmd)) {
		cobc_free (buffptr);
	}

#ifdef	WIFSIGNALED
	if (WIFSIGNALED(ret)) {
#ifdef	SIGINT
		if (WTERMSIG(ret) == SIGINT) {
			cobc_sig_handler (SIGINT);
		}
#endif
#ifdef	SIGQUIT
		if (WTERMSIG(ret) == SIGQUIT) {
			cobc_sig_handler (SIGQUIT);
		}
#endif
	}
#endif
	if (verbose_output) {
		fputs (_("return status:"), stderr);
		fprintf (stderr, "\t%d\n", ret);
		fflush (stderr);
	}
	return !!ret;
}
#endif

/* Preprocess source */

static int
preprocess (struct filename *fn)
{
	struct cobc_mem_struct	*m;
	struct cobc_mem_struct	*ml;
	const char		*sourcename;
	int			save_source_format;
	int			save_fold_copy;
	int			save_fold_call;
	int			ret;

	/* Initialize */
	cb_source_file = NULL;
	cb_source_line = 0;

	if (cb_unix_lf) {
		ppout = fopen(fn->preprocess, "wb");
	} else {
		ppout = fopen(fn->preprocess, "w");
	}
	if (!ppout) {
		cobc_terminate (fn->preprocess);
	}

	if (fn->file_is_stdin) {
		sourcename = COB_DASH;
	} else {
		sourcename = fn->source;
	}
	if (ppopen (sourcename, NULL) != 0) {
		fclose (ppout);
		ppout = NULL;
		if (fn->preprocess) {
			(void)unlink (fn->preprocess);
		}
		cobc_free_mem ();
		exit (1);
	}

	if (verbose_output) {
		fputs (_("preprocessing:"), stderr);
		fprintf (stderr, "\t%s -> %s\n",
			 sourcename, fn->preprocess);
		fflush (stderr);
	}

	if (cobc_gen_listing && !cobc_list_file) {
		if (cb_unix_lf) {
			cb_listing_file = fopen (fn->listing_file, "wb");
		} else {
			cb_listing_file = fopen (fn->listing_file, "w");
		}
		if (!cb_listing_file) {
			cobc_terminate (fn->listing_file);
		}
	}

	/* Reset pplex/ppparse variables */
	plex_clear_vars ();
	ppparse_clear_vars (cb_define_list);

	/* Save default flags in case program directives change them */
	save_source_format = cb_source_format;
	save_fold_copy = cb_fold_copy;
	save_fold_call = cb_fold_call;

	/* Preprocess */
	ppparse ();

	/* Restore default flags */
	cb_source_format = save_source_format;
	cb_fold_copy = save_fold_copy;
	cb_fold_call = save_fold_call;

	if (ppin) {
		fclose (ppin);
		ppin = NULL;
	}

	if (ppout) {
		if (unlikely(fclose(ppout) != 0)) {
			cobc_terminate(fn->preprocess);
		}
		ppout = NULL;
	}

	/* Release flex buffers - After file close */
	plex_call_destroy ();

	for (m = cobc_plexmem_base; m; ) {
		ml = m;
		m = m->next;
		cobc_free (ml);
	}
	cobc_plexmem_base = NULL;

	if (cobc_gen_listing && !cobc_list_file) {
		if (unlikely(fclose (cb_listing_file) != 0)) {
			cobc_terminate(fn->listing_file);
		}
		if (cobc_gen_listing > 1) {
			snprintf (cobc_buffer, cobc_buffer_size,
				 "cobxref %s -R", fn->listing_file);
			cobc_buffer[cobc_buffer_size] = 0;
			if (verbose_output) {
				cobc_cmd_print (cobc_buffer);
			}
			ret = system (cobc_buffer);
			if (verbose_output) {
				fputs (_("return status:"), stderr);
				fprintf (stderr, "\t%d\n", ret);
				fflush (stderr);
			}
			if (ret) {
				fputs (_("'cobxref' execution unsuccessful"),
					stderr);
				putc ('\n', stderr);
#ifdef	_WIN32
				fprintf (stderr, _("check that 'cobxref' is in %s"),
					"%%PATH%%");
#else
				fprintf (stderr, _("check that 'cobxref' is in %s"),
					"$PATH");
#endif
				putc ('\n', stderr);
				fputs (_("no listing produced"),
					stderr);
				putc ('\n', stderr);
				fflush (stderr);
			}
			unlink (fn->listing_file);
		}
		cb_listing_file = NULL;
	}

	if (verbose_output) {
		fputs (_("return status:"), stderr);
		fprintf (stderr, "\t%d\n", errorcount);
		fflush (stderr);
	}
	return !!errorcount;
}

/* Routines to generate program listing */

static void
set_listing_title_code (void)
{
	strcpy (cb_listing_title, "LINE    ");
	if (cb_listing_files->source_format != CB_FORMAT_FREE) {
		strcat (cb_listing_title,
			"PG/LN  A...B..............................."
			".............................");
		if (cb_listing_wide) {
			if (cb_listing_files->source_format == CB_FORMAT_FIXED
			    && cb_text_column == 72) {
				strcat (cb_listing_title, "SEQUENCE");
			} else {
				strcat (cb_listing_title,
					"........................................");
			}
		}
	} else {
		if (cb_listing_wide) {
			strcat (cb_listing_title,
				"................................");
		}
		strcat (cb_listing_title,
			".....................SOURCE..................."
			"..........................");
		if (cb_listing_wide) {
			strcat (cb_listing_title, "........");
		}
	}
}

static void
set_listing_title_symbols (void)
{
	strcpy (cb_listing_title,
		"SIZE TYPE           LVL  NAME                           PICTURE");
}


static void
print_program_header (void)
{
	char		version[20];
	const char	*format_str;

	/* early exit if page break is disabled and not forced */
	if (cb_lines_per_page == 0 && cb_listing_linecount != 0) {
		return;
	}

	if (++cb_listing_linecount >= cb_lines_per_page) {
		sprintf (version, "%s.%d", PACKAGE_VERSION, PATCH_LEVEL);
		if (cb_lines_per_page != 0) {
			cb_listing_linecount = 0;	
		}
		if (cb_listing_eject) {
			fputs ("\f", cb_src_list_file);
		} else {
			cb_listing_eject = 1;
		}

		if (cb_listing_wide) {
			format_str = "%s %-14.14s %-60.60s %s  Page %04d\n\n";
		} else {
			format_str = "%s %-14.14s %-20.20s %s  Page %04d\n\n";
		}
		fprintf (cb_src_list_file,
			 format_str,
			 PACKAGE_NAME,
			 version,
			 cb_listing_filename,
			 cb_listing_date,
			 ++cb_listing_page);

		fprintf (cb_src_list_file, "%s\n\n", cb_listing_title);
	}
}

static char *
check_filler_name (char *name)
{
	if (!memcmp (name, "FILLER", 6)) {
		name = (char *)"FILLER";
	}
	return name;
}

static int
set_picture (struct cb_field *field, char *picture, int picture_len)
{
	int usage_len;
	char picture_usage[CB_LIST_PICSIZE];

	memset (picture, 0, CB_LIST_PICSIZE);

	/* Check non-picture information first */
	switch (field->usage) {
	case CB_USAGE_INDEX:
	case CB_USAGE_LENGTH:
	case CB_USAGE_OBJECT:
	case CB_USAGE_POINTER:
	case CB_USAGE_PROGRAM_POINTER:
	case CB_USAGE_LONG_DOUBLE:
	case CB_USAGE_FP_BIN32:
	case CB_USAGE_FP_BIN64:
	case CB_USAGE_FP_BIN128:
	case CB_USAGE_FP_DEC64:
	case CB_USAGE_FP_DEC128:
	case CB_USAGE_SIGNED_CHAR:
	case CB_USAGE_SIGNED_SHORT:
	case CB_USAGE_SIGNED_INT:
	case CB_USAGE_SIGNED_LONG:
	case CB_USAGE_UNSIGNED_CHAR:
	case CB_USAGE_UNSIGNED_SHORT:
	case CB_USAGE_UNSIGNED_INT:
	case CB_USAGE_UNSIGNED_LONG:
		return 0;
	default:
		break;
	}

	/* check for invalid picture next */
	if (field->pic && !field->pic->orig) {
		strcpy (picture, "INVALID");
		return 1;
	}

	/* Get usage for this picture */
	strcpy (picture_usage, cb_get_usage_string (field->usage));
	usage_len = strlen (picture_usage);

	/* set picture for the rest */
	if (field->usage == CB_USAGE_BINARY
		   || field->usage == CB_USAGE_FLOAT
		   || field->usage == CB_USAGE_DOUBLE
		   || field->usage == CB_USAGE_PACKED
		   || field->usage == CB_USAGE_COMP_5
		   || field->usage == CB_USAGE_COMP_6
		   || field->usage == CB_USAGE_COMP_X) {
		if (field->pic) {
			strncpy (picture, field->pic->orig, picture_len - 1 - usage_len);
			picture[CB_LIST_PICSIZE - 1] = 0;
			strcat (picture, " ");
		}
	} else {
		if (!field->pic) {
			return 0;
		}
		strncpy (picture, field->pic->orig, picture_len - 1);
		return 1;
	}
	
	strcat (picture, picture_usage);
	return 1;
}

static void
set_category_from_usage (int usage, char *type)
{
	switch (usage) {
	case CB_USAGE_INDEX:
		strcpy (type, "INDEX");
		break;
	case CB_USAGE_POINTER:
	case CB_USAGE_PROGRAM_POINTER:
		strcpy (type, "POINTER");
		break;
	case CB_USAGE_DISPLAY:
		strcpy (type, "ALPHANUMERIC");
		break;
	case CB_USAGE_NATIONAL:
		strcpy (type, "NATIONAL");
		break;
	case CB_USAGE_BIT:
		strcpy (type, "BOOLEAN");
		break;
	default:
		strcpy (type, "NUMERIC");
		break;
	}
}

static void
set_category (int category, int usage, char *type)
{
	switch (category) {
	case CB_CATEGORY_UNKNOWN:
		set_category_from_usage (usage, type);
		break;
	case CB_CATEGORY_ALPHABETIC:
		strcpy (type, "ALPHABETIC");
		break;
	case CB_CATEGORY_ALPHANUMERIC:
	case CB_CATEGORY_ALPHANUMERIC_EDITED:
		strcpy (type, "ALPHANUMERIC");
		break;
	case CB_CATEGORY_BOOLEAN:
		strcpy (type, "BOOLEAN");
		break;
	case CB_CATEGORY_INDEX:
		strcpy (type, "INDEX");
		break;
	case CB_CATEGORY_NATIONAL:
	case CB_CATEGORY_NATIONAL_EDITED:
		strcpy (type, "NATIONAL");
		break;
	case CB_CATEGORY_NUMERIC:
	case CB_CATEGORY_NUMERIC_EDITED:
		strcpy (type, "NUMERIC");
		break;
	case CB_CATEGORY_OBJECT_REFERENCE:
		strcpy (type, "OBJECT REF");
		break;
	case CB_CATEGORY_DATA_POINTER:
	case CB_CATEGORY_PROGRAM_POINTER:
		strcpy (type, "POINTER");
		break;
	default:
		strcpy (type, "UNKNOWN");
	}
}

static void
print_fields (int lvl, struct cb_field *top)
{
	int	first = 1;
	int	get_cat;
	int	got_picture;
	int	old_level = 0;
	int	item_size;
	int	picture_len = cb_listing_wide ? 65 : 25;
	char	type[20];
	char	picture[CB_LIST_PICSIZE];
	char	lcl_name[80];

	for (; top; top = top->sister) {
		if (!top->level) {
			continue;
		}

		memset (lcl_name, 0, sizeof(lcl_name));
		memset (lcl_name, ' ', lvl * 2);
		strcat (lcl_name, check_filler_name((char *)top->name));
		get_cat = 1;
		got_picture = 1;

		if (top->level == 01) {
			if (!first) {
				print_program_header ();
				fprintf (cb_src_list_file, "\n");
			}
			if (top->children) {
				strcpy (type, "GROUP");
				get_cat = 0;
				got_picture = 0;
			}
		} else if (top->level == 77 && !first
			   && old_level != 77) {
			print_program_header ();
			fprintf (cb_src_list_file, "\n");
		}

		if (get_cat) {
			set_category (top->common.category, top->usage, type);
			got_picture = set_picture (top, picture, picture_len);
			if (top->redefines)
				strcat (type, "-R");
		}

		print_program_header ();
		item_size = top->size;
		if (got_picture) {
			fprintf (cb_src_list_file,
				"%04d %-14.14s %02d   %-30.30s %s\n",
			 	item_size, type, top->level, lcl_name,
			 	picture);
		} else if (top->depending) {
		   	item_size *= top->occurs_max;
			fprintf (cb_src_list_file,
				"%04d %-14.14s %02d   %-30.30s OCCURS %d TO %d\n",
			 	item_size, type, top->level, lcl_name,
			 	top->occurs_min, top->occurs_max);
		} else if (top->flag_occurs) {
		   	item_size *= top->occurs_max;
			fprintf (cb_src_list_file,
				"%04d %-14.14s %02d   %-30.30s OCCURS %d\n",
			 	item_size, type, top->level, lcl_name,
			 	top->occurs_max);
		} else { /* Trailing spaces break testsuite AT_DATA */
			fprintf (cb_src_list_file,
				"%04d %-14.14s %02d   %s\n",
			 	item_size, type, top->level, lcl_name);
		}
		first = 0;
		old_level = top->level;

		if (top->children) {
			print_fields (lvl + 1, top->children);
		}
	}
}

static void
print_files_and_their_records (cb_tree file_list_p)
{
	cb_tree	l;

	for (l = file_list_p; l; l = CB_CHAIN (l)) {
		print_program_header ();
		fprintf (cb_src_list_file, "%04d %-14.14s      %s\n",
			 CB_FILE (CB_VALUE (l))->record_max,
			 "FILE",
			 CB_FILE (CB_VALUE (l))->name);
		if (CB_FILE (CB_VALUE (l))->record) {
			print_fields (0, CB_FILE (CB_VALUE (l))->record);
		}
		print_program_header ();
		fprintf (cb_src_list_file, "\n");
	}
}

static void
print_fields_in_section (struct cb_field *first_field_in_section)
{
	if (first_field_in_section != NULL) {
		print_fields (0, first_field_in_section);
		print_program_header ();
		fprintf (cb_src_list_file, "\n");
	}
}

static COB_INLINE COB_A_INLINE void
force_new_page_for_next_line (void)
{
	cb_listing_linecount = cb_lines_per_page;
}

static void
print_program_trailer (void)
{
	struct cb_program	*p;
	struct cb_program	*q;
	int			 print_names = 0;


	if (!cb_no_symbols && (p = current_program) != NULL) {
		/* Print file/symbol tables */

		set_listing_title_symbols();
		force_new_page_for_next_line ();
		print_program_header ();

		if (p->next_program) {
			print_names = 1;
		}

		for (q = p; q; q = q->next_program) {
			if (print_names) {
				print_program_header ();
				fprintf (cb_src_list_file,
					"     %-14s      %s\n",
			 	 	(q->prog_type == CB_FUNCTION_TYPE ?
				 		"FUNCTION" : "PROGRAM"),
			 	 	q->program_name);
			}
			print_files_and_their_records (q->file_list);
			print_fields_in_section (q->working_storage);
			print_fields_in_section (q->local_storage);
			print_fields_in_section (q->linkage_storage);
			print_fields_in_section (q->screen_storage);
			print_fields_in_section (q->report_storage);
		}
	} else {
		print_program_header ();
		fputc ('\n', cb_src_list_file);
	}

	/* Print error counts */

	print_program_header ();
	switch (warningcount) {
	case 0:
		fputs ("0 Warnings in program\n", cb_src_list_file);
		break;
	case 1:
		/* FIXME: Change to P_, needs changes to Makevars and tests */
		fputs ("1 Warning in program\n", cb_src_list_file);
		break;
	default:
		fprintf (cb_src_list_file, "%d Warnings in program\n", warningcount);
		break;
	}
	print_program_header ();
	switch (errorcount) {
	case 0:
		fputs ("0 Errors in program\n", cb_src_list_file);
		break;
	case 1:
		/* FIXME: Change to P_, needs changes to Makevars and tests */
		fputs ("1 Error in program\n", cb_src_list_file);
		break;
	default:
		fprintf (cb_src_list_file, "%d Errors in program\n", errorcount);
		break;
	}
	force_new_page_for_next_line ();
}

/*
  Find next token after bp, copy it to token and store the token terminator in
  term. Return pointer to the character after the terminator.
*/
static char *
get_next_token (char *bp, char *token, char *term)
{
	char *token_start = token;

	/* Repeat until a token is found */
	do {
		/* Find first non-space character */
		while (*bp && isspace (*bp)) {
			bp++;
		}

		term[0] = '\0';
		term[1] = '\0';
		if (*bp == '\0') {
			return NULL;
		}

		/* Copy characters into token until a terminator is found. */
		while (*bp) {
			if (*bp == '&') {
				bp++;
				continue;
			}
			if (*bp == '.' && isdigit(*(bp + 1))) {
				;
			} else if (isspace (*bp) || *bp == ',' || *bp == '.' || *bp == ';') {
				term[0] = *bp++;
				break;
			}
			*token++ = *bp++;
		}
		*token = '\0';
	} while (*token_start == '\0' && *term != '\0');

	return bp;
}

static void
terminate_str_at_first_of_char (const char c, char * const str)
{
	char	*first_instance  = strchr (str, c);

	if (first_instance != NULL) {
		*first_instance = '\0';
	}
}

/*
  Copies the next CB_LINE_LENGTH chars from fd into out_line. If fixed is true,
  out_line is padded with spaces to column CB_ENDLINE. The return value is
  either the length of out_line, or -1 if the end of fd is reached.
*/
static int
get_next_listing_line (FILE *fd, char **pline, int fixed)
{
	char	*in_char, *out_line;
	unsigned int	i = 0;
	char	in_line[CB_LINE_LENGTH + 2];

	if (*pline == NULL) {
	   *pline = cobc_malloc (CB_LINE_LENGTH + 2);
	}
	out_line = *pline;

	if (!fgets (in_line, CB_LINE_LENGTH, fd)) {
		memset (out_line, 0, CB_LINE_LENGTH);
		return -1;
	}

	terminate_str_at_first_of_char ('\n', in_line);
	terminate_str_at_first_of_char ('\r', in_line);

	for (in_char = in_line; *in_char; in_char++) {
		if (*in_char == '\t') {
			out_line[i++] = ' ';
			while (i % cb_tab_width != 0) {
				out_line[i++] = ' ';
			}
		} else {
			out_line[i++] = *in_char;
		}
	}

	if (fixed) {
		while (i < (unsigned int)CB_ENDLINE) {
			out_line[i++] = ' ';
		}
	} else {
		out_line[i++] = ' ';
	}
	out_line[i] = 0;

	return i;
}

static int
line_has_page_eject (const char *line, const int fixed)
{
	char	*directive_start;

	if (fixed && line[CB_INDICATOR] == '/') {
		return 1;
	} else {
		directive_start = strchr (line, '>');
		return directive_start != NULL
			&& !strncasecmp (directive_start, ">>PAGE", 6);
	}
}

static int
line_has_listing_directive (const char *line, int *on_off)
{
	char	*directive_start;
	char	token[32], term[2];

	directive_start = strchr (line, '>');
	if (directive_start != NULL &&
		!strncasecmp (directive_start, ">>LISTING", 9)) {
		directive_start += 9;
		*on_off = 1;
		get_next_token (directive_start, token, term);
		if (!strcasecmp (token, "OFF"))
			*on_off = 0;
		return 1;
	}
	return 0;
}

static void
terminate_str_at_first_trailing_space (char * const str)
{
	int	i;

	for (i = strlen (str) - 1; i && isspace (str[i]); i--) {
		str[i] = '\0';
	}
}

static void
print_fixed_line (const int line_num, char pch, char *line)
{
	int		i;
	int		len = strlen (line);
	const int	max_chars_on_line = cb_listing_wide ? 112 : 72;
	const char	*format_str;
#define BUFFER_LEN 133
	char		buffer[BUFFER_LEN];

	if (line[CB_INDICATOR] == '&') {
		line[CB_INDICATOR] = '-';
		pch = '+';
	}

	for (i = 0; len > 0; i += max_chars_on_line, len -= max_chars_on_line) {
		print_program_header ();

		if (cb_listing_wide) {
			format_str = "%06d%c %-112.112s";
		} else {
			format_str = "%06d%c %-72.72s";
		}
		sprintf (buffer, format_str, line_num, pch, line + i);
		
		terminate_str_at_first_trailing_space (buffer);

		fprintf (cb_src_list_file, "%s\n", buffer);
		if (cb_text_column == 72)
			break;
		pch = '+';
	}
}

static void
print_free_line (const int line_num, char pch, char *line)
{
	int		i;
	int		len = strlen (line);
	const int	max_chars_on_line = cb_listing_wide ? 112 : 72;
	const char	*format_str;
	char		buffer[133];

	for (i = 0; len > 0; i += max_chars_on_line, len -= max_chars_on_line) {
		print_program_header ();

		if (cb_listing_wide) {
			format_str = "%06d%c %-112.112s";
		} else {
			format_str = "%06d%c %-72.72s";
		}
		sprintf (buffer, format_str, line_num, pch, line + i);

		terminate_str_at_first_trailing_space (buffer);

		fprintf (cb_src_list_file, "%s\n", buffer);
		pch = '+';
	}
}

static void
print_errors_for_line (const struct list_error * const first_error,
		       const int line_num)
{
	const struct list_error	*err;

	for (err = first_error; err; err = err->next) {
		if (err->line == line_num) {
			print_program_header ();
			fprintf (cb_src_list_file, "%s%s\n", err->prefix, err->msg);
		}
	}
}

static void
print_line (struct list_files *cfile, char *line, int line_num, int in_copy)
{
	struct list_skip	*skip;
	int	do_print;
	int	on_off;
	char	pch;

	if (line_has_page_eject (line, cfile->source_format != CB_FORMAT_FREE)) {
		force_new_page_for_next_line ();
	}

	do_print = cfile->listing_on;
	if (line_has_listing_directive (line, &on_off)) {
		cfile->listing_on = on_off;
		/* always print the directive itself */
		do_print = 1;
	}

	if (do_print) {
		pch = in_copy ? 'C' : ' ';
		for (skip = cfile->skip_head; skip; skip = skip->next) {
			if (skip->skipline == line_num) {
				pch = 'X';
				break;
			}
		}

		terminate_str_at_first_trailing_space (line);
		if (cfile->source_format == CB_FORMAT_FIXED) {
			print_fixed_line (line_num, pch, line);
		} else { /* CB_FORMAT_FREE */
			print_free_line (line_num, pch, line);
		}
	}

	/* Print errors regardless of LISTING setting */
	if (cfile->err_head) {
		print_errors_for_line (cfile->err_head, line_num);
	}
}

/*
  Copy each token in pline from the start of pline[first_idx] to the end of
  pline[last_idx] into cmp_line, separated by a space. Tokens are copied from
  the first_col of each line and up to the end of line or the sequence area (if
  fixed is true).
  Return the column to which pline[last_idx] was read up to.
*/
static int
compare_prepare (char *cmp_line, char *pline[CB_READ_AHEAD],
		 int first_idx, int last_idx, int first_col, int fixed)
{
	int	i;
	int	out_pos = 0;
	int	line_idx;
	int	in_string = 0;
	int	last_col = CB_SEQUENCE;
	int	last_nonspace;

	cmp_line[0] = 0;

	/* Collapse pline into a string of tokens separated by spaces */
	for (line_idx = first_idx; line_idx < last_idx; line_idx++) {
		if (!fixed) {
			last_col = strlen (pline[line_idx]);
		}

		/* Go the last non-space character */
		for (last_nonspace = last_col;
		     isspace (pline[line_idx][last_nonspace - 1]) && last_nonspace > first_col;
		     last_nonspace--);
		/* Go to first non-space character */
		for (i = first_col; (i < last_nonspace) && isspace (pline[line_idx][i]); i++);

		/* Copy chars between the first and last non-space characters */
		while (i < last_nonspace) {
			if (isspace (pline[line_idx][i])) {
				cmp_line[out_pos++] = ' ';
				for (i++; (i < last_nonspace) && isspace (pline[line_idx][i]); i++);
				if (i == last_nonspace) {
					break;
				}
			} else if (pline[line_idx][i] == '"') {
				/*
				  Merge multi-part strings into one string,
				  reading another line if necessary to find the
				  end.
				*/
				if (in_string) {
					i++;
				} else {
					cmp_line[out_pos++] = pline[line_idx][i++];
					in_string = 1;
				}

				for (; (i < last_nonspace) && (pline[line_idx][i] != '"'); ) {
					cmp_line[out_pos++] = pline[line_idx][i++];
				}
				if (pline[line_idx][i] == '"') {
					cmp_line[out_pos++] = pline[line_idx][i++];
					in_string = 0;
				}
				if (i == last_nonspace) {
					break;
				}
			} else {
				cmp_line[out_pos++] = pline[line_idx][i++];
			}
		}
	}
	cmp_line[out_pos] = 0;
#ifdef DEBUG_REPLACE
	fprintf (stdout, "   last_col = %d\n   cmp_line: %s\n", last_col, cmp_line);
#endif
	return last_col;
}

/*
  Add adjust to each line number less than line_num (if appropriate) in cfile's
  copy, replace and error lists.
*/
static void
adjust_line_numbers (struct list_files *cfile, int line_num, int adjust)
{
	struct list_files	*cur;
	struct list_replace	*rep;
	struct list_error	*err;

	for (cur = cfile->copy_head; cur; cur = cur->next) {
		cur->copy_line += adjust;
	}

	for (rep = cfile->replace_head; rep; rep = rep->next) {
		if (rep->firstline > line_num) {
			rep->firstline += adjust;
		}
	}

	for (err = cfile->err_head; err; err = err->next) {
		err->line += adjust;
	}
}

static COB_INLINE COB_A_INLINE int
is_debug_line (char *line, int fixed)
{
	if (line == NULL || line[0] == 0) {
		return 0;
	}
	return !cb_flag_debugging_line
		&& ((fixed && IS_DEBUG_LINE (line))
		    || (!fixed && !strncasecmp (line, "D ", 2)));
}

static COB_INLINE COB_A_INLINE int
is_comment_line (char *line, int fixed)
{
	if (line == NULL || line[0] == 0) {
		return 0;
	}
	return (fixed && IS_COMMENT_LINE (line))
		|| (!fixed && !strncmp (line, "*>", 2));
}

static int
is_continuation_line (char *line, int fixed)
{
	int i;

	if (line == NULL || line[0] == 0) {
		return 0;
	}
	if (fixed) {
		/* check for "-" in column 7 */
		if (IS_CONTINUE_LINE(line)) {
			return 1;
		}
	} else {
		/* check for "&" as last character */
		/* CHECKME: does this work with inline comments after "&"? */
		i = strlen (line) - 1;
		while (i && isspace (line[i])) i--;
		if (line[i] == '&') {
			return 1;
		}
	}

	return 0;
}

static void
pline_check_limit (int pline_cnt, char *filename, int line_num)
{
	if (pline_cnt >= CB_READ_AHEAD) {
		cobc_err_msg (_("%s: %d: Too many continuation lines"),
				filename, line_num);
		cobc_abort_terminate ();
	}
}

/* TODO: Modularise! */
static int
print_replace_text (struct list_files *cfile, FILE *fd,
		    struct list_replace *rep, char *pline[CB_READ_AHEAD],
		    int pline_cnt, int line_num)
{
	char	*rfp = rep->from;
	char	*fp;
	char	*tp;
	int	i;
	int	j;
	int	k = 0;
	const int	fixed = (cfile->source_format == CB_FORMAT_FIXED);
	int	first_col = fixed ? CB_MARGIN_A : 0;
	int	last;
	int	multi_token;
	int	match = 0;
	int	eof = 0;
	int	submatch = 0;
	int	seccount = 0;
	int	overread = 0;
	int	tokmatch = 0;
	int	subword = 0;
	int	ttix, ttlen, ftlen;
	int	nextrec;
	char	lterm[2];
	char	fterm[2];
	char	ftoken[CB_LINE_LENGTH + 2];
	char	tterm[2];
	char	ttoken[CB_LINE_LENGTH + 2];
	char	cmp_line[CB_LINE_LENGTH + 2];
	char	newline[CB_LINE_LENGTH + 2];
	char	frm_line[CB_LINE_LENGTH + 2];

	if (is_comment_line (pline[0], fixed)) {
		return pline_cnt;
	}

	terminate_str_at_first_trailing_space (rfp);
	while (*rfp && isspace (*rfp)) {
		rfp++;
	}
	multi_token = (strchr (rfp, ' ') != NULL);

#ifdef DEBUG_REPLACE
	fprintf (stdout, "print_replace_text: line_num = %d", line_num);
	fprintf (stdout, ", multi_token = %s, fixed = %s\n",
		 multi_token ? "TRUE" : "FALSE", fixed ? "TRUE" : "FALSE");
	fprintf (stdout, "   pline_cnt = %d\n", pline_cnt);
	for (i = 0; i < pline_cnt; i++) {
		fprintf (stdout, "   pline[%2d]: %s\n", i, pline[i]);
	}
	fprintf (stdout, "   rep: first = %d, last = %d, lead_trail = %d\n",
		 rep->firstline, rep->lastline, rep->lead_trail);
	fprintf (stdout, "   from: '%s'\n", rfp);
	fprintf (stdout, "   to:   '%s'\n", rep->to);
#endif

	last = compare_prepare (cmp_line, pline, 0, pline_cnt, first_col, fixed);

	newline[0] = 0;
	if (multi_token) {
		/*
		  Attempt to match the source text from the beginning of each
		  line (continuing the match to the next line if need be). If a
		  match is found, output the line to newline with the match
		  replaced.
		*/

		strcpy (frm_line, rfp);
		fp = get_next_token (frm_line, ftoken, fterm);
	next_line:
		tp = get_next_token (cmp_line, ttoken, tterm);

		/*
		  Read tokens until the match is complete or until a match
		  fails.
		*/
		while (tp && fp) {
			if (!strcasecmp (ttoken, ftoken)) {
				/*
				  Mark two tokens as matched, then read next
				  pair.
				*/
				submatch = 1;
				if (fterm[0] == tterm[0]) {
					lterm[0] = 0;
				} else {
					lterm[0] = tterm[0];
				}
				lterm[1] = tterm[1];
				tp = get_next_token (tp, ttoken, tterm);
				fp = get_next_token (fp, ftoken, fterm);
			} else {
				/* Discard partial match. */
				if (seccount == 0) {
					strcat (newline, ttoken);
					strcat (newline, tterm);
				}
				submatch = 0;

				/* Start matching from beginning of from_line again. */
				strcpy (frm_line, rfp);
				fp = get_next_token (frm_line, ftoken, fterm);
				tp = get_next_token (tp, ttoken, tterm);
				break;
			}
		}
		if (!fp && submatch) {
			/*
			  If the match is complete, output the match's
			  replacement.
			*/
			match = 1;
			strcat (newline, rep->to);
			strcat (newline, lterm);
			if (tp) {
				strcat (newline, ttoken);
				strcat (newline, tterm);
				strcat (newline, tp);
			}
		}
		else if (!tp && submatch) {
			/*
			  If we run out of chars from the original source, get
			  more.
			*/

#ifdef DEBUG_REPLACE
			fprintf (stdout, "   submatch = TRUE\n");
#endif
			if (eof) {
				return pline_cnt;
			}

			/*
			  Overwrite the current line if it is a comment or debug
			  line.
			*/
			if (is_comment_line (pline[pline_cnt], fixed)) {
				adjust_line_numbers (cfile, line_num,  -1);
				overread = 1;
			}
			if (is_debug_line (pline[pline_cnt], fixed)) {
				adjust_line_numbers (cfile, line_num,  -1);
				overread = 1;
			}

			/*
			  Read lines until we find a non-comment, non-debug
			  line.
			 */
		next_rec:
			if (!is_comment_line (pline[pline_cnt], fixed)) {
				pline_cnt++;
			}
			pline_check_limit (pline_cnt, cfile->name, line_num);
			if (get_next_listing_line (fd, &pline[pline_cnt], fixed) < 0) {
				pline[pline_cnt][0] = 0;
				eof = 1;
			}
			if (is_debug_line (pline[pline_cnt], fixed)
			    || is_comment_line (pline[pline_cnt], fixed)) {
				adjust_line_numbers (cfile, line_num,  -1);
				goto next_rec;
			}
#ifdef DEBUG_REPLACE
			fprintf (stdout, "   pline[%2d]: %s\n", pline_cnt - 1,
				 pline[pline_cnt - 1]);
#endif
			line_num++;
			seccount++;
			if (overread) {
				overread = 0;
				goto next_rec;
			}
			last = compare_prepare (cmp_line, pline, pline_cnt - 1, pline_cnt,
						first_col, fixed);
			strcat (newline, " ");
			goto next_line;
		}
	} else {
		strcpy (frm_line, rfp);
		fp = get_next_token (frm_line, ftoken, fterm);
		if (ftoken[0] == ':' || ftoken[0] == '(') {
			subword = 1;
		}
		ftlen = strlen (ftoken);

		/*
		  For each token in cmp_line, try to match it with the token in
		  from_line.
		 */
		for (tp = get_next_token (cmp_line, ttoken, tterm); tp;
		     tp = get_next_token (tp, ttoken, tterm)) {
#ifdef DEBUG_REPLACE
			fprintf (stdout, "   tterm = '%s', ttoken = '%s', ftoken = '%s'\n",
				 tterm, ttoken, ftoken);
#endif
			ttlen = strlen (ttoken);
			ttix = 0;
			if (rep->lead_trail == CB_REPLACE_LEADING) {
				subword = 1;
			} else if (rep->lead_trail == CB_REPLACE_TRAILING) {
				if (ttlen >= ftlen) {
					subword = 1;
					ttix = ttlen - ftlen;
					ttlen = ttix;
				}
			}
			if (subword) {
				tokmatch = !strncasecmp (&ttoken[ttix], ftoken, ftlen);
			} else {
				tokmatch = !strcasecmp (ttoken, ftoken);
			}
			if (tokmatch) {
				if (subword) {
					if (rep->lead_trail == CB_REPLACE_LEADING) {
						strcat (newline, rep->to);
						strcat (newline, &ttoken[ftlen]);
					} else if (rep->lead_trail == CB_REPLACE_TRAILING) {
						strncat (newline, ttoken, ttlen);
						strcat (newline, rep->to);
					} else {
						strcat (newline, rep->to);
					}
				} else {
					strcat (newline, rep->to);
				}
				strcat (newline, tterm);
				match = 1;
			} else {
				strcat (newline, ttoken);
				strcat (newline, tterm);
			}
		}
	}

	if (match) {
#ifdef DEBUG_REPLACE
		fprintf (stdout, "   match = TRUE\n   newline = %s\n", newline);
#endif
		if (fixed) {
			fp = get_next_token (newline, ftoken, fterm);
			for (j = first_col; (j < last) && isspace (pline[k][j]); j++);
			if (j >= CB_MARGIN_B) {
				first_col = CB_MARGIN_B;
			}
			for (k = 0; k < pline_cnt; k++) {
				nextrec = 0;
				j = first_col;
				while (fp && !nextrec) {
					ftlen = strlen (ftoken);
					i = 0;
					if (ftlen >= (CB_SEQUENCE - first_col)) {
#ifdef DEBUG_REPLACE
						fprintf (stdout, "   ftlen = %d\n", ftlen);
#endif
						while (ftlen) {
							pline[k][j++] = ftoken[i++];
							ftlen--;
							if (j == CB_SEQUENCE) {
#ifdef DEBUG_REPLACE
								fprintf (stdout, "   NEW pline[%2d] = %s\n",
									 k, pline[k]);
#endif
								j = first_col;
								k++;
								if (k == pline_cnt) {
									pline_check_limit (pline_cnt + 1, cfile->name, line_num);
									if (pline[pline_cnt + 1] == NULL) {
	   									pline[pline_cnt + 1] = cobc_malloc (CB_LINE_LENGTH + 2);
									}
									strcpy (pline[pline_cnt + 1], pline[pline_cnt]);
									strcpy (pline[pline_cnt], pline[pline_cnt - 1]);
									memset (&pline[pline_cnt][CB_MARGIN_A], ' ',
										CB_SEQUENCE - CB_MARGIN_A);
									pline[pline_cnt][CB_INDICATOR] = '&';
									pline_cnt++;
								}
							}
						}
						pline[k][j++] = ' ';
					} else {
						if ((j + 2 + ftlen) < last) {
							for (i = 0; i < (int)strlen (ftoken); i++)
								pline[k][j++] = ftoken[i];
							pline[k][j++] = fterm[0] ? fterm[0] : ' ';
							if (fterm[0] == '.')
								pline[k][j++] = ' ';
						} else {
							nextrec = 1;
						}
					}
					fp = get_next_token (fp, ftoken, fterm);
				}
				if (j == first_col) {
					pline[k][CB_INDICATOR] = ' ';
				}
				while (j < last) {
					pline[k][j++] = ' ';
				}
#ifdef DEBUG_REPLACE
				fprintf (stdout, "   NEW pline[%2d] = %s\n", k, pline[k]);
#endif
			}
		} else {
			fp = get_next_token (newline, ftoken, fterm);
			for (k = 0; k < pline_cnt; k++) {
				for (j = first_col; pline[k][j] && pline[k][j] == ' '; j++) ;
				pline[k][j] = 0;
				while (fp) {
					strcat (pline[k], ftoken);
					strcat (pline[k], fterm);
					j++;
					fp = get_next_token (fp, ftoken, fterm);
				}
				if (j == first_col) {
					strcat (pline[k], " ");
				}
			}
		}
	}

	return pline_cnt;
}

static void
free_replace_list (struct list_files *cfile, const int line_num)
{
	struct list_replace	*rep;

	while (cfile->replace_head
	       && cfile->replace_head->firstline < line_num) {
		rep = cfile->replace_head;
		cfile->replace_head = rep->next;
		cobc_free (rep);
	}
}

static void
copy_list_replace (struct list_replace *src, struct list_files *dst_file)
{
	struct list_replace	*copy;

	copy = cobc_malloc (sizeof (struct list_replace));
	memcpy (copy, src, sizeof (struct list_replace));
	copy->next = NULL;

	if (dst_file->replace_tail) {
		dst_file->replace_tail->next = copy;
	}
	if (!dst_file->replace_head) {
		dst_file->replace_head = copy;
	}
	dst_file->replace_tail = copy;
}

/* TODO: Modularise! */
static int
print_replace_main (struct list_files *cfile, FILE *fd,
		    char *pline[CB_READ_AHEAD], int pline_cnt, int line_num)
{
	static int	in_replace = 0;
	char	*tp;
	struct list_replace	*rep;
	int	i;
	const int	fixed = (cfile->source_format == CB_FORMAT_FIXED);
	const int	first_col = fixed ? CB_MARGIN_A : 0;
	int	is_copy;
	int	is_replace;
	int	is_off;
	char	tterm[2] = { '\0' };
	char	ttoken[CB_LINE_LENGTH + 2] = { '\0' };
	char	cmp_line[CB_LINE_LENGTH + 2] = { '\0' };

	if (is_comment_line (pline[0], cfile->source_format != CB_FORMAT_FREE)) {
		return pline_cnt;
	}

#ifdef DEBUG_REPLACE
	fprintf (stdout, "print_replace_main: line_num = %d\n", line_num);
	fprintf (stdout, "   pline_cnt = %d\n", pline_cnt);
	for (i = 0; i < pline_cnt; i++) {
		fprintf (stdout, "   pline[%2d]: %s\n", i, pline[i]);
	}
#endif

	compare_prepare (cmp_line, pline, 0, pline_cnt, first_col,
			 cfile->source_format);
	tp = get_next_token (cmp_line, ttoken, tterm);
	is_replace = !strcasecmp (ttoken, "REPLACE");
	is_copy = !strcasecmp (ttoken, "COPY");
	is_off = 0;
	if (is_replace) {
		tp = get_next_token (tp, ttoken, tterm);
		is_off = !strcasecmp (ttoken, "OFF");
	}

	if (!in_replace && is_replace) {
		if (!is_off) {
			in_replace = 1;
#ifdef DEBUG_REPLACE
			for (i = 0, rep = cfile->replace_head; rep; i++, rep = rep->next) {
				if (rep->firstline < (line_num + 10)) {
					if (i == 0)
						fprintf (stdout, "   replace_list: \n");
					fprintf (stdout, "      line[%d]: %d\n", i, rep->firstline);
					fprintf (stdout, "      from[%d]: '%s'\n", i, rep->from);
					fprintf (stdout, "      to  [%d]: '%s'\n", i, rep->to);
				}
			}
#endif
		}
	} else if (in_replace) {
		if (is_replace && is_off) {
			in_replace = 0;
			free_replace_list (cfile, line_num);
		} else {
			if (is_copy) {
				if (cfile->copy_head) {
					struct list_files *cur;

					/* List all lines read so far and then discard them. */
					for (i = 0; i < pline_cnt; i++) {
						print_line (cfile, pline[i], line_num + i, 0);
						pline[i][0] = 0;
					}

					/* Print copy file */
					cur = cfile->copy_head;
					if (!cur->replace_head) {
						for (rep = cfile->replace_head;
						     rep && rep->firstline <= line_num;
						     rep = rep->next) {
							copy_list_replace (rep, cur);
						}
					}
					print_program_code (cur, 1);
					cfile->copy_head = cur->next;
					cobc_free (cur);
				}
			} else {
				for (rep = cfile->replace_head;
				     rep && rep->firstline < line_num;
				     rep = rep->next) {
					pline_cnt = print_replace_text (cfile, fd, rep, pline,
								       pline_cnt, line_num);
				}
			}
		}
	}
	return pline_cnt;
}

/* TODO: Modularise! */
static void
print_program_code (struct list_files *cfile, int in_copy)
{
	FILE			*fd = NULL;
	struct list_replace	*rep;
	struct list_files	*cur;
	struct list_error	*err;
	int	i;
	int	line_num;
	const int	fixed = (cfile->source_format == CB_FORMAT_FIXED);
	int	done;
	int	eof;
	int	pline_cnt;
	char	*pline[CB_READ_AHEAD];
	int	prec;

	cfile->listing_on = 1;
	memset (pline, 0, sizeof(pline));
#ifdef DEBUG_REPLACE
	struct list_skip *skip;

	fprintf (stdout, "print_program_code: in_copy = %s\n",
		 in_copy ? "YES" : "NO");
	fprintf (stdout, "   name: %s\n", cfile->name);
	fprintf (stdout, "   copy_line: %d\n", cfile->copy_line);
	for (i = 0, cur = cfile->copy_head; cur; i++, cur = cur->next) {
		if (i == 0) {
			fprintf (stdout, "   copy_books: \n");
		}
		fprintf (stdout, "      name[%d]: %s\n", i, cur->name);
		fprintf (stdout, "      line[%d]: %d\n", i, cur->copy_line);
	}
	for (i = 0, rep = cfile->replace_head; rep; i++, rep = rep->next) {
		if (i == 0) {
			fprintf (stdout, "   replace_list: \n");
		}
		fprintf (stdout, "      line[%d]: %d\n", i, rep->firstline);
		fprintf (stdout, "      from[%d]: '%s'\n", i, rep->from);
		fprintf (stdout, "      to  [%d]: '%s'\n", i, rep->to);
	}
	for (i = 0, err = cfile->err_head; err; i++, err = err->next) {
		if (i == 0) {
			fprintf (stdout, "   error_list: \n");
		}
		fprintf (stdout, "      line[%d]: %d\n", i, err->line);
		fprintf (stdout, "      pref[%d]: '%s'\n", i, err->prefix);
		fprintf (stdout, "      msg [%d]: '%s'\n", i, err->msg);
	}
	for (i = 0, skip = cfile->skip_head; skip; i++, skip = skip->next) {
		if (i == 0) {
			fprintf (stdout, "   skip_list: \n");
		}
		fprintf (stdout, "      line[%d]: %d\n", i, skip->skipline);
	}
#endif
	if (cfile->name) {
		fd = fopen (cfile->name, "r");
	}
	if (fd != NULL) {
		line_num = 1;
		pline_cnt = 0;

		eof = 0;
		done = 0;
		pline_check_limit (pline_cnt, cfile->name, line_num);
		if (get_next_listing_line (fd, &pline[pline_cnt], fixed) >= 0) {

			while (!done) {
				pline_check_limit (pline_cnt, cfile->name, line_num);
				if (get_next_listing_line (fd, &pline[pline_cnt + 1], fixed) < 0) {
					if (eof) {
						done = 1;
						break;
					}
					eof = 1;
				}
				pline_cnt++;
				prec = 0;
				if (is_continuation_line (pline[fixed ? pline_cnt : pline_cnt - 1],
							  cfile->source_format != CB_FORMAT_FREE)) {
					continue;
				}

				if (!strncmp (pline[0], "#line ", 6)) {
					line_num = atoi (&pline[0][6]);
					prec = -1;
				}

				if (!in_copy) {
					pline_cnt = print_replace_main (cfile, fd, pline, pline_cnt,
								       line_num);
				} else if (cfile->replace_head) {
					rep = cfile->replace_head;
					while (rep) {
						pline_cnt = print_replace_text (cfile, fd, rep, pline,
									       pline_cnt, line_num);
						rep = rep->next;
					}
				}

				for (i = 0; i < pline_cnt; i++) {
					if (pline[i][0]) {
						if (pline[i][CB_INDICATOR] == '&') {
							print_line (cfile, pline[i], line_num, in_copy);
						} else {
							print_line (cfile, pline[i], line_num + i, in_copy);
							prec++;
						}
					}
				}

				if (cfile->copy_head) {
					cur = cfile->copy_head;
					if (cur->copy_line == line_num) {
						struct list_replace *repl;

						rep = cfile->replace_head;
						/*  COPY in COPY, add replacement text to new COPY */
						while (rep && in_copy) {
							repl = cobc_malloc (sizeof (struct list_replace));
							memcpy (repl, rep, sizeof (struct list_replace));
							repl->next = NULL;
							if (cur->replace_tail) {
								cur->replace_tail->next = repl;
							}
							if (!cur->replace_head) {
								cur->replace_head = repl;
							}
							cur->replace_tail = repl;
							rep = rep->next;
						}
						print_program_code (cur, 1);
						cfile->copy_head = cur->next;
						cobc_free (cur);
					}
				}
				strcpy (pline[0], pline[pline_cnt]);
				for (i = 1; i < pline_cnt+1; i++) {
				   memset (pline[i], 0, CB_LINE_LENGTH);
				}
				line_num += prec;
				pline_cnt = 0;
				if (pline[0][0] == 0) {
					eof = 1;
				}
			}
		}
		fclose (fd);

		/* Non-existent file, print errors to listing */
	} else {
		if (cfile->err_head) {
			for (err = cfile->err_head; err; err = err->next) {
				print_program_header ();
				fprintf (cb_src_list_file, "%s%s\n", err->prefix, err->msg);
			}
		}
		if (cfile->copy_head) {
			cur = cfile->copy_head;
			print_program_code (cur, 1);
			cfile->copy_head = cur->next;
			cobc_free (cur);
		}
	}

	for (i = 0; i < CB_READ_AHEAD; i++) {
		if (pline[i] == NULL) {
			break;
		}
		cobc_free (pline[i]);
	}
	while (cfile->err_head) {
		err = cfile->err_head;
		cfile->err_head = err->next;
		cobc_free (err);
	}
}

/* Create single-element C source */

static int
process_translate (struct filename *fn)
{
	struct cb_program	*p;
	struct cb_program	*q;
	struct cb_program	*r;
	struct nested_list	*nlp;
	struct handler_struct	*hstr1;
	struct handler_struct	*hstr2;
	struct local_filename	*lf;
	struct list_files       *cfile;
	int			ret;
	int			i;

	/* Initialize */
	cb_source_file = NULL;
	cb_source_line = 0;

	/* Open the input file */
	yyin = fopen (fn->preprocess, "r");
	if (!yyin) {
		cobc_terminate (fn->preprocess);
	}

	if (verbose_output) {
		fputs (_("parsing:"), stderr);
		fprintf (stderr, "\t%s (%s)\n", fn->preprocess, fn->source);
		fflush (stderr);
	}

	current_program = NULL;
	cb_init_constants ();

	/* Parse */
	ret = yyparse ();

	fclose (yyin);
	yyin = NULL;

	/* Release flex buffers - After file close */
	ylex_call_destroy ();

	if (verbose_output) {
		fputs (_("return status:"), stderr);
		fprintf (stderr, "\t%d\n", ret);
		fflush (stderr);
	}

	/* Print the listing for this file */
	if (cb_src_list_file) {
		cfile = cb_listing_files;
		print_program_code (cfile, 0);
		if ((struct list_files *) cb_listing_files == cb_current_file) {
			cb_current_file = cb_current_file->next;
		}
		cb_listing_files = cb_listing_files->next;
		cobc_free (cfile);

		/* Print program trailer */
		print_program_trailer();
	}

	if (ret) {
		return 1;
	}
	if (cb_flag_syntax_only ||
		(current_program && current_program->entry_list == NULL)) {
		return 0;
	}

	/* Set up USE GLOBAL handlers */
	p = current_program;
	for (q = p; q; q = q->next_program) {
		q->global_file_list = cb_list_reverse (q->global_file_list);
		if (q->nested_level) {
			for (r = q->next_program; r; r = r->next_program) {
				if (r->nested_level >= q->nested_level) {
					continue;
				}
				for (i = COB_OPEN_INPUT; i <= COB_OPEN_EXTEND; ++i) {
					hstr1 = &q->global_handler[i];
					hstr2 = &r->global_handler[i];
					if (!hstr1->handler_label &&
					    hstr2->handler_label &&
					    hstr2->handler_label->flag_global) {
						hstr1->handler_label = hstr2->handler_label;
						hstr1->handler_prog = r;
					}
				}
				if (!r->nested_level) {
					break;
				}
			}
		}
	}

	if (verbose_output) {
		fputs (_("translating:"), stderr);
		fprintf (stderr, "\t%s -> %s (%s)\n",
			 fn->preprocess, fn->translate, fn->source);
		fflush (stderr);
	}

	/* Open the output file */
	if (cb_unix_lf) {
		yyout = fopen (fn->translate, "wb");
	} else {
		yyout = fopen (fn->translate, "w");
	}
	if (!yyout) {
		cobc_terminate (fn->translate);
	}

	/* Open the common storage file */
	cb_storage_file_name = fn->trstorage;
	if (cb_unix_lf) {
		cb_storage_file = fopen (cb_storage_file_name, "wb");
	} else {
		cb_storage_file = fopen (cb_storage_file_name, "w");
	}
	if (!cb_storage_file) {
		cobc_terminate (cb_storage_file_name);
	}

	p = program_list_reverse (current_program);

	/* Set up local storage files */
	lf = NULL;
	ret = 1;
	for (q = p; q; q = q->next_program, ret++) {
		lf = cobc_main_malloc (sizeof(struct local_filename));
		lf->local_name = cobc_main_malloc (fn->translate_len + 12U);
		if (q == p && !q->next_program) {
			sprintf (lf->local_name, "%s.l.h", fn->translate);
		} else {
			sprintf (lf->local_name, "%s.l%d.h", fn->translate, ret);
		}
		if (cb_unix_lf) {
			lf->local_fp = fopen (lf->local_name, "wb");
		} else {
			lf->local_fp = fopen (lf->local_name, "w");
		}
		if (!lf->local_fp) {
			cobc_terminate (lf->local_name);
		}
		q->local_include = lf;
		lf->next = fn->localfile;
		fn->localfile = lf;
	}

	/* Entries for COMMON programs */
	for (q = p; q; q = q->next_program) {
		i = q->nested_level;
		for (nlp = q->common_prog_list; nlp; nlp = nlp->next) {
			for (r = q->next_program; r; r = r->next_program) {
				if (r->nested_level <= i) {
					break;
				}
				cb_insert_common_prog (r, nlp->nested_prog);
			}
		}
	}

	/* Translate to C */
	codegen (p, 0);

	/* Close files */
	if(unlikely(fclose (cb_storage_file) != 0)) {
		cobc_terminate (cb_storage_file_name);
	}
	cb_storage_file = NULL;
	if(unlikely(fclose (yyout) != 0)) {
		cobc_terminate (fn->translate);
	}
	yyout = NULL;
	for (q = p; q; q = q->next_program) {
		if (unlikely(!q->local_include->local_fp)) {
			continue;
		}
		if (unlikely(fclose (q->local_include->local_fp) != 0)) {
			cobc_terminate(lf->local_name);
		}
		q->local_include->local_fp = NULL;
	}
	return !!errorcount;
}

/* Create single-element assembly source */

static int
process_compile (struct filename *fn)
{
	char	*name;
	size_t	bufflen;
	size_t	size;

	if (output_name) {
		name = output_name;
	} else {
		name = file_basename (fn->source);
#ifndef	_MSC_VER
		strcat (name, ".s");
#endif
	}
	size = strlen (name);
#ifdef	_MSC_VER
	size *= 2U;
#endif

	bufflen = cobc_cc_len + cobc_cflags_len
			+ size + fn->translate_len
			+ cobc_include_len + 64U;

	cobc_chk_buff_size (bufflen);

#ifdef	_MSC_VER
	sprintf (cobc_buffer, gflag_set ?
		"%s /c %s %s /Od /MDd /Zi /FR /c /Fa\"%s\" /Fo\"%s\" \"%s\"" :
		"%s /c %s %s /MD /c /Fa\"%s\" /Fo\"%s\" \"%s\"",
			cobc_cc, cobc_cflags, cobc_include, name,
			name, fn->translate);
	if (verbose_output) {
		return process (cobc_buffer);
	} else {
		return process_filtered (cobc_buffer, fn);
	}
#elif defined(__WATCOMC__)
	sprintf (cobc_buffer, "%s -fe=\"%s\" -s %s %s %s", cobc_cc, name,
			cobc_cflags, cobc_include, fn->translate);
	return process (cobc_buffer);
#else
	sprintf (cobc_buffer, "%s -S -o \"%s\" %s %s %s", cobc_cc, name,
			cobc_cflags, cobc_include, fn->translate);
	return process(cobc_buffer);
#endif

}

/* Create single-element assembled object */

static int
process_assemble (struct filename *fn)
{
#ifndef _MSC_VER
	int		ret;
#endif
	size_t		bufflen;
#ifdef	__OS400__
	char	*name;
#endif

	bufflen = cobc_cc_len + cobc_cflags_len + fn->object_len
			+ fn->translate_len + cobc_include_len
			+ cobc_pic_flags_len + 64U;

	cobc_chk_buff_size (bufflen);

#ifdef	_MSC_VER
	sprintf (cobc_buffer, gflag_set ?
		"%s /c %s %s /Od /MDd /Zi /FR /Fo\"%s\" \"%s\"" :
		"%s /c %s %s     /MD          /Fo\"%s\" \"%s\"",
			cobc_cc, cobc_cflags, cobc_include,
			fn->object, fn->translate);
	if (verbose_output) {
		return process (cobc_buffer);
	} else {
		return process_filtered (cobc_buffer, fn);
	}
#elif defined(__OS400__)
	name = (char *) fn->translate;
	if (name[0] != '/') {
		char	*p;

		p = cobc_main_malloc (COB_LARGE_BUFF);
		getcwd (p, COB_LARGE_BUFF);
		strcat (p, "/");
		strcat (p, name);
		name = p;
	}
	file_stripext ((char *) fn->object);
	sprintf (cobc_buffer, "%s -c %s %s -o %s %s",
		 cobc_cc, cobc_cflags, cobc_include,
		 fn->object, name);
	ret = process (cobc_buffer);
	if ((ret == 0) && cobc_flag_run) {
		ret = process_run (name);
	}
	return ret;
#elif defined(__WATCOMC__)
	if (cb_compile_level == CB_LEVEL_MODULE ||
	    cb_compile_level == CB_LEVEL_LIBRARY) {
		sprintf (cobc_buffer, "%s -c %s %s %s -fe=\"%s\" \"%s\"",
			 cobc_cc, cobc_cflags, cobc_include,
			 COB_PIC_FLAGS, fn->object, fn->translate);
	} else {
		sprintf (cobc_buffer, "%s -c %s %s -fe=\"%s\" \"%s\"",
			 cobc_cc, cobc_cflags, cobc_include,
			 fn->object, fn->translate);
	}
	ret = process (cobc_buffer);
	if ((ret == 0) && cobc_flag_run) {
		ret = process_run (fn->object);
	}
	return ret;
#else
	if (cb_compile_level == CB_LEVEL_MODULE ||
	    cb_compile_level == CB_LEVEL_LIBRARY ||
	    cb_compile_level == CB_LEVEL_ASSEMBLE) {
		sprintf (cobc_buffer, "%s -c %s %s %s -o \"%s\" \"%s\"",
			 cobc_cc, cobc_cflags, cobc_include,
			 COB_PIC_FLAGS, fn->object, fn->translate);
	} else {
		// Only for CB_LEVEL_EXECUTABLE
		sprintf (cobc_buffer, "%s -c %s %s -o \"%s\" \"%s\"",
			 cobc_cc, cobc_cflags, cobc_include,
			 fn->object, fn->translate);
	}
	ret = process (cobc_buffer);
	return ret;
#endif

}

/* Create single-element loadable object without intermediate stages */

static int
process_module_direct (struct filename *fn)
{
	char	*name;
#ifdef	_MSC_VER
	char	*exename;
#endif
	size_t	bufflen;
	size_t	size;
	int	ret;

	if (output_name) {
		name = output_name_buff;
		strcpy (name, output_name);
#if	defined(_MSC_VER) || defined(__OS400__) || defined(__WATCOMC__) || defined(__BORLANDC__)
		file_stripext (name);
#else
		if (strchr (output_name, '.') == NULL) {
			strcat (name, ".");
			strcat (name, COB_MODULE_EXT);
		}
#endif
	} else {
		name = file_basename (fn->source);
#if	!defined(_MSC_VER) && !defined(__OS400__) && !defined(__WATCOMC__) && !defined(__BORLANDC__)
		strcat (name, ".");
		strcat (name, COB_MODULE_EXT);
#endif
	}
#ifdef	_MSC_VER
	exename = cobc_stradd_dup (name, ".dll");
#endif

	size = strlen (name);
#ifdef	_MSC_VER
	size *= 2U;
#endif

	bufflen = cobc_cc_len + cobc_cflags_len
			+ cobc_export_dyn_len + cobc_shared_opt_len
			+ size + fn->translate_len
			+ cobc_libs_len + cobc_ldflags_len + cobc_include_len
#ifdef	_MSC_VER
			+ manilink_len
#endif
			+ cobc_lib_paths_len + cobc_pic_flags_len + 128U;

	cobc_chk_buff_size (bufflen);

#ifdef	_MSC_VER
	sprintf (cobc_buffer, gflag_set ?
		"%s %s %s /Od /MDd /LDd /Zi /FR /Fe\"%s\" /Fo\"%s\" \"%s\" %s %s %s %s" :
		"%s %s %s     /MD  /LD          /Fe\"%s\" /Fo\"%s\" \"%s\" %s %s %s %s",
			cobc_cc, cobc_cflags, cobc_include, exename, name,
			fn->translate,
			manilink, cobc_ldflags, cobc_libs, cobc_lib_paths);
	if (verbose_output) {
		ret = process (cobc_buffer);
	} else {
		ret = process_filtered (cobc_buffer, fn);
	}
	/* Embedding manifest */
	if (ret == 0) {
		sprintf (cobc_buffer,
			 "%s /manifest \"%s.manifest\" /outputresource:\"%s\";#2",
			 manicmd, exename, exename);
		ret = process (cobc_buffer);
		sprintf (cobc_buffer, "%s.manifest", exename);
		cobc_check_action (cobc_buffer);
	}
	sprintf (cobc_buffer, "%s.exp", name);
	cobc_check_action (cobc_buffer);
	sprintf (cobc_buffer, "%s.lib", name);
	if (strstr(fn->source, cobc_buffer) == NULL)	cobc_check_action (cobc_buffer);
	sprintf (cobc_buffer, "%s.%s", name, COB_OBJECT_EXT);
	if (strstr(fn->source, cobc_buffer) == NULL)	cobc_check_action (cobc_buffer);
#else	/* _MSC_VER */
#ifdef	__OS400__
	if (fn->translate[0] != '/') {
		char	*p;

		p = cobc_main_malloc (COB_LARGE_BUFF);
		getcwd (p, COB_LARGE_BUFF);
		strcat (p, "/");
		strcat (p, fn->translate);
		fn->translate = p;
		fn->translate_len = strlen (p);
	}
#endif
#ifdef	__WATCOMC__
	sprintf (cobc_buffer, "%s %s %s %s %s %s -fe=\"%s\" \"%s\" %s %s %s",
		 cobc_cc, cobc_cflags, cobc_include, COB_SHARED_OPT,
		 COB_PIC_FLAGS, COB_EXPORT_DYN, name,
		 fn->translate, cobc_ldflags, cobc_libs, cobc_lib_paths);
#else
	sprintf (cobc_buffer, "%s %s %s %s %s %s -o \"%s\" \"%s\" %s %s %s",
		 cobc_cc, cobc_cflags, cobc_include, COB_SHARED_OPT,
		 COB_PIC_FLAGS, COB_EXPORT_DYN, name,
		 fn->translate, cobc_ldflags, cobc_libs, cobc_lib_paths);
#endif
	ret = process (cobc_buffer);
#ifdef	COB_STRIP_CMD
	if (strip_output && ret == 0) {
		sprintf (cobc_buffer, "%s \"%s\"", COB_STRIP_CMD, name);
		ret = process (cobc_buffer);
	}
#endif
#endif	/* _MSC_VER */
	if ((ret == 0) && cobc_flag_run) {
		ret = process_run (name);
	}
	return ret;
}

/* Create single-element loadable object */

static int
process_module (struct filename *fn)
{
	char	*name;
#ifdef	_MSC_VER
	char	*exename;
#endif
	size_t	bufflen;
	size_t	size;
	int	ret;

	if (output_name) {
		name = output_name_buff;
		strcpy (name, output_name);
#if	defined(_MSC_VER) || defined(__OS400__) || defined(__WATCOMC__) || defined(__BORLANDC__)
		file_stripext (name);
#else
		if (strchr (output_name, '.') == NULL) {
			strcat (name, ".");
			strcat (name, COB_MODULE_EXT);
		}
#endif
	} else {
		name = file_basename (fn->source);
#if	!defined(_MSC_VER) && !defined(__OS400__) && !defined(__WATCOMC__) &&! defined(__BORLANDC__)
		strcat (name, ".");
		strcat (name, COB_MODULE_EXT);
#endif
	}
#ifdef	_MSC_VER
	exename = cobc_stradd_dup (name, ".dll");
#endif

	size = strlen (name);
	bufflen = cobc_cc_len + cobc_ldflags_len
			+ cobc_export_dyn_len + cobc_shared_opt_len
			+ size + fn->object_len + cobc_libs_len
#ifdef	_MSC_VER
			+ manilink_len
#endif
			+ cobc_lib_paths_len + cobc_pic_flags_len + 128U;

	cobc_chk_buff_size (bufflen);

#ifdef	_MSC_VER
	sprintf (cobc_buffer, gflag_set ?
		"%s /Od /MDd /LDd /Zi /FR /Fe\"%s\" \"%s\" %s %s %s %s" :
		"%s     /MD  /LD          /Fe\"%s\" \"%s\" %s %s %s %s",
		cobc_cc, exename, fn->object,
		manilink, cobc_ldflags, cobc_libs, cobc_lib_paths);
	if (verbose_output) {
		ret = process (cobc_buffer);
	} else {
		ret = process_filtered (cobc_buffer, fn);
	}
	/* Embedding manifest */
	if (ret == 0) {
		sprintf (cobc_buffer,
			 "%s /manifest \"%s.manifest\" /outputresource:\"%s\";#2",
			 manicmd, exename, exename);
		ret = process (cobc_buffer);
		sprintf (cobc_buffer, "%s.manifest", exename);
		cobc_check_action (cobc_buffer);
	}
	sprintf (cobc_buffer, "%s.exp", name);
	cobc_check_action (cobc_buffer);
	sprintf (cobc_buffer, "%s.lib", name);
	if (strstr(fn->source, cobc_buffer) == NULL)	cobc_check_action (cobc_buffer);
	sprintf (cobc_buffer, "%s.obj", name);
	if (strstr(fn->source, cobc_buffer) == NULL)	cobc_check_action (cobc_buffer);
#else	/* _MSC_VER */
#ifdef	__WATCOMC__
	sprintf (cobc_buffer, "%s %s %s %s -fe=\"%s\" \"%s\" %s %s %s",
		 cobc_cc, COB_SHARED_OPT, COB_PIC_FLAGS, COB_EXPORT_DYN,
		 name, fn->object, cobc_ldflags, cobc_libs, cobc_lib_paths);
#else
	sprintf (cobc_buffer, "%s %s %s %s -o \"%s\" \"%s\" %s %s %s",
		 cobc_cc, COB_SHARED_OPT, COB_PIC_FLAGS, COB_EXPORT_DYN,
		 name, fn->object, cobc_ldflags, cobc_libs, cobc_lib_paths);
#endif
	ret = process (cobc_buffer);
#ifdef	COB_STRIP_CMD
	if (strip_output && ret == 0) {
		sprintf (cobc_buffer, "%s %s", COB_STRIP_CMD, name);
		ret = process (cobc_buffer);
	}
#endif
#endif	/* _MSC_VER */
	/* cobcrun job? */
	if ((ret == 0) && cobc_flag_run) {
		ret = process_run (name);
	}
	return ret;
}

/* Create multi-element loadable object */

static int
process_library (struct filename *l)
{
	struct filename	*f;
	char		*name;
#ifdef	_MSC_VER
	char	*exename;
#endif
	size_t		bufflen;
	size_t		size;
	int		ret;

	if (!l) {
		cobc_err_msg (_("call to '%s' with invalid parameter '%s'"),
			"process_library", "l");
		COBC_ABORT ();
	}

	for (f = l; f; f = f->next) {
		strcat (cobc_objects_buffer, "\"");
		strcat (cobc_objects_buffer, f->object);
		strcat (cobc_objects_buffer, "\" ");
	}

	if (output_name) {
		name = output_name_buff;
		strcpy (name, output_name);
#if	defined(_MSC_VER) || defined(__OS400__) || defined(__WATCOMC__) || defined(__BORLANDC__)
		file_stripext (name);
#else
		if (strchr (output_name, '.') == NULL) {
			strcat (name, ".");
			strcat (name, COB_MODULE_EXT);
		}
#endif
	} else {
		name = file_basename (l->source);
#if	!defined(_MSC_VER) && !defined(__OS400__) && !defined(__WATCOMC__) && !defined(__BORLANDC__)
		strcat (name, ".");
		strcat (name, COB_MODULE_EXT);
#endif
	}
#ifdef	_MSC_VER
	exename = cobc_stradd_dup (name, ".dll");
#endif

	size = strlen (name);
	bufflen = cobc_cc_len + cobc_ldflags_len
			+ cobc_export_dyn_len + cobc_shared_opt_len
			+ size + cobc_objects_len + cobc_libs_len
#ifdef	_MSC_VER
			+ manilink_len
#endif
			+ cobc_lib_paths_len + cobc_pic_flags_len + 64U;

	cobc_chk_buff_size (bufflen);

#ifdef	_MSC_VER
	sprintf (cobc_buffer, gflag_set ?
		"%s /Od /MDd /LDd /Zi /FR /Fe\"%s\" %s %s %s %s %s" :
		"%s     /MD  /LD          /Fe\"%s\" %s %s %s %s %s",
		cobc_cc, exename, cobc_objects_buffer,
		manilink, cobc_ldflags, cobc_libs, cobc_lib_paths);
	if (verbose_output) {
		ret = process (cobc_buffer);
	} else {
		ret = process_filtered (cobc_buffer, l);
	}
	/* Embedding manifest */
	if (ret == 0) {
		sprintf (cobc_buffer,
			 "%s /manifest \"%s.manifest\" /outputresource:\"%s\";#2",
			 manicmd, exename, exename);
		ret = process (cobc_buffer);
		sprintf (cobc_buffer, "%s.manifest", exename);
		cobc_check_action (cobc_buffer);
	}
	sprintf (cobc_buffer, "%s.exp", name);
	cobc_check_action (cobc_buffer);
	sprintf (cobc_buffer, "%s.lib", name);

	for (f = l; f; f = f->next) {
		if (strstr (f->source, cobc_buffer) != NULL) {
			break;
		}
	}
	if (!f)	cobc_check_action (cobc_buffer);
#else	/* _MSC_VER */
#ifdef	__WATCOMC__
	sprintf (cobc_buffer, "%s %s %s %s -fe=\"%s\" %s %s %s %s",
		 cobc_cc, COB_SHARED_OPT, COB_PIC_FLAGS,
		 COB_EXPORT_DYN, name, cobc_objects_buffer,
		 cobc_ldflags, cobc_libs, cobc_lib_paths);
#else
	sprintf (cobc_buffer, "%s %s %s %s -o \"%s\" %s %s %s %s",
		 cobc_cc, COB_SHARED_OPT, COB_PIC_FLAGS,
		 COB_EXPORT_DYN, name, cobc_objects_buffer,
		 cobc_ldflags, cobc_libs, cobc_lib_paths);
#endif
	ret = process (cobc_buffer);
#ifdef	COB_STRIP_CMD
	if (strip_output && ret == 0) {
		sprintf (cobc_buffer, "%s \"%s\"", COB_STRIP_CMD, name);
		ret = process (cobc_buffer);
	}
#endif
#endif	/* _MSC_VER */
	/* cobcrun job? */
	if ((ret == 0) && cobc_flag_run) {
		ret = process_run (name);
	}
	return ret;
}

/* Create executable */

static int
process_link (struct filename *l)
{
	struct filename	*f;
	const char		*name;
#ifdef	_MSC_VER
	const char		*exename;
#endif
	size_t		bufflen;
	size_t		size;
	int		ret;

	if (!l) {
		cobc_err_msg (_("call to '%s' with invalid parameter '%s'"),
			"process_link", "l");
		COBC_ABORT ();
	}

	for (f = l; f; f = f->next) {
#ifdef	__OS400__
		file_stripext ((char *) f->object);
		strcat (cobc_objects_buffer, f->object);
		strcat (cobc_objects_buffer, " ");
#else
		strcat (cobc_objects_buffer, "\"");
		strcat (cobc_objects_buffer, f->object);
		strcat (cobc_objects_buffer, "\" ");
#endif
	}

	if (output_name) {
#if	defined(_MSC_VER) || defined(__OS400__) || defined(__WATCOMC__) || defined(__BORLANDC__)
		name = cobc_main_strdup (output_name);
		file_stripext ((char *)name);
#else
		name = output_name;
#endif
	} else {
		if (l->file_is_stdin) {
			name = COB_DASH_OUT;
		} else {
			name = file_basename (l->source);
		}
	}
#ifdef	_MSC_VER
	exename = cobc_stradd_dup (name, ".exe");
#endif

	size = strlen (name);
	bufflen = cobc_cc_len + cobc_ldflags_len
			+ cobc_export_dyn_len + size
			+ cobc_objects_len + cobc_libs_len
#ifdef	_MSC_VER
			+ manilink_len
#endif
			+ cobc_lib_paths_len + 64U;

	cobc_chk_buff_size (bufflen);

#ifdef	_MSC_VER
	sprintf (cobc_buffer, gflag_set ?
		"%s /Od /MDd /Zi /FR /Fe\"%s\" %s %s %s %s %s" :
		"%s     /MD          /Fe\"%s\" %s %s %s %s %s",
		cobc_cc, exename, cobc_objects_buffer,
		manilink, cobc_ldflags, cobc_libs, cobc_lib_paths);
	if (verbose_output) {
		ret = process (cobc_buffer);
	} else {
		ret = process_filtered (cobc_buffer, l);
	}
	/* Embedding manifest */
	if (ret == 0) {
		sprintf (cobc_buffer,
			 "%s /manifest \"%s.manifest\" /outputresource:\"%s\";#1",
			 manicmd, exename, exename);
		ret = process (cobc_buffer);
		sprintf (cobc_buffer, "%s.manifest", exename);
		cobc_check_action (cobc_buffer);
	}
#else	/* _MSC_VER */
#ifdef	__WATCOMC__
	sprintf (cobc_buffer, "%s %s -fe=\"%s\" %s %s %s %s",
		 cobc_cc, COB_EXPORT_DYN, name, cobc_objects_buffer,
		 cobc_ldflags, cobc_libs, cobc_lib_paths);
#else
	sprintf (cobc_buffer, "%s %s -o \"%s\" %s %s %s %s",
		 cobc_cc, COB_EXPORT_DYN, name, cobc_objects_buffer,
		 cobc_ldflags, cobc_libs, cobc_lib_paths);
#endif

	ret = process (cobc_buffer);

#ifdef	__hpux
	if (ret == 0) {
		sprintf (cobc_buffer, "chatr -s +s enable \"%s%s\" 1>/dev/null 2>&1",
			 name, COB_EXEEXT);
		process (cobc_buffer);
	}
#endif

#ifdef	COB_STRIP_CMD
	if (strip_output && ret == 0) {
		sprintf (cobc_buffer, "%s \"%s%s\"",
			 COB_STRIP_CMD, name, COB_EXEEXT);
		ret = process (cobc_buffer);
	}
#endif

#endif	/* _MSC_VER */

	/* run job? */
	if ((ret == 0) && cobc_flag_run) {
		ret = process_run (name);
	}
	return ret;
}

/* Main function */
int
main (int argc, char **argv)
{
	struct filename		*fn;
	char			*p;
	struct cobc_mem_struct	*mptr;
	struct cobc_mem_struct	*mptrt;
	struct list_files	*newfile;
	unsigned int		iparams;
	unsigned int		local_level;
	int			status;
	int			year;
	int			day;
	int			i;
	char			month[32];

#ifdef	ENABLE_NLS
	struct stat	localest;
	const char* localedir;
#endif

	file_list = NULL;
	cb_listing_file = NULL;
	cb_src_list_file = NULL;
	ppin = NULL;
	ppout = NULL;
	yyin = NULL;
	yyout = NULL;


	cob_reg_sighnd (&cobc_sig_handler);

	cb_saveargc = argc;
	cb_saveargv = argv;

	/* General buffers */
	cobc_buffer = cobc_main_malloc ((size_t)COB_LARGE_BUFF);
	cobc_buffer_size = COB_LARGE_MAX;
	basename_buffer = cobc_main_malloc ((size_t)COB_MINI_BUFF);
	basename_len = COB_MINI_MAX - 16;

	cobc_libs = cobc_main_malloc ((size_t)COB_SMALL_BUFF);
	cobc_lib_paths = cobc_main_malloc ((size_t)COB_SMALL_BUFF);
	cobc_cflags = cobc_main_malloc ((size_t)COB_MINI_BUFF);
	cobc_ldflags = cobc_main_malloc ((size_t)COB_MINI_BUFF);
	cobc_include = cobc_main_malloc ((size_t)COB_MINI_BUFF);

	cobc_libs_size = COB_SMALL_MAX;
	cobc_lib_paths_size = COB_SMALL_MAX;
	cobc_cflags_size = COB_MINI_MAX;
	cobc_include_size = COB_MINI_MAX;
	cobc_ldflags_size = COB_MINI_MAX;

	cb_source_file = NULL;
	save_temps_dir = NULL;
	base_string = NULL;
	cobc_objects_len = 0;
	cb_id = 1;
	cb_pic_id = 1;
	cb_attr_id = 1;
	cb_literal_id = 1;
	cb_field_id = 1;
#ifdef	COB_EBCDIC_MACHINE
	cb_ebcdic_sign = 1;
#else
	cb_ebcdic_sign = 0;
#endif

#ifdef	_WIN32
	/* Allows running tests under Win */
	p = getenv ("COB_UNIX_LF");
	if (p && (*p == 'Y' || *p == 'y' ||
		*p == 'O' || *p == 'o' ||
		*p == 'T' || *p == 't' || 
		*p == '1')) {
		cb_unix_lf = 1;
		(void)_setmode (_fileno (stdin), _O_BINARY);
		(void)_setmode (_fileno (stdout), _O_BINARY);
		(void)_setmode (_fileno (stderr), _O_BINARY);
	}
#endif

#ifdef	HAVE_SETLOCALE
	setlocale (LC_ALL, "");
	setlocale (LC_NUMERIC, "C");
#endif

#ifdef	ENABLE_NLS
	localedir = getenv("LOCALEDIR");
	if (localedir != NULL
	&& !stat (localedir, &localest)
	&& (S_ISDIR (localest.st_mode))) {
		bindtextdomain (PACKAGE, localedir);
	} else {
		bindtextdomain (PACKAGE, LOCALEDIR);
	}
	textdomain (PACKAGE);
#endif

	/* Initialize variables */

	/* Set up build time stamp */
	memset (month, 0, sizeof(month));
	day = 0;
	year = 0;
	status = sscanf (__DATE__, "%s %d %d", month, &day, &year);
	if (status == 3) {
		snprintf (cobc_buffer, (size_t)COB_MINI_MAX,
			  "%s %2.2d %4.4d %s", month, day, year, __TIME__);
	} else {
		snprintf (cobc_buffer, (size_t)COB_MINI_MAX,
			  "%s %s", __DATE__, __TIME__);
	}
	cb_oc_build_stamp = cobc_main_strdup (cobc_buffer);

	output_name = NULL;

	cobc_cc = cobc_getenv_path ("COB_CC");
	if (cobc_cc == NULL) {
		cobc_cc = COB_CC;
	}

	cob_config_dir = cobc_getenv_path ("COB_CONFIG_DIR");
	if (cob_config_dir == NULL) {
		cob_config_dir = COB_CONFIG_DIR;
	}

	p = cobc_getenv ("COB_CFLAGS");
	if (p) {
		COBC_ADD_STR (cobc_cflags, p, NULL, NULL);
	} else {
		COBC_ADD_STR (cobc_cflags, COB_CFLAGS, NULL, NULL);
	}

	p = cobc_getenv ("COB_LDFLAGS");
	if (p) {
		COBC_ADD_STR (cobc_ldflags, p, NULL, NULL);
	} else {
		COBC_ADD_STR (cobc_ldflags, COB_LDFLAGS, NULL, NULL);
	}

	p = cobc_getenv ("COB_LIBS");
	if (p) {
		COBC_ADD_STR (cobc_libs, p, NULL, NULL);
	} else {
		COBC_ADD_STR (cobc_libs, COB_LIBS, NULL, NULL);
	}

	p = cobc_getenv ("COB_LDADD");
	if (p) {
		COBC_ADD_STR (cobc_libs, " ", p, NULL);
	}

	p = cobc_getenv ("COB_LIB_PATHS");
	if (p) {
		COBC_ADD_STR (cobc_lib_paths, p, NULL, NULL);
	} else {
		COBC_ADD_STR (cobc_lib_paths, " ", NULL, NULL);
	}

	/* Different styles for warning/error messages */
	p = cobc_getenv ("COB_MSG_FORMAT");
#if defined (_MSC_VER)
	if (p && strcasecmp(p, "GCC") == 0) {
		cb_msg_style = CB_MSG_STYLE_GCC;
	} else {
		cb_msg_style = CB_MSG_STYLE_MSC;
	}
#else
	if (p && strcasecmp(p, "MSC") == 0) {
		cb_msg_style = CB_MSG_STYLE_MSC;
	} else {
		cb_msg_style = CB_MSG_STYLE_GCC;
	}
#endif

	/* Set default computed goto usage if appropriate */
#if	defined(__GNUC__) && !defined(__clang__)
	cb_flag_computed_goto = 1;
#elif	defined(__SUNPRO_C) && __SUNPRO_C >= 0x570
	cb_flag_computed_goto = 1;
#elif	defined(__xlc__) && defined(__IBMC__) && __IBMC__ >= 700
	cb_flag_computed_goto = 1;
	COBC_ADD_STR (cobc_cflags, " -qlanglvl=extended",
		      NULL, NULL);
#endif

	/* Compiler special options */

#ifdef	__WATCOMC__
	/* -s = no overflow checks, otherwise need to code/link a CHK routine */
	COBC_ADD_STR (cobc_cflags, " -s -wcd=118", NULL, NULL);
#endif

#ifdef	__INTEL_COMPILER
	COBC_ADD_STR (cobc_cflags, " -vec-report0 -opt-report 0",
		      NULL, NULL);
#elif	defined(__GNUC__)
#if	0	/* RXWRXW - gcse */
	COBC_ADD_STR (cobc_cflags, " -Wno-unused -fsigned-char -fno-gcse",
		      NULL, NULL);
#else
	COBC_ADD_STR (cobc_cflags, " -Wno-unused -fsigned-char",
		      NULL, NULL);
#endif
	/* --param max-goto-duplication-insns=100000 */
#ifdef	HAVE_PSIGN_OPT
	COBC_ADD_STR (cobc_cflags, " -Wno-pointer-sign", NULL, NULL);
#endif

#elif defined(__xlc__)
	COBC_ADD_STR (cobc_cflags, " -qro -qroconst",
		      NULL, NULL);
#endif

	/* Enable default I/O exceptions */
	CB_EXCEPTION_ENABLE (COB_EC_I_O) = 1;

	/* Compiler initialization I */
#ifndef	HAVE_DESIGNATED_INITS
	cobc_init_reserved ();
#endif

	/* Process command line arguments */
	iargs = process_command_line (argc, argv);

	cb_text_column = cb_config_text_column;
	
	/* Check the filename */
	if (iargs == argc) {
		cobc_err_exit (_("no input files"));
	}

	/* Windows stuff reliant upon verbose option */
#ifdef	_MSC_VER
	if (!verbose_output) {
		COBC_ADD_STR (cobc_cflags, " /nologo", NULL, NULL);
	}
	if (!verbose_output) {
		manicmd = "mt /nologo";
		manilink = "/link /nologo /manifest";
	} else {
		manicmd = "mt";
		manilink = "/link /manifest";
	}
	manilink_len = strlen (manilink);
#endif

#ifdef	__WATCOMC__
	if (!verbose_output) {
		COBC_ADD_STR (cobc_cflags, " -q", NULL, NULL);
	}
#endif

#if	0	/* gcc opts */

#if	defined(__GNUC__) && !defined (__INTEL_COMPILER)
	if (!gflag_set && !cobc_wants_debug && !aflag_set) {
		COBC_ADD_STR (cobc_cflags, " -fomit-frame-pointer",
			      " -fno-asynchronous-unwind-tables",
			      " -U_FORTIFY_SOURCE");
#if	0	/* RXWRXW - Default opt */
		if (!cob_optimize) {
			COBC_ADD_STR (cobc_cflags, CB_COPT_1, NULL, NULL);
		}
#endif
#ifdef	HAVE_UNWIND_OPT
#endif
	}
#endif

#endif

	/* Append default extensions */
	CB_TEXT_LIST_ADD (cb_extension_list, ".CPY");
	CB_TEXT_LIST_ADD (cb_extension_list, ".CBL");
	CB_TEXT_LIST_ADD (cb_extension_list, ".COB");
	CB_TEXT_LIST_ADD (cb_extension_list, ".cpy");
	CB_TEXT_LIST_ADD (cb_extension_list, ".cbl");
	CB_TEXT_LIST_ADD (cb_extension_list, ".cob");
	CB_TEXT_LIST_ADD (cb_extension_list, "");

	/* Process COB_COPY_DIR and COBCPY environment variables */
	process_env_copy_path (getenv ("COB_COPY_DIR"));
	process_env_copy_path (getenv ("COBCPY"));

	/* Add default COB_COPY_DIR directory */
	CB_TEXT_LIST_CHK (cb_include_list, COB_COPY_DIR);

	/* Defaults are set here */
	if (!cb_flag_syntax_only) {
		if (!wants_nonfinal) {
			if (cobc_flag_main) {
				cb_compile_level = CB_LEVEL_EXECUTABLE;
			} else if (cobc_flag_module) {
				cb_compile_level = CB_LEVEL_MODULE;
			} else if (cobc_flag_library) {
				cb_compile_level = CB_LEVEL_LIBRARY;
			} else if (cb_compile_level == 0) {
				cb_compile_level = CB_LEVEL_MODULE;
				cobc_flag_module = 1;
			}
		}
		if (wants_nonfinal && cb_compile_level != CB_LEVEL_PREPROCESS &&
		    !cobc_flag_main && !cobc_flag_module && !cobc_flag_library) {
			cobc_flag_module = 1;
		}
	} else {
		cb_compile_level = CB_LEVEL_TRANSLATE;
		cobc_flag_main = 0;
		cobc_flag_module = 0;
		cobc_flag_library = 0;
	}

	if (output_name && cb_compile_level < CB_LEVEL_LIBRARY &&
	    (argc - iargs) > 1) {
		cobc_err_exit (_("%s option invalid in this combination"), "-o");
	}

	if (cb_flag_notrunc) {
		cb_binary_truncate = 0;
		cb_pretty_display = 0;
	}

	/* Compiler initialization II */
#ifndef	HAVE_DESIGNATED_INITS
	cobc_init_typeck ();
#endif

	memset (cb_listing_title, 0, sizeof (cb_listing_title));
	/* If -P=file specified, all lists go to this file */
	if (cobc_list_file) {
		if (cb_unix_lf) {
			cb_listing_file = fopen (cobc_list_file, "wb");
		} else {
			cb_listing_file = fopen (cobc_list_file, "w");
		}
		if (!cb_listing_file) {
			cobc_terminate (cobc_list_file);
		}
	}

	/* Open source listing file */
	if (cb_listing_outputfile) {
		if (cb_unix_lf) {
			cb_src_list_file = fopen (cb_listing_outputfile, "wb");
		} else {
			cb_src_list_file = fopen (cb_listing_outputfile, "w");
		}
		if (!cb_src_list_file)
			cobc_terminate (cb_listing_outputfile);
	}

	if (verbose_output) {
		fputs (_("command line:"), stderr);
		putc ('\t', stderr);
		for (i = 0; i < argc; ++i) {
			fprintf (stderr, "%s ", argv[i]);
		}
		putc ('\n', stderr);
		fflush (stderr);
	}

	cobc_cc_len = strlen (cobc_cc);
	cobc_cflags_len = strlen (cobc_cflags);
	cobc_libs_len = strlen (cobc_libs);
	cobc_lib_paths_len = strlen (cobc_lib_paths);
	cobc_include_len = strlen (cobc_include);
	cobc_ldflags_len = strlen (cobc_ldflags);
	cobc_export_dyn_len = strlen (COB_EXPORT_DYN);
	cobc_shared_opt_len = strlen (COB_SHARED_OPT);
	cobc_pic_flags_len = strlen (COB_PIC_FLAGS);

	/* Process input files */
	status = 0;
	iparams = 0;
	local_level = 0;

	while (iargs < argc) {

		/* Set up file parameters */
		fn = process_filename (argv[iargs++]);
		if (!fn) {
			cobc_clean_up (1);
			return 1;
		}
#if 1 // checkme, changed by Sergey
	}
	for (fn = file_list; fn; fn = fn->next) {
#endif

		/* Initialize listing */
		if (cb_src_list_file) {
			newfile = cobc_malloc (sizeof (struct list_files));
			memset (newfile, 0, sizeof (struct list_files));
			if (cb_current_file) {
				cb_current_file->next = newfile;
			}
			if (!cb_listing_files) {
				cb_listing_files = newfile;
			}
			cb_current_file = newfile;
			cb_current_file->name = cobc_strdup (fn->source);
			cb_current_file->source_format = cb_source_format;
			cb_current_file = cb_listing_files;
			force_new_page_for_next_line();
		}

		/* Initialize general vars */
		errorcount = 0;
		cb_id = 1;
		cb_pic_id = 1;
		cb_attr_id = 1;
		cb_literal_id = 1;
		cb_field_id = 1;
		demangle_name = fn->demangle_source;
		memset (optimize_defs, 0, sizeof(optimize_defs));

		iparams++;
		if (iparams > 1 && cb_compile_level == CB_LEVEL_EXECUTABLE) {
			local_level = cb_compile_level;
			cb_compile_level = CB_LEVEL_ASSEMBLE;
			cobc_flag_main = 0;
		}


		if (cb_compile_level >= CB_LEVEL_PREPROCESS &&
		    fn->need_preprocess) {
			/* Preprocess */
			fn->has_error = preprocess (fn);
			status |= fn->has_error;
			/* If preprocessing raised errors go on but only check syntax */
			if (fn->has_error) {
				cb_flag_syntax_only = 1;
			}
		}

		if (cobc_list_file) {
			putc ('\n', cb_listing_file);
		}


		if (cb_src_list_file) {
			cb_listing_page = 0;
			strcpy (cb_listing_filename, fn->source);
			set_listing_title_code();
		}

		if (cb_compile_level < CB_LEVEL_TRANSLATE) {
			if (cb_listing_files && cb_src_list_file) {
				print_program_code (cb_listing_files, 0);
				print_program_trailer();
			}
			continue;
		}
		if (fn->need_translate) {
			/* Parse / Translate (to C code) */
			fn->has_error = process_translate (fn);
			status |= fn->has_error;
			/* Free parse memory */
			for (mptr = cobc_parsemem_base; mptr; ) {
				mptrt = mptr;
				mptr = mptr->next;
				cobc_free (mptrt);
			}
			cobc_parsemem_base = NULL;
		}
		if (cb_compile_level < CB_LEVEL_COMPILE ||
		    cb_flag_syntax_only || fn->has_error) {
			continue;
		}
		if (cb_compile_level == CB_LEVEL_COMPILE) {
			/* Compile to assembler code */
			fn->has_error = process_compile (fn);
			status |= fn->has_error;
			continue;
		}

		if (cb_compile_level == CB_LEVEL_MODULE && fn->need_assemble) {
			/* Build module direct */
			fn->has_error = process_module_direct (fn);
			status |= fn->has_error;
		} else {
			/* Compile to object code */
			if (cb_compile_level >= CB_LEVEL_ASSEMBLE &&
			    fn->need_assemble) {
				fn->has_error = process_assemble (fn);
				status |= fn->has_error;
			}
			if (fn->has_error) {
				continue;
			}

			/* Build module */
			if (cb_compile_level == CB_LEVEL_MODULE) {
				fn->has_error = process_module (fn);
				status |= fn->has_error;
			}
		}
	}

	if (cobc_list_file) {
		fclose (cb_listing_file);
		cb_listing_file = NULL;
	}

	/* Clear rest of preprocess stuff */
	plex_clear_all ();

	/* Clear rest of parser stuff */
	ylex_clear_all ();

	if (local_level == CB_LEVEL_EXECUTABLE) {
		cb_compile_level = CB_LEVEL_EXECUTABLE;
	}

	if (cb_compile_level < CB_LEVEL_LIBRARY ||
	    status || cb_flag_syntax_only) {
		/* Finished */
		cobc_clean_up (status);
		return status;
	}

	/* Allocate objects buffer */
	cobc_objects_buffer = cobc_main_malloc (cobc_objects_len);

	if (file_list) {
		/* Link */
		if (cb_compile_level == CB_LEVEL_LIBRARY) {
			/* Multi-program module */
			status = process_library (file_list);
		} else {
			/* Executable */
			status = process_link (file_list);
		}
	}

	/* We have completed */
	cobc_clean_up (status);

	return status;
}
