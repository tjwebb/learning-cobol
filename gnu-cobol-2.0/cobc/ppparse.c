/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         ppparse
#define yylex           pplex
#define yyerror         pperror
#define yydebug         ppdebug
#define yynerrs         ppnerrs

#define yylval          pplval
#define yychar          ppchar

/* Copy the first part of user declarations.  */
#line 29 "ppparse.y" /* yacc.c:339  */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define	COB_IN_PPPARSE	1
#include "cobc.h"

#ifndef	_STDLIB_H
#define	_STDLIB_H 1
#endif

#define pperror(x)	cb_error ("%s", x)

#define COND_EQ		0
#define COND_LT		1U
#define COND_GT		2U
#define COND_LE		3U
#define COND_GE		4U
#define COND_NE		5U

/* Local variables */

static struct cb_define_struct	*ppp_setvar_list;
static unsigned int		current_cmd;

#if	0	/* RXWRXW OPT */
static const char	* const compopts[] = {
	"ibm",
	"ibmcomp",
	"iso2002",
	"mf",
	"mfcomment",
	"sticky-linkage",
	"trunc",
	"noibmcomp",
	"nofold-copy-name",
	"nofoldcopyname",
	"nomfcomment",
	"nosticky-linkage",
	"notrunc"
};

static const char	* const varopts[] = {
	"fold-copy-name",
	"foldcopyname",
	"sourceformat",
	"trunc"
};
#endif

/* Local functions */

static char *
fix_filename (char *name)
{
	/* remove quotation from alphanumeric literals */
	if (name[0] == '\'' || name[0] == '\"') {
		name++;
		name[strlen (name) - 1] = 0;
	}
	return name;
}

static char *
fold_lower (char *name)
{
	unsigned char	*p;

	for (p = (unsigned char *)name; *p; p++) {
		if (isupper (*p)) {
			*p = (cob_u8_t)tolower (*p);
		}
	}
	return name;
}

static char *
fold_upper (char *name)
{
	unsigned char	*p;

	for (p = (unsigned char *)name; *p; p++) {
		if (islower (*p)) {
			*p = (cob_u8_t)toupper (*p);
		}
	}
	return name;
}

static struct cb_replace_list *
ppp_replace_list_add (struct cb_replace_list *list,
		     const struct cb_text_list *old_text,
		     const struct cb_text_list *new_text,
		     const unsigned int lead_or_trail)
{
	struct cb_replace_list *p;

	p = cobc_plex_malloc (sizeof (struct cb_replace_list));
	p->line_num = cb_source_line;
	p->old_text = old_text;
	p->new_text = new_text;
	p->lead_trail = lead_or_trail;
	if (!list) {
		p->last = p;
		return p;
	}
	list->last->next = p;
	list->last = p;
	return list;
}

static unsigned int
ppp_set_value (struct cb_define_struct *p, const char *value)
{
	const char	*s;
	size_t		size;
	unsigned int	dotseen;
	int		sign;
	int		int_part;
	int		dec_part;

	if (!value) {
		p->deftype = PLEX_DEF_NONE;
		p->value = NULL;
		p->sign = 0;
		p->int_part = 0;
		p->dec_part = 0;
		return 0;
	}

	if (*value == '"' || *value == '\'') {
		sign = *value;
		p->value = cobc_plex_strdup (value + 1);
		size = strlen (p->value) - 1;
		if (sign != p->value[size]) {
			p->value = NULL;
			p->deftype = PLEX_DEF_NONE;
			return 1;
		}
		p->value[size] = 0;
		p->deftype = PLEX_DEF_LIT;
		p->sign = 0;
		p->int_part = 0;
		p->dec_part = 0;
		return 0;
	}

	p->value = cobc_plex_strdup (value);
	p->deftype = PLEX_DEF_NUM;
	p->sign = 0;
	p->int_part = 0;
	p->dec_part = 0;

	sign = 0;
	if (*value == '+') {
		value++;
	} else if (*value == '-') {
		value++;
		sign = 1;
	}
	int_part = 0;
	dec_part = 0;
	size = 0;
	dotseen = 0;
	s = value;
	for ( ; *s; ++s, ++size) {
		if (*s == '.') {
			if (dotseen) {
				p->deftype = PLEX_DEF_NONE;
				return 1;
			}
			dotseen = 1;
			continue;
		}
		if (*s > '9' || *s < '0') {
			p->deftype = PLEX_DEF_NONE;
			return 1;
		}
		if (!dotseen) {
			int_part = (int_part * 10) + (*s - '0');
		} else {
			dec_part = (dec_part * 10) + (*s - '0');
		}
	}

	if (!int_part && !dec_part) {
		sign = 0;
	}
	p->sign = sign;
	p->int_part = int_part;
	p->dec_part = dec_part;
	return 0;
}

static unsigned int
ppp_compare_vals (const struct cb_define_struct *p1,
		 const struct cb_define_struct *p2,
		 const unsigned int cond)
{
	int	result;

	if (!p1 || !p2) {
		return 0;
	}
	if (p1->deftype != PLEX_DEF_LIT && p1->deftype != PLEX_DEF_NUM) {
		return 0;
	}
	if (p2->deftype != PLEX_DEF_LIT && p2->deftype != PLEX_DEF_NUM) {
		return 0;
	}
	if (p1->deftype != p2->deftype) {
		cb_warning (_("directive comparison on different types"));
		return 0;
	}
	if (p1->deftype == PLEX_DEF_LIT) {
		result = strcmp (p1->value, p2->value);
	} else {
		if (p1->sign && !p2->sign) {
			result = -1;
		} else if (!p1->sign && p2->sign) {
			result = 1;
		} else if (p1->int_part < p2->int_part) {
			if (p1->sign) {
				result = 1;
			} else {
				result = -1;
			}
		} else if (p1->int_part > p2->int_part) {
			if (p1->sign) {
				result = -1;
			} else {
				result = 1;
			}
		} else if (p1->dec_part < p2->dec_part) {
			if (p1->sign) {
				result = 1;
			} else {
				result = -1;
			}
		} else if (p1->dec_part > p2->dec_part) {
			if (p1->sign) {
				result = -1;
			} else {
				result = 1;
			}
		} else {
			result = 0;
		}
	}
	switch (cond) {
	case COND_EQ:
		return (result == 0);
	case COND_LT:
		return (result < 0);
	case COND_GT:
		return (result > 0);
	case COND_LE:
		return (result <= 0);
	case COND_GE:
		return (result >= 0);
	case COND_NE:
		return (result != 0);
	default:
		break;
	}
	return 0;
}

static struct cb_define_struct *
ppp_define_add (struct cb_define_struct *list, const char *name,
	       const char *text, const unsigned int override)
{
	struct cb_define_struct	*p;
	struct cb_define_struct	*l;

	/* Check duplicate */
	for (l = list; l; l = l->next) {
		if (!strcasecmp (name, l->name)) {
			if (!override && l->deftype != PLEX_DEF_DEL) {
				cb_error (_("duplicate define"));
				return NULL;
			}
			if (l->value) {
				l->value = NULL;
			}
			if (ppp_set_value (l, text)) {
				cb_error (_("invalid constant"));
				return NULL;
			}
			return list;
		}
	}

	p = cobc_plex_malloc (sizeof (struct cb_define_struct));
	p->name = cobc_plex_strdup (name);
	if (ppp_set_value (p, text)) {
		cb_error (_("invalid constant"));
		return NULL;
	}

	if (!list) {
		p->last = p;
		return p;
	}
	list->last->next = p;
	list->last = p;
	return list;
}

static void
ppp_define_del (const char *name)
{
	struct cb_define_struct	*l;

	for (l = ppp_setvar_list; l; l = l->next) {
		if (!strcmp (name, l->name)) {
			l->deftype = PLEX_DEF_DEL;
			if (l->value) {
				l->value = NULL;
			}
			l->sign = 0;
			l->int_part = 0;
			l->dec_part = 0;
			break;
		}
	}
}

static struct cb_define_struct *
ppp_search_lists (const char *name)
{
	struct cb_define_struct	*p;

	for (p = ppp_setvar_list; p; p = p->next) {
		if (!strcasecmp (name, p->name)) {
			if (p->deftype != PLEX_DEF_DEL) {
				return p;
			}
			break;
		}
	}
	return NULL;
}

static struct cb_text_list *
ppp_list_add (struct cb_text_list *list, const char *text)
{
	struct cb_text_list	*p;

	p = cobc_plex_malloc (sizeof (struct cb_text_list));
	p->text = cobc_plex_strdup (text);
	if (!list) {
		p->last = p;
		return p;
	}
	list->last->next = p;
	list->last = p;
	return list;
}

static unsigned int
ppp_search_comp_vars (const char *name)
{
#undef	CB_PARSE_DEF
#define	CB_PARSE_DEF(x,z)	if (!strcasecmp (name, x)) return (z);
#include "ppparse.def"
#undef	CB_PARSE_DEF
	return 0;
}

static unsigned int
ppp_check_needs_quote (const char *envval)
{
	const char	*s;
	size_t		size;
	unsigned int	dot_seen;
	unsigned int	sign_seen;

	/* Non-quoted value - Check if possible numeric */
	dot_seen = 0;
	sign_seen = 0;
	size = 0;
	s = envval;
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
		return 1;
	}
	return 0;
}

/* Global functions */

void
ppparse_clear_vars (const struct cb_define_struct *p)
{
	const struct cb_define_struct	*q;

	ppp_setvar_list = NULL;
	/* Set standard DEFINE's */
	if (cb_perform_osvs) {
		ppp_setvar_list = ppp_define_add (ppp_setvar_list,
						  "PERFORM-TYPE",
						  "'OSVS'", 0);
	} else {
		ppp_setvar_list = ppp_define_add (ppp_setvar_list,
						  "PERFORM-TYPE",
						  "'MF'", 0);
	}
	if (cb_ebcdic_sign) {
		ppp_setvar_list = ppp_define_add (ppp_setvar_list,
						  "SIGN",
						  "'EBCDIC'", 0);
	} else {
		ppp_setvar_list = ppp_define_add (ppp_setvar_list,
						  "SIGN",
						  "'ASCII'", 0);
	}
#ifdef	WORDS_BIGENDIAN
	ppp_setvar_list = ppp_define_add (ppp_setvar_list,
					  "ENDIAN",
					  "'BIG'", 0);
#else
	ppp_setvar_list = ppp_define_add (ppp_setvar_list,
					  "ENDIAN",
					  "'LITTLE'", 0);
#endif
#if	' ' == 0x20
	ppp_setvar_list = ppp_define_add (ppp_setvar_list,
					  "CHARSET",
					  "'ASCII'", 0);
#elif	' ' == 0x40
	ppp_setvar_list = ppp_define_add (ppp_setvar_list,
					  "CHARSET",
					  "'EBCDIC'", 0);
#else
	ppp_setvar_list = ppp_define_add (ppp_setvar_list,
					  "CHARSET",
					  "'UNKNOWN'", 0);
#endif
	/* Set DEFINE's from '-D' option(s) */
	for (q = p; q; q = q->next) {
		ppp_setvar_list = ppp_define_add (ppp_setvar_list,
						  q->name,
						  q->value, 0);
	}
}


#line 547 "ppparse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "ppparse.h".  */
#ifndef YY_PP_PPPARSE_H_INCLUDED
# define YY_PP_PPPARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int ppdebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOKEN_EOF = 0,
    ALSO = 258,
    BY = 259,
    COPY = 260,
    EQEQ = 261,
    IN = 262,
    LAST = 263,
    LEADING = 264,
    OF = 265,
    OFF = 266,
    PRINTING = 267,
    REPLACE = 268,
    REPLACING = 269,
    SUPPRESS = 270,
    TRAILING = 271,
    DOT = 272,
    GARBAGE = 273,
    PAGE_DIRECTIVE = 274,
    LISTING_DIRECTIVE = 275,
    SOURCE_DIRECTIVE = 276,
    FORMAT = 277,
    IS = 278,
    FIXED = 279,
    FREE = 280,
    VARIABLE = 281,
    DEFINE_DIRECTIVE = 282,
    AS = 283,
    PARAMETER = 284,
    OVERRIDE = 285,
    SET_DIRECTIVE = 286,
    CONSTANT = 287,
    SOURCEFORMAT = 288,
    FOLDCOPYNAME = 289,
    NOFOLDCOPYNAME = 290,
    IF_DIRECTIVE = 291,
    ELSE_DIRECTIVE = 292,
    ENDIF_DIRECTIVE = 293,
    ELIF_DIRECTIVE = 294,
    GE = 295,
    LE = 296,
    LT = 297,
    GT = 298,
    EQ = 299,
    NE = 300,
    NOT = 301,
    THAN = 302,
    TO = 303,
    OR = 304,
    EQUAL = 305,
    GREATER = 306,
    LESS = 307,
    SET = 308,
    DEFINED = 309,
    TURN_DIRECTIVE = 310,
    ON = 311,
    CHECKING = 312,
    WITH = 313,
    LOCATION = 314,
    TERMINATOR = 315,
    TOKEN = 316,
    VARIABLE_NAME = 317,
    LITERAL = 318
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 502 "ppparse.y" /* yacc.c:355  */

	char			*s;
	struct cb_text_list	*l;
	struct cb_replace_list	*r;
	struct cb_define_struct	*ds;
	unsigned int		ui;
	int			si;

#line 661 "ppparse.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE pplval;

int ppparse (void);

#endif /* !YY_PP_PPPARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 676 "ppparse.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   172

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  47
/* YYNRULES -- Number of rules.  */
#define YYNRULES  122
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  187

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   318

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      64,    65,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   609,   609,   610,   614,   615,   616,   620,   621,   622,
     623,   624,   625,   627,   626,   632,   631,   636,   640,   647,
     648,   652,   662,   663,   695,   699,   727,   730,   737,   741,
     746,   750,   755,   763,   767,   801,   810,   820,   828,   834,
     835,   839,   846,   847,   850,   852,   853,   854,   858,   859,
     863,   870,   877,   887,   902,   909,   910,   914,   927,   941,
     945,   949,   953,   957,   961,   965,   969,   973,   977,   981,
     988,  1011,  1014,  1021,  1022,  1025,  1026,  1031,  1034,  1041,
    1045,  1052,  1056,  1060,  1064,  1071,  1075,  1082,  1086,  1090,
    1097,  1104,  1108,  1115,  1119,  1126,  1130,  1137,  1144,  1159,
    1163,  1171,  1175,  1185,  1188,  1196,  1199,  1207,  1210,  1218,
    1221,  1227,  1227,  1228,  1228,  1229,  1229,  1230,  1230,  1231,
    1231,  1232,  1232
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "$undefined", "ALSO", "BY", "COPY",
  "\"==\"", "IN", "LAST", "LEADING", "OF", "OFF", "PRINTING", "REPLACE",
  "REPLACING", "SUPPRESS", "TRAILING", "\".\"", "\"word\"",
  "PAGE_DIRECTIVE", "LISTING_DIRECTIVE", "SOURCE_DIRECTIVE", "FORMAT",
  "IS", "FIXED", "FREE", "VARIABLE", "DEFINE_DIRECTIVE", "AS", "PARAMETER",
  "OVERRIDE", "SET_DIRECTIVE", "CONSTANT", "SOURCEFORMAT", "FOLDCOPYNAME",
  "NOFOLDCOPYNAME", "IF_DIRECTIVE", "ELSE_DIRECTIVE", "ENDIF_DIRECTIVE",
  "ELIF_DIRECTIVE", "\">=\"", "\"<=\"", "\"<\"", "\">\"", "\"=\"",
  "\"<>\"", "NOT", "THAN", "TO", "OR", "EQUAL", "GREATER", "LESS", "SET",
  "DEFINED", "TURN_DIRECTIVE", "ON", "CHECKING", "WITH", "LOCATION",
  "\"end of line\"", "\"Identifier or Literal\"", "\"Variable\"",
  "\"Literal\"", "'('", "')'", "$accept", "statement_list", "statement",
  "directive", "$@1", "$@2", "set_directive", "set_choice", "set_options",
  "source_directive", "format_type", "define_directive", "page_directive",
  "listing_directive", "turn_directive", "ec_list", "on_or_off",
  "with_loc", "if_directive", "variable_or_literal", "object_id",
  "condition_clause", "copy_statement", "copy_in", "in_or_of",
  "copy_suppress", "copy_replacing", "replace_statement", "replacing_list",
  "text_src", "text_dst", "text_partial_src", "text_partial_dst",
  "token_list", "identifier", "subscripts", "lead_trail", "_override",
  "_not", "_also", "_last", "_as", "_format", "_is", "_printing", "_than",
  "_to", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,    40,    41
};
# endif

#define YYPACT_NINF -102

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-102)))

#define YYTABLE_NINF -112

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -102,     3,  -102,   -16,   115,  -102,    -2,    31,    14,    46,
    -102,  -102,  -102,  -102,   -37,  -102,    26,    38,    78,    37,
    -102,  -102,     1,    52,  -102,  -102,  -102,  -102,  -102,  -102,
      73,    48,    15,  -102,  -102,  -102,    49,    92,    92,  -102,
     -13,    46,  -102,   -27,   -27,  -102,  -102,   -36,  -102,  -102,
    -102,  -102,  -102,   104,    60,    61,  -102,  -102,  -102,     1,
     120,    -5,   119,  -102,  -102,    91,    92,  -102,     9,    92,
      64,    65,  -102,    66,  -102,    -4,     4,  -102,  -102,  -102,
      -7,  -102,   118,   117,  -102,  -102,     0,   128,   119,     5,
      72,    74,    75,    76,   130,  -102,  -102,  -102,  -102,  -102,
      77,  -102,   108,   108,    79,  -102,  -102,  -102,    93,    93,
    -102,    12,    82,  -102,  -102,  -102,  -102,  -102,     1,  -102,
    -102,  -102,     5,   139,     6,  -102,    -5,  -102,  -102,  -102,
     -28,   138,   140,   108,  -102,  -102,  -102,  -102,  -102,    47,
      62,  -102,  -102,     1,  -102,   140,  -102,     7,  -102,  -102,
    -102,     8,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,
      97,   100,   100,  -102,  -102,    11,    11,  -102,  -102,  -102,
     142,  -102,  -102,  -102,   101,   102,  -102,  -102,  -102,  -102,
    -102,    99,   103,    97,    97,  -102,  -102
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,   107,    38,     0,   113,     0,     0,
      13,    17,    18,    15,     0,     3,     0,     0,     0,    71,
     108,   110,     0,     0,    11,    40,    39,    12,   114,     7,
     115,     0,   111,    56,     8,    37,     0,   111,   111,    24,
      26,     9,    19,     0,     0,    42,    10,     0,     6,     4,
       5,    73,    74,    75,     0,     0,   101,   102,    95,    79,
       0,    86,     0,    80,   116,     0,   111,   112,     0,   111,
       0,     0,    22,     0,    20,   115,   115,    14,    54,    16,
      44,    43,   117,    77,    72,    93,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,    29,    30,    31,    28,
       0,    33,   103,   103,     0,    23,    25,    27,   105,   105,
      45,     0,     0,    49,    41,    47,   118,    76,     0,    70,
      85,    94,     0,     0,     0,    81,    89,    96,    97,    99,
       0,     0,     0,   103,   104,    34,    35,    21,   106,     0,
       0,    46,    48,    78,    83,     0,    87,     0,   100,    98,
      90,     0,    82,    36,    64,    66,    67,    65,    68,    69,
     121,   119,   119,    51,    50,     0,     0,    84,    88,    91,
       0,   122,    63,   120,    60,    62,    58,    57,    52,    53,
      92,     0,     0,   121,   121,    59,    61
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -102,  -102,  -102,  -102,  -102,  -102,  -102,   111,  -102,  -102,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,    43,   112,   147,
      -8,    17,  -102,  -102,  -102,  -102,  -102,  -102,    41,   -59,
      39,    80,    19,    36,   -57,  -102,   -58,   -85,    53,  -102,
    -102,    -9,  -102,    18,  -102,    10,  -101
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    15,    16,    43,    44,    41,    42,    72,    29,
      99,    34,    24,    27,    46,    47,   114,   115,    77,    78,
     178,   165,    17,    53,    54,    83,   119,    18,    59,    60,
     125,    94,   152,    86,    61,   130,    62,   135,   139,    22,
      23,    68,    30,    65,   117,   174,   172
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      87,    88,    90,     2,   110,    91,   120,    55,     3,    25,
      56,   124,   146,   168,   169,    67,     4,    57,   136,    64,
     101,    80,     5,     6,     7,    45,    81,    64,    70,    71,
       8,    73,   126,   148,     9,    75,    76,   149,   102,    10,
      11,    12,    13,    67,    51,    19,    31,    52,   153,   111,
    -111,   112,   113,    28,    26,    49,   -55,   100,    14,    92,
     104,   121,    58,    63,   -56,   126,    58,    85,   121,   170,
     112,   113,   103,   176,   177,   -55,    32,    33,    36,    37,
      38,    39,   185,   186,    87,    88,    48,   154,   155,   156,
     157,   158,   159,   108,   109,    50,    64,   160,   161,   162,
     163,   164,   154,   155,   156,   157,   158,   159,    40,    95,
      66,    69,   160,   161,   162,    96,    97,    98,    20,    82,
      67,    84,    85,    21,    89,    93,  -109,   105,   106,   107,
     116,   118,   122,   127,   132,   128,   129,   131,   134,   138,
     133,   142,   137,   145,   150,   171,   151,   173,   180,   183,
     181,   182,    74,   184,   141,    35,    79,   166,   179,   143,
     147,   144,   140,     0,   167,     0,     0,     0,   123,     0,
       0,     0,   175
};

static const yytype_int16 yycheck[] =
{
      59,    59,     7,     0,    11,    10,     6,     6,     5,    11,
       9,     6,     6,     6,     6,    28,    13,    16,   103,    23,
      11,    57,    19,    20,    21,    62,    62,    23,    37,    38,
      27,    40,    89,    61,    31,    62,    63,    65,    29,    36,
      37,    38,    39,    28,     7,    61,    32,    10,   133,    56,
      63,    58,    59,    22,    56,    17,    60,    66,    55,    64,
      69,    61,    61,    11,    60,   122,    61,    61,    61,    61,
      58,    59,    63,    62,    63,    60,    62,    63,    32,    33,
      34,    35,   183,   184,   143,   143,    60,    40,    41,    42,
      43,    44,    45,    75,    76,    17,    23,    50,    51,    52,
      53,    54,    40,    41,    42,    43,    44,    45,    62,    18,
      62,    62,    50,    51,    52,    24,    25,    26,     3,    15,
      28,    61,    61,     8,     4,     6,    11,    63,    63,    63,
      12,    14,     4,    61,     4,    61,    61,    61,    30,    46,
      63,    59,    63,     4,     6,    48,     6,    47,     6,    50,
      49,    49,    41,    50,   111,     8,    44,   140,   166,   118,
     124,   122,   109,    -1,   145,    -1,    -1,    -1,    88,    -1,
      -1,    -1,   162
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    67,     0,     5,    13,    19,    20,    21,    27,    31,
      36,    37,    38,    39,    55,    68,    69,    88,    93,    61,
       3,     8,   105,   106,    78,    11,    56,    79,    22,    75,
     108,    32,    62,    63,    77,    85,    32,    33,    34,    35,
      62,    72,    73,    70,    71,    62,    80,    81,    60,    17,
      17,     7,    10,    89,    90,     6,     9,    16,    61,    94,
      95,   100,   102,    11,    23,   109,    62,    28,   107,    62,
     107,   107,    74,   107,    73,    62,    63,    84,    85,    84,
      57,    62,    15,    91,    61,    61,    99,    95,   102,     4,
       7,    10,    64,     6,    97,    18,    24,    25,    26,    76,
     107,    11,    29,    63,   107,    63,    63,    63,   109,   109,
      11,    56,    58,    59,    82,    83,    12,   110,    14,    92,
       6,    61,     4,    97,     6,    96,   100,    61,    61,    61,
     101,    61,     4,    63,    30,   103,   103,    63,    46,   104,
     104,    83,    59,    94,    96,     4,     6,    99,    61,    65,
       6,     6,    98,   103,    40,    41,    42,    43,    44,    45,
      50,    51,    52,    53,    54,    87,    87,    98,     6,     6,
      61,    48,   112,    47,   111,   111,    62,    63,    86,    86,
       6,    49,    49,    50,    50,   112,   112
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    66,    67,    67,    68,    68,    68,    69,    69,    69,
      69,    69,    69,    70,    69,    71,    69,    69,    69,    72,
      72,    73,    73,    73,    73,    73,    74,    74,    75,    76,
      76,    76,    76,    77,    77,    77,    77,    77,    78,    79,
      79,    80,    81,    81,    82,    82,    82,    82,    83,    83,
      84,    84,    84,    84,    84,    85,    85,    86,    86,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      88,    89,    89,    90,    90,    91,    91,    92,    92,    93,
      93,    94,    94,    94,    94,    95,    95,    96,    96,    96,
      97,    98,    98,    99,    99,   100,   100,   100,   100,   101,
     101,   102,   102,   103,   103,   104,   104,   105,   105,   106,
     106,   107,   107,   108,   108,   109,   109,   110,   110,   111,
     111,   112,   112
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     0,     3,     0,     3,     1,     1,     1,
       2,     4,     2,     3,     1,     3,     0,     2,     3,     1,
       1,     1,     1,     3,     4,     4,     5,     1,     0,     1,
       1,     3,     1,     2,     0,     1,     2,     1,     2,     1,
       4,     4,     5,     5,     1,     1,     1,     1,     1,     5,
       2,     5,     2,     2,     1,     1,     1,     1,     1,     1,
       5,     0,     2,     1,     1,     0,     2,     0,     2,     3,
       3,     3,     4,     4,     5,     3,     1,     2,     3,     1,
       3,     2,     3,     1,     2,     1,     3,     3,     4,     1,
       2,     1,     1,     0,     1,     0,     1,     0,     1,     0,
       1,     0,     1,     0,     1,     0,     1,     0,     1,     0,
       1,     0,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 13:
#line 627 "ppparse.y" /* yacc.c:1646  */
    {
	current_cmd = PLEX_ACT_IF;
  }
#line 1907 "ppparse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 632 "ppparse.y" /* yacc.c:1646  */
    {
	current_cmd = PLEX_ACT_ELIF;
  }
#line 1915 "ppparse.c" /* yacc.c:1646  */
    break;

  case 17:
#line 637 "ppparse.y" /* yacc.c:1646  */
    {
	plex_action_directive (PLEX_ACT_ELSE, 0);
  }
#line 1923 "ppparse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 641 "ppparse.y" /* yacc.c:1646  */
    {
	plex_action_directive (PLEX_ACT_END, 0);
  }
#line 1931 "ppparse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 653 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_define_struct	*p;

	p = ppp_define_add (ppp_setvar_list, (yyvsp[-2].s), (yyvsp[0].s), 1);
	if (p) {
		ppp_setvar_list = p;
		fprintf (ppout, "#DEFLIT %s %s\n", (yyvsp[-2].s), (yyvsp[0].s));
	}
  }
#line 1945 "ppparse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 664 "ppparse.y" /* yacc.c:1646  */
    {
	char	*p;
	size_t	size;
	int	quote;

	p = (yyvsp[0].s);
	if (*p == '\"' || *p == '\'') {
		quote = *p;
		p++;
		size = strlen (p) - 1;
		if (p[size] != quote) {
			cb_error (_("invalid %s directive"), "SOURCEFORMAT");
		}
		p[size] = 0;
	}
	if (!strcasecmp (p, "FIXED")) {
		cb_source_format = CB_FORMAT_FIXED;
		cb_text_column = cb_config_text_column;
	} else if (!strcasecmp (p, "FREE")) {
		cb_source_format = CB_FORMAT_FREE;
	} else if (!strcasecmp (p, "VARIABLE")) {
		cb_source_format = CB_FORMAT_FIXED;
		/* This is an arbitrary value; perhaps change later? */
		cb_text_column = 500;
	} else {
		cb_error (_("invalid %s directive"), "SOURCEFORMAT");
	}
	if (cb_src_list_file) {
		cb_current_file->source_format = cb_source_format;
	}
  }
#line 1981 "ppparse.c" /* yacc.c:1646  */
    break;

  case 24:
#line 696 "ppparse.y" /* yacc.c:1646  */
    {
	cb_fold_copy = 0;
  }
#line 1989 "ppparse.c" /* yacc.c:1646  */
    break;

  case 25:
#line 700 "ppparse.y" /* yacc.c:1646  */
    {
	char	*p;
	size_t	size;
	int	quote;

	p = (yyvsp[0].s);
	if (*p == '\"' || *p == '\'') {
		quote = *p;
		p++;
		size = strlen (p) - 1;
		if (p[size] != quote) {
			cb_error (_("invalid %s directive"), "FOLD-COPY-NAME");
		}
		p[size] = 0;
	}
	if (!strcasecmp (p, "UPPER")) {
		cb_fold_copy = COB_FOLD_UPPER;
	} else if (!strcasecmp (p, "LOWER")) {
		cb_fold_copy = COB_FOLD_LOWER;
	} else {
		cb_error (_("invalid %s directive"), "FOLD-COPY-NAME");
	}
  }
#line 2017 "ppparse.c" /* yacc.c:1646  */
    break;

  case 26:
#line 727 "ppparse.y" /* yacc.c:1646  */
    {
	fprintf (ppout, "#OPTION %s\n", (yyvsp[0].s));
  }
#line 2025 "ppparse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 731 "ppparse.y" /* yacc.c:1646  */
    {
	fprintf (ppout, "#OPTION %s %s\n", (yyvsp[-2].s), (yyvsp[0].s));
  }
#line 2033 "ppparse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 742 "ppparse.y" /* yacc.c:1646  */
    {
	cb_source_format = CB_FORMAT_FIXED;
	cb_text_column = cb_config_text_column;
  }
#line 2042 "ppparse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 747 "ppparse.y" /* yacc.c:1646  */
    {
	cb_source_format = CB_FORMAT_FREE;
  }
#line 2050 "ppparse.c" /* yacc.c:1646  */
    break;

  case 31:
#line 751 "ppparse.y" /* yacc.c:1646  */
    {
	cb_source_format = CB_FORMAT_FIXED;
	cb_text_column = 500;
  }
#line 2059 "ppparse.c" /* yacc.c:1646  */
    break;

  case 32:
#line 756 "ppparse.y" /* yacc.c:1646  */
    {
	cb_error (_("invalid %s directive"), "SOURCE");
	YYERROR;
  }
#line 2068 "ppparse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 764 "ppparse.y" /* yacc.c:1646  */
    {
	ppp_define_del ((yyvsp[-2].s));
  }
#line 2076 "ppparse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 768 "ppparse.y" /* yacc.c:1646  */
    {
	char			*s;
	char			*q;
	struct cb_define_struct	*p;
	size_t			size;

	s = getenv ((yyvsp[-3].s));
	q = NULL;
	if (s && *s && *s != ' ') {
		if (*s == '"' || *s == '\'') {
			size = strlen (s) - 1U;
			/* Ignore if improperly quoted */
			if (s[0] == s[size]) {
				q = s;
			}
		} else {
			if (ppp_check_needs_quote (s)) {
				/* Alphanumeric literal */
				q = cobc_plex_malloc (strlen (s) + 4U);
				sprintf (q, "'%s'", s);
			} else {
				/* Numeric literal */
				q = s;
			}
		}
	}
	if (q) {
		p = ppp_define_add (ppp_setvar_list, (yyvsp[-3].s), q, (yyvsp[0].ui));
		if (p) {
			ppp_setvar_list = p;
		}
	}
  }
#line 2114 "ppparse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 802 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_define_struct	*p;

	p = ppp_define_add (ppp_setvar_list, (yyvsp[-3].s), (yyvsp[-1].s), (yyvsp[0].ui));
	if (p) {
		ppp_setvar_list = p;
	}
  }
#line 2127 "ppparse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 811 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_define_struct	*p;

	p = ppp_define_add (ppp_setvar_list, (yyvsp[-3].s), (yyvsp[-1].s), (yyvsp[0].ui));
	if (p) {
		ppp_setvar_list = p;
		fprintf (ppout, "#DEFLIT %s %s\n", (yyvsp[-3].s), (yyvsp[-1].s));
	}
  }
#line 2141 "ppparse.c" /* yacc.c:1646  */
    break;

  case 37:
#line 821 "ppparse.y" /* yacc.c:1646  */
    {
	cb_error (_("invalid %s directive"), "DEFINE/SET");
  }
#line 2149 "ppparse.c" /* yacc.c:1646  */
    break;

  case 38:
#line 828 "ppparse.y" /* yacc.c:1646  */
    {
	CB_PENDING (_("PAGE directive"));
  }
#line 2157 "ppparse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 840 "ppparse.y" /* yacc.c:1646  */
    {
	CB_PENDING (_("TURN directive"));
  }
#line 2165 "ppparse.c" /* yacc.c:1646  */
    break;

  case 50:
#line 864 "ppparse.y" /* yacc.c:1646  */
    {
	unsigned int		found;

	found = (ppp_search_lists ((yyvsp[-3].s)) != NULL);
	plex_action_directive (current_cmd, found ^ (yyvsp[-1].ui));
  }
#line 2176 "ppparse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 871 "ppparse.y" /* yacc.c:1646  */
    {
	unsigned int		found;

	found = ppp_search_comp_vars ((yyvsp[-3].s));
	plex_action_directive (current_cmd, found ^ (yyvsp[-1].ui));
  }
#line 2187 "ppparse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 878 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_define_struct	*p;
	unsigned int		found;

	found = 0;
	p = ppp_search_lists ((yyvsp[-4].s));
	found = ppp_compare_vals (p, (yyvsp[0].ds), (yyvsp[-1].ui));
	plex_action_directive (current_cmd, found ^ (yyvsp[-2].ui));
  }
#line 2201 "ppparse.c" /* yacc.c:1646  */
    break;

  case 53:
#line 888 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_define_struct	*p;
	unsigned int		found;

	found = 0;
	p = cobc_plex_malloc (sizeof (struct cb_define_struct));
	p->next = NULL;
	if (ppp_set_value (p, (yyvsp[-4].s))) {
		cb_error (_("invalid constant"));
	} else {
		found = ppp_compare_vals (p, (yyvsp[0].ds), (yyvsp[-1].ui));
	}
	plex_action_directive (current_cmd, found ^ (yyvsp[-2].ui));
  }
#line 2220 "ppparse.c" /* yacc.c:1646  */
    break;

  case 54:
#line 903 "ppparse.y" /* yacc.c:1646  */
    {
	cb_error (_("invalid %s directive"), "IF/ELIF");
  }
#line 2228 "ppparse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 915 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_define_struct	*p;

	p = cobc_plex_malloc (sizeof (struct cb_define_struct));
	p->next = NULL;
	if (ppp_set_value (p, (yyvsp[0].s))) {
		cb_error (_("invalid constant"));
		(yyval.ds) = NULL;
	} else {
		(yyval.ds) = p;
	}
  }
#line 2245 "ppparse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 928 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_define_struct	*p;

	p = ppp_search_lists ((yyvsp[0].s));
	if (p != NULL && p->deftype != PLEX_DEF_NONE) {
		(yyval.ds) = p;
	} else {
		(yyval.ds) = NULL;
	}
  }
#line 2260 "ppparse.c" /* yacc.c:1646  */
    break;

  case 59:
#line 942 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_GE;
  }
#line 2268 "ppparse.c" /* yacc.c:1646  */
    break;

  case 60:
#line 946 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_GT;
  }
#line 2276 "ppparse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 950 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_LE;
  }
#line 2284 "ppparse.c" /* yacc.c:1646  */
    break;

  case 62:
#line 954 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_LT;
  }
#line 2292 "ppparse.c" /* yacc.c:1646  */
    break;

  case 63:
#line 958 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_EQ;
  }
#line 2300 "ppparse.c" /* yacc.c:1646  */
    break;

  case 64:
#line 962 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_GE;
  }
#line 2308 "ppparse.c" /* yacc.c:1646  */
    break;

  case 65:
#line 966 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_GT;
  }
#line 2316 "ppparse.c" /* yacc.c:1646  */
    break;

  case 66:
#line 970 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_LE;
  }
#line 2324 "ppparse.c" /* yacc.c:1646  */
    break;

  case 67:
#line 974 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_LT;
  }
#line 2332 "ppparse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 978 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_EQ;
  }
#line 2340 "ppparse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 982 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = COND_NE;
  }
#line 2348 "ppparse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 989 "ppparse.y" /* yacc.c:1646  */
    {
	fputc ('\n', ppout);
	(yyvsp[-3].s) = fix_filename ((yyvsp[-3].s));
	if (cb_fold_copy == COB_FOLD_LOWER) {
		(yyvsp[-3].s) = fold_lower ((yyvsp[-3].s));
	} else if (cb_fold_copy == COB_FOLD_UPPER) {
		(yyvsp[-3].s) = fold_upper ((yyvsp[-3].s));
	}
	if ((yyvsp[-2].s)) {
		(yyvsp[-2].s) = fix_filename ((yyvsp[-2].s));
		if (cb_fold_copy == COB_FOLD_LOWER) {
			(yyvsp[-2].s) = fold_lower ((yyvsp[-2].s));
		} else if (cb_fold_copy == COB_FOLD_UPPER) {
			(yyvsp[-2].s) = fold_upper ((yyvsp[-2].s));
		}
	}
	ppcopy ((yyvsp[-3].s), (yyvsp[-2].s), (yyvsp[0].r));
  }
#line 2371 "ppparse.c" /* yacc.c:1646  */
    break;

  case 71:
#line 1011 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.s) = NULL;
  }
#line 2379 "ppparse.c" /* yacc.c:1646  */
    break;

  case 72:
#line 1015 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.s) = (yyvsp[0].s);
  }
#line 2387 "ppparse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 1031 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.r) = NULL;
  }
#line 2395 "ppparse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 1035 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.r) = (yyvsp[0].r);
  }
#line 2403 "ppparse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 1042 "ppparse.y" /* yacc.c:1646  */
    {
	pp_set_replace_list ((yyvsp[0].r), (yyvsp[-1].ui));
  }
#line 2411 "ppparse.c" /* yacc.c:1646  */
    break;

  case 80:
#line 1046 "ppparse.y" /* yacc.c:1646  */
    {
	pp_set_replace_list (NULL, (yyvsp[-1].ui));
  }
#line 2419 "ppparse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 1053 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.r) = ppp_replace_list_add (NULL, (yyvsp[-2].l), (yyvsp[0].l), 0);
  }
#line 2427 "ppparse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 1057 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.r) = ppp_replace_list_add (NULL, (yyvsp[-2].l), (yyvsp[0].l), (yyvsp[-3].ui));
  }
#line 2435 "ppparse.c" /* yacc.c:1646  */
    break;

  case 83:
#line 1061 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.r) = ppp_replace_list_add ((yyvsp[-3].r), (yyvsp[-2].l), (yyvsp[0].l), 0);
  }
#line 2443 "ppparse.c" /* yacc.c:1646  */
    break;

  case 84:
#line 1065 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.r) = ppp_replace_list_add ((yyvsp[-4].r), (yyvsp[-2].l), (yyvsp[0].l), (yyvsp[-3].ui));
  }
#line 2451 "ppparse.c" /* yacc.c:1646  */
    break;

  case 85:
#line 1072 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = (yyvsp[-1].l);
  }
#line 2459 "ppparse.c" /* yacc.c:1646  */
    break;

  case 86:
#line 1076 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = (yyvsp[0].l);
  }
#line 2467 "ppparse.c" /* yacc.c:1646  */
    break;

  case 87:
#line 1083 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = NULL;
  }
#line 2475 "ppparse.c" /* yacc.c:1646  */
    break;

  case 88:
#line 1087 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = (yyvsp[-1].l);
  }
#line 2483 "ppparse.c" /* yacc.c:1646  */
    break;

  case 89:
#line 1091 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = (yyvsp[0].l);
  }
#line 2491 "ppparse.c" /* yacc.c:1646  */
    break;

  case 90:
#line 1098 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add (NULL, (yyvsp[-1].s));
  }
#line 2499 "ppparse.c" /* yacc.c:1646  */
    break;

  case 91:
#line 1105 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = NULL;
  }
#line 2507 "ppparse.c" /* yacc.c:1646  */
    break;

  case 92:
#line 1109 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add (NULL, (yyvsp[-1].s));
  }
#line 2515 "ppparse.c" /* yacc.c:1646  */
    break;

  case 93:
#line 1116 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add (NULL, (yyvsp[0].s));
  }
#line 2523 "ppparse.c" /* yacc.c:1646  */
    break;

  case 94:
#line 1120 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add ((yyvsp[-1].l), (yyvsp[0].s));
  }
#line 2531 "ppparse.c" /* yacc.c:1646  */
    break;

  case 95:
#line 1127 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add (NULL, (yyvsp[0].s));
  }
#line 2539 "ppparse.c" /* yacc.c:1646  */
    break;

  case 96:
#line 1131 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add ((yyvsp[-2].l), " ");
	(yyval.l) = ppp_list_add ((yyval.l), "IN");
	(yyval.l) = ppp_list_add ((yyval.l), " ");
	(yyval.l) = ppp_list_add ((yyval.l), (yyvsp[0].s));
  }
#line 2550 "ppparse.c" /* yacc.c:1646  */
    break;

  case 97:
#line 1138 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add ((yyvsp[-2].l), " ");
	(yyval.l) = ppp_list_add ((yyval.l), "OF");
	(yyval.l) = ppp_list_add ((yyval.l), " ");
	(yyval.l) = ppp_list_add ((yyval.l), (yyvsp[0].s));
  }
#line 2561 "ppparse.c" /* yacc.c:1646  */
    break;

  case 98:
#line 1145 "ppparse.y" /* yacc.c:1646  */
    {
	struct cb_text_list *l;

	(yyval.l) = ppp_list_add ((yyvsp[-3].l), " ");
	(yyval.l) = ppp_list_add ((yyval.l), "(");
	(yyvsp[-1].l) = ppp_list_add ((yyvsp[-1].l), ")");
	for (l = (yyval.l); l->next; l = l->next) {
		;
	}
	l->next = (yyvsp[-1].l);
  }
#line 2577 "ppparse.c" /* yacc.c:1646  */
    break;

  case 99:
#line 1160 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add (NULL, (yyvsp[0].s));
  }
#line 2585 "ppparse.c" /* yacc.c:1646  */
    break;

  case 100:
#line 1164 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.l) = ppp_list_add ((yyvsp[-1].l), " ");
	(yyval.l) = ppp_list_add ((yyval.l), (yyvsp[0].s));
  }
#line 2594 "ppparse.c" /* yacc.c:1646  */
    break;

  case 101:
#line 1172 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = CB_REPLACE_LEADING;
  }
#line 2602 "ppparse.c" /* yacc.c:1646  */
    break;

  case 102:
#line 1176 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = CB_REPLACE_TRAILING;
  }
#line 2610 "ppparse.c" /* yacc.c:1646  */
    break;

  case 103:
#line 1185 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 0;
  }
#line 2618 "ppparse.c" /* yacc.c:1646  */
    break;

  case 104:
#line 1189 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 1U;
  }
#line 2626 "ppparse.c" /* yacc.c:1646  */
    break;

  case 105:
#line 1196 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 0;
  }
#line 2634 "ppparse.c" /* yacc.c:1646  */
    break;

  case 106:
#line 1200 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 1U;
  }
#line 2642 "ppparse.c" /* yacc.c:1646  */
    break;

  case 107:
#line 1207 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 0;
  }
#line 2650 "ppparse.c" /* yacc.c:1646  */
    break;

  case 108:
#line 1211 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 1U;
  }
#line 2658 "ppparse.c" /* yacc.c:1646  */
    break;

  case 109:
#line 1218 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 0;
  }
#line 2666 "ppparse.c" /* yacc.c:1646  */
    break;

  case 110:
#line 1222 "ppparse.y" /* yacc.c:1646  */
    {
	(yyval.ui) = 1U;
  }
#line 2674 "ppparse.c" /* yacc.c:1646  */
    break;


#line 2678 "ppparse.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 1234 "ppparse.y" /* yacc.c:1906  */

