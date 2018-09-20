/*
  Copyright 2010-2018 David Anderson. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement
  or the like.  Any license provided herein, whether implied or
  otherwise, applies only to this software file.  Patent licenses, if
  any, provided herein do not apply to combinations of this program with
  other software, or any other product whatsoever.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write the Free Software Foundation, Inc., 51
  Franklin Street - Fifth Floor, Boston MA 02110-1301, USA.
*/

#include "globals.h"
#include "dwconf.h"
#include "dwgetopt.h"

#include "common.h"
#include "makename.h"
#include "uri.h"
#include "esb.h"                /* For flexible string buffer. */
#include "sanitized.h"
#include "tag_common.h"

#include "command_options.h"
#include "compiler_info.h"
#include <assert.h>

static const char *remove_quotes_pair(const char *text);
static char *special_program_name(char *n);
static void suppress_check_dwarf();

extern char *dwoptarg;

static const char *usage_text[] = {
"Usage: DwarfDump <options> <object file>",
"options:\t-a\tprint all .debug_* sections",
"\t\t-b\tprint abbrev section",
"\t\t-c\tprint loc section",
"\t\t-c<str>\tcheck only specific compiler objects",
"\t\t  \t  <str> is described by 'DW_AT_producer'. Examples:",
"\t\t  \t    -cg       check only GCC compiler objects",
"\t\t  \t    -cs       check only SNC compiler objects",
"\t\t  \t    -c'350.1' check only compiler objects with 350.1 in the CU name",
"\t\t-C\tactivate printing (with -i) of warnings about",
"\t\t\tcertain common extensions of DWARF.",
"\t\t-d\tdense: one line per entry (info section only)",
"\t\t-D\tdo not show offsets",  /* Do not show any offsets */
"\t\t-e\tellipsis: short names for tags, attrs etc.",
"\t\t-E[hliaprfoRstxd]\tprint object Header and/or section information",
"\t\t  \th=header,l=line,i=info,a=abbrev,p=pubnames,r=aranges,",
"\t\t  \tf=frames,o=loc,R=Ranges,s=strings,t=pubtypes,x=text,",
"\t\t  \td=default sections, same as -E and {liaprfoRstx}",
"\t\t-f\tprint dwarf frame section",
"\t\t-F\tprint gnu .eh_frame section",
"\t\t-g\t(use incomplete loclist support)",
"\t\t-G\tshow global die offsets",
"\t\t-h\tprint the dwarfdump help message (this options list) (",
"\t\t-H <num>\tlimit output to the first <num> major units",
"\t\t\t  example: to stop after <num> compilation units",
"\t\t-i\tprint info section",
"\t\t-I\tprint sections .gdb_index, .debug_cu_index, .debug_tu_index",
/* FIXME -kw is check macros */
"\t\t-k[abcdDeEfFgGilmMnrRsStu[f]x[e]y] check dwarf information",
"\t\t   a\tdo all checks",
"\t\t   b\tcheck abbreviations",     /* Check abbreviations */
"\t\t   c\texamine DWARF constants", /* Check for valid DWARF constants */
"\t\t   d\tshow check results",      /* Show check results */
"\t\t   D\tcheck duplicated attributes",  /* Duplicated attributes */
"\t\t   e\texamine attributes of pubnames",
"\t\t   E\texamine attributes encodings",  /* Attributes encoding */
"\t\t   f\texamine frame information (use with -f or -F)",
"\t\t   F\texamine integrity of files-lines attributes", /* Files-Lines integrity */
"\t\t   g\tcheck debug info gaps", /* Check for debug info gaps */
"\t\t   G\tprint only unique errors", /* Only print the unique errors */
"\t\t   i\tdisplay summary for all compilers", /* Summary all compilers */
"\t\t   l\tcheck location list (.debug_loc)",  /* Location list integrity */
"\t\t   m\tcheck ranges list (.debug_ranges)", /* Ranges list integrity */
"\t\t   M\tcheck ranges list (.debug_aranges)",/* Aranges list integrity */
"\t\t   n\texamine names in attributes",       /* Check for valid names */
"\t\t   r\texamine tag-attr relation",
"\t\t   R\tcheck forward references to DIEs (declarations)", /* Check DW_AT_specification references */
"\t\t   s\tperform checks in silent mode",
"\t\t   S\tcheck self references to DIEs",
"\t\t   t\texamine tag-tag relations",
#ifdef HAVE_USAGE_TAG_ATTR
"\t\t   u\tprint tag-tree and tag-attribute usage (basic format)",
"\t\t   uf\tprint tag-tree and tag-attribute usage (full format)",
#endif /* HAVE_USAGE_TAG_ATTR */
"\t\t   x\tbasic frames check (.eh_frame, .debug_frame)",
"\t\t   xe\textensive frames check (.eh_frame, .debug_frame)",
"\t\t   y\texamine type info",
"\t\t\tUnless -C option given certain common tag-attr and tag-tag",
"\t\t\textensions are assumed to be ok (not reported).",
"\t\t-l[s]\tprint line section",
"\t\t   s\tdo not print <pc> address",
"\t\t-m\tprint macinfo section",
"\t\t-M\tprint the form name for each attribute",
"\t\t-n\tsuppress frame information function name lookup",
"\t\t  \t(when printing frame information from multi-gigabyte",
"\t\t  \tobject files this option may save significant time).",
"\t\t-N\tprint ranges section",
"\t\t-O file=<path>\tname the output file",
"\t\t-o[liaprfoR]\tprint relocation info",
"\t\t  \tl=line,i=info,a=abbrev,p=pubnames,r=aranges,f=frames,o=loc,R=Ranges",
"\t\t-p\tprint pubnames section",
"\t\t--print-debug-names\tprint details from the .debug_names section",
"\t\t--print-str-offsets\tprint details from the .debug_str_offsets section",
"\t\t-P\tprint list of compile units per producer", /* List of CUs per compiler */
"\t\t-Q\tsuppress printing section data",
"\t\t-r\tprint aranges section",
"\t\t-R\tPrint frame register names as r33 etc",
"\t\t  \t    and allow up to 1200 registers.",
"\t\t  \t    Print using a 'generic' register set.",
"\t\t-s\tprint string section",
"\t\t-S[v] <option>=<text>\tsearch for <text> in attributes",
"\t\t  \tv\tprint number of occurrences",
"\t\t  \twith <option>:",
"\t\t  \t-S any=<text>\tany <text>",
"\t\t  \t-S match=<text>\tmatching <text>",
#ifdef HAVE_REGEX
"\t\t  \t-S regex=<text>\tuse regular expression matching",
#endif
"\t\t  \t (only one -S option allowed, any= and regex= ",
"\t\t  \t  only usable if the functions required are ",
"\t\t  \t  found at configure time)",
"\t\t-t[afv] static: ",
"\t\t   a\tprint both sections",
"\t\t   f\tprint static func section",
"\t\t   v\tprint static var section",
"\t\t-u<file> print sections only for specified file",
"\t\t-v\tverbose: show more information",
"\t\t-vv verbose: show even more information",
"\t\t-V print version information",
"\t\t-x abi=<abi>\tname abi in dwarfdump.conf",
"\t\t-x groupnumber=<n>\tgroupnumber to print",
"\t\t-x name=<path>\tname dwarfdump.conf",
"\t\t-x tied=<tiedpath>\tname an associated object file (Split DWARF)",
#if 0
"\t\t-x nosanitizestrings\tLet bogus string characters come thru printf",
#endif
"\t\t-w\tprint weakname section",
"\t\t-W\tprint parent and children tree (wide format) with the -S option",
"\t\t-Wp\tprint parent tree (wide format) with the -S option",
"\t\t-Wc\tprint children tree (wide format) with the -S option",
"\t\t-y\tprint type section",
"",
0
};

static const char *usage_debug_text[] = {
"Usage: DwarfDump <debug_options>",
"options:\t-0\tprint this information",
"\t\t-1\tDump RangesInfo Table",
"\t\t-2\tDump Location (.debug_loc) Info",
"\t\t-3\tDump Ranges (.debug_ranges) Info",
"\t\t-4\tDump Linkonce Table",
"\t\t-5\tDump Visited Info",
""
};

/*  These configure items are for the
    frame data.  We're flexible in
    the path to dwarfdump.conf .
    The HOME strings here are transformed in
    dwconf.c to reference the environment
    variable $HOME .

    As of August 2018 CONFPREFIX is always set as it
    comes from autoconf --prefix, aka  $prefix
    which defaults to /usr/local

    The install puts the .conf file in
    CONFPREFIX/dwarfdump/
*/
static char *config_file_defaults[] = {
    "dwarfdump.conf",
    "./dwarfdump.conf",
    "HOME/.dwarfdump.conf",
    "HOME/dwarfdump.conf",
#ifdef CONFPREFIX
/* See Makefile.am dwarfdump_CFLAGS. This prefix
    is the --prefix option (defaults to /usr/local
    and Makefile.am adds /share/dwarfdump ) */
/* We need 2 levels of macro to get the name turned into
   the string we want. */
#define STR2(s) # s
#define STR(s)  STR2(s)
    STR(CONFPREFIX) "/dwarfdump.conf",
#else
    /*  This no longer used as of August 2018. */
    "/usr/lib/dwarfdump.conf",
#endif
    0
};
static const char *config_file_abi = 0;

/* Do printing of most sections.
   Do not do detailed checking.
*/
static void
do_all(void)
{
    glflags.gf_frame_flag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag =  TRUE; /* .debug_types */
    glflags.gf_line_flag = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_macinfo_flag = TRUE;
    glflags.gf_macro_flag = TRUE;
    glflags.gf_aranges_flag = TRUE;
    /*  Do not do
        glflags.gf_loc_flag = TRUE
        glflags.gf_abbrev_flag = TRUE;
        glflags.gf_ranges_flag = TRUE;
        because nothing in
        the DWARF spec guarantees the sections are free of random bytes
        in areas not referenced by .debug_info */
    glflags.gf_string_flag = TRUE;
    /*  Do not do
        glflags.gf_reloc_flag = TRUE;
        as print_relocs makes no sense for non-elf dwarfdump users.  */
    glflags.gf_static_func_flag = TRUE; /* SGI only*/
    glflags.gf_static_var_flag = TRUE; /* SGI only*/
    glflags.gf_pubtypes_flag = TRUE;  /* both SGI typenames and dwarf_pubtypes. */
    glflags.gf_weakname_flag = TRUE; /* SGI only*/
    glflags.gf_header_flag = TRUE; /* Dump header info */
    glflags.gf_debug_names_flag = TRUE;
}

static int
get_number_value(char *v_in,long int *v_out)
{
    long int v= 0;
    size_t len = strlen(v_in);
    char *endptr = 0;

    if (len < 1) {
        return DW_DLV_ERROR;
    }
    v = strtol(v_in,&endptr,10);
    if (endptr == v_in) {
        return DW_DLV_NO_ENTRY;
    }
    if (*endptr != '\0') {
        return DW_DLV_ERROR;
    }
    *v_out = v;
    return DW_DLV_OK;
}

static void suppress_print_dwarf()
{
    glflags.gf_do_print_dwarf = FALSE;
    glflags.gf_do_check_dwarf = TRUE;
}

/* Remove matching leading/trailing quotes.
   Does not alter the passed in string.
   If quotes removed does a makename on a modified string. */
static const char *
remove_quotes_pair(const char *text)
{
    static char single_quote = '\'';
    static char double_quote = '\"';
    char quote = 0;
    const char *p = text;
    int len = strlen(text);

    if (len < 2) {
        return p;
    }

    /* Compare first character with ' or " */
    if (p[0] == single_quote) {
        quote = single_quote;
    } else {
        if (p[0] == double_quote) {
            quote = double_quote;
        }
        else {
            return p;
        }
    }
    {
        if (p[len - 1] == quote) {
            char *altered = calloc(1,len+1);
            const char *str2 = 0;
            strcpy(altered,p+1);
            altered[len - 2] = '\0';
            str2 =  makename(altered);
            free(altered);
            return str2;
        }
    }
    return p;
}

/*  By trimming a /dwarfdump.O
    down to /dwarfdump  (keeping any prefix
    or suffix)
    we can avoid a sed command in
    regressiontests/DWARFTEST.sh
    and save 12 minutes run time of a regression
    test.

    The effect is, when nothing has changed in the
    normal output, that the program_name matches too.
    Because we don't want a different name of dwarfdump
    to cause a mismatch.  */
static char *
special_program_name(char *n)
{
    char * mp = "/dwarfdump.O";
    char * revstr = "/dwarfdump";
    char *cp = n;
    size_t mslen = strlen(mp);

    for(  ; *cp; ++cp ) {
        if (*cp == *mp) {
            if(!strncmp(cp,mp,mslen)){
                esb_append(glflags.newprogname,revstr);
                cp += mslen-1;
            } else {
                esb_appendn(glflags.newprogname,cp,1);
            }
        } else {
            esb_appendn(glflags.newprogname,cp,1);
        }
    }
    return esb_get_string(glflags.newprogname);
}

static void suppress_check_dwarf()
{
    glflags.gf_do_print_dwarf = TRUE;
    if (glflags.gf_do_check_dwarf) {
        printf("Warning: check flag turned off, "
            "checking and printing are separate.\n");
    }
    glflags.gf_do_check_dwarf = FALSE;
    set_checks_off();
}

/*  The strings whose pointers are returned here
    from makename are never destructed, but
    that is ok since there are only about 10 created at most.  */
const char *
do_uri_translation(const char *s,const char *context)
{
    struct esb_s str;
    char *finalstr = 0;
    if (!glflags.gf_uri_options_translation) {
        return makename(s);
    }
    esb_constructor(&str);
    translate_from_uri(s,&str);
    if (glflags.gf_do_print_uri_in_input) {
        if (strcmp(s,esb_get_string(&str))) {
            printf("Uri Translation on option %s\n",context);
            printf("    \'%s\'\n",s);
            printf("    \'%s\'\n",esb_get_string(&str));
        }
    }
    finalstr = makename(esb_get_string(&str));
    esb_destructor(&str);
    return finalstr;
}

static void option_1000(void);
static void option_1001(void);
static void option_trace(void);

static void option_a(void);
static void option_b(void);
static void option_c(void);
static void option_cs(void);
static void option_cg(void);
static void option_C(void);
static void option_d(void);
static void option_D(void);
static void option_e(void);
static void option_E(void);
static void option_Ea(void);
static void option_Ed(void);
static void option_Ef(void);
static void option_Eh(void);
static void option_Ei(void);
static void option_EI(void);
static void option_El(void);
static void option_Em(void);
static void option_Eo(void);
static void option_Ep(void);
static void option_Er(void);
static void option_ER(void);
static void option_Es(void);
static void option_Et(void);
static void option_Ex(void);
static void option_f(void);
static void option_F(void);
static void option_g(void);
static void option_G(void);
static void option_h(void);
static void option_H(void);
static void option_i(void);
static void option_I(void);
static void option_k(void);
static void option_ka(void);
static void option_kb(void);
static void option_kc(void);
static void option_kd(void);
static void option_kD(void);
static void option_ke(void);
static void option_kE(void);
static void option_kf(void);
static void option_kF(void);
static void option_kg(void);
static void option_kG(void);
static void option_ki(void);
static void option_kl(void);
static void option_km(void);
static void option_kM(void);
static void option_kn(void);
static void option_kr(void);
static void option_kR(void);
static void option_ks(void);
static void option_kS(void);
static void option_kt(void);
#ifdef HAVE_USAGE_TAG_ATTR
static void option_ku(void);
static void option_kuf(void);
#endif /* HAVE_USAGE_TAG_ATTR */
static void option_kw(void);
static void option_kx(void);
static void option_kxe(void);
static void option_ky(void);
static void option_l(void);
static void option_ls(void);
static void option_m(void);
static void option_M(void);
static void option_n(void);
static void option_N(void);
static void option_o(void);
static void option_oa(void);
static void option_of(void);
static void option_oi(void);
static void option_ol(void);
static void option_oo(void);
static void option_op(void);
static void option_or(void);
static void option_oR(void);
static void option_O(void);
static void option_p(void);
static void option_P(void);
static void option_q(void);
static void option_Q(void);
static void option_r(void);
static void option_R(void);
static void option_s(void);
static void option_S(void);
static void option_t(void);
static void option_ta(void);
static void option_tf(void);
static void option_tv(void);
static void option_u(void);
static void option_U(void);
static void option_v(void);
static void option_V(void);
static void option_w(void);
static void option_W(void);
static void option_Wc(void);
static void option_Wp(void);
static void option_x(void);
static void option_y(void);
static void option_z(void);

/*  Extracted from 'process_args', as they are used by option handlers. */
static boolean usage_error = FALSE;
static int option = 0;

/* process arguments and return object filename */
const char *
process_args(int argc, char *argv[])
{
    int longindex = 0;
    static struct dwoption  longopts[] =  {
        {"print-str-offsets",  dwno_argument,  0,1000},
        {"print-debug-names",  dwno_argument,  0,1001},
        {0,0,0,0}
    };

    glflags.program_name = special_program_name(argv[0]);
    glflags.program_fullname = argv[0];

    suppress_check_dwarf();
    if (argv[1] != NULL && argv[1][0] != '-') {
        do_all();
    }
    glflags.gf_section_groups_flag = TRUE;

    /* j unused */
    while ((option = dwgetopt_long(argc, argv,
        "#:abc::CdDeE::fFgGhH:iIk:l::mMnNo::O:pPqQrRsS:t:u:UvVwW::x:yz",
        longopts,&longindex)) != EOF) {

        switch (option) {
        case 1000: option_1000(); break;
        case 1001: option_1001(); break;
        case '#': option_trace(); break;
        case 'a': option_a();     break;
        case 'b': option_b();     break;
        case 'c': option_c();     break;
        case 'C': option_C();     break;
        case 'd': option_d();     break;
        case 'D': option_D();     break;
        case 'e': option_e();     break;
        case 'E': option_E();     break;
        case 'f': option_f();     break;
        case 'F': option_F();     break;
        case 'g': option_g();     break;
        case 'G': option_G();     break;
        case 'h': option_h();     break;
        case 'H': option_H();     break;
        case 'i': option_i();     break;
        case 'I': option_I();     break;
        case 'k': option_k();     break;
        case 'l': option_l();     break;
        case 'm': option_m();     break;
        case 'M': option_M();     break;
        case 'n': option_n();     break;
        case 'N': option_N();     break;
        case 'o': option_o();     break;
        case 'O': option_O();     break;
        case 'p': option_p();     break;
        case 'P': option_P();     break;
        case 'q': option_q();     break;
        case 'Q': option_Q();     break;
        case 'r': option_r();     break;
        case 'R': option_R();     break;
        case 's': option_s();     break;
        case 'S': option_S();     break;
        case 't': option_t();     break;
        case 'u': option_u();     break;
        case 'U': option_U();     break;
        case 'v': option_v();     break;
        case 'V': option_V();     break;
        case 'w': option_w();     break;
        case 'W': option_W();     break;
        case 'x': option_x();     break;
        case 'y': option_y();     break;
        case 'z': option_z();     break;
        default: usage_error = TRUE; break;
        }
    }

    init_conf_file_data(glflags.config_file_data);
    if (config_file_abi && glflags.gf_generic_1200_regs) {
        printf("Specifying both -R and -x abi= is not allowed. Use one "
            "or the other.  -x abi= ignored.\n");
        config_file_abi = FALSE;
    }
    if (glflags.gf_generic_1200_regs) {
        init_generic_config_1200_regs(glflags.config_file_data);
    }
    if (config_file_abi &&
        (glflags.gf_frame_flag || glflags.gf_eh_frame_flag)) {
        int res = 0;
        res = find_conf_file_and_read_config(
            esb_get_string(glflags.config_file_path),
            config_file_abi,
            config_file_defaults,
            glflags.config_file_data);

        if (res > 0) {
            printf
                ("Frame not configured due to error(s). Giving up.\n");
            glflags.gf_eh_frame_flag = FALSE;
            glflags.gf_frame_flag = FALSE;
        }
    }
    if (usage_error ) {
        printf("%s option error.\n",glflags.program_name);
        printf("To see the options list: %s -h\n",glflags.program_name);
        exit(FAILED);
    }
    if (dwoptind != (argc - 1)) {
        printf("No object file name provided to %s\n",glflags.program_name);
        printf("To see the options list: %s -h\n",glflags.program_name);
        exit(FAILED);
    }
    /*  FIXME: it seems silly to be printing section names
        where the section does not exist in the object file.
        However we continue the long-standard practice
        of printing such by default in most cases. For now. */
    if (glflags.group_number == DW_GROUPNUMBER_DWO) {
        /*  For split-dwarf/DWO some sections make no sense.
            This prevents printing of meaningless headers where no
            data can exist. */
        glflags.gf_pubnames_flag = FALSE;
        glflags.gf_eh_frame_flag = FALSE;
        glflags.gf_frame_flag    = FALSE;
        glflags.gf_macinfo_flag  = FALSE;
        glflags.gf_aranges_flag  = FALSE;
        glflags.gf_ranges_flag   = FALSE;
        glflags.gf_static_func_flag = FALSE;
        glflags.gf_static_var_flag = FALSE;
        glflags.gf_weakname_flag = FALSE;
    }
    if (glflags.group_number > DW_GROUPNUMBER_BASE) {
        /* These no longer apply, no one uses. */
        glflags.gf_static_func_flag = FALSE;
        glflags.gf_static_var_flag = FALSE;
        glflags.gf_weakname_flag = FALSE;
        glflags.gf_pubnames_flag = FALSE;
    }

    if (glflags.gf_do_check_dwarf) {
        /* Reduce verbosity when checking (checking means checking-only). */
        glflags.verbose = 1;
    }
    return do_uri_translation(argv[dwoptind],"file-to-process");
}

/*  Handlers for the command line options. */

void option_1000(void)
{
    assert(option == 1000);

    glflags.gf_print_str_offsets = TRUE;
}

void option_1001(void)
{
    assert(option == 1001);

    glflags.gf_debug_names_flag = TRUE;
}

/*  option '-#' */
void option_trace(void)
{
    assert(option == '#');

    int nTraceLevel =  atoi(dwoptarg);
    if (nTraceLevel >= 0 && nTraceLevel <= MAX_TRACE_LEVEL) {
        glflags.nTrace[nTraceLevel] = 1;
    }
    /* Display dwarfdump debug options. */
    if (dump_options) {
        print_usage_message(glflags.program_name,usage_debug_text);
        exit(OKAY);
    }
}

/*  option '-a' */
void option_a(void)
{
    assert(option == 'a');

    suppress_check_dwarf();
    do_all();
}

/*  option '-b' */
void option_b(void)
{
    assert(option == 'b');

    glflags.gf_abbrev_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-c' */
void option_c(void)
{
    assert(option == 'c');

    /* Specify compiler name. */
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 's': option_cs(); break;
        case 'g': option_cg(); break;
        default:
            /*  Assume a compiler version to check,
                most likely a substring of a compiler name.  */
            if (!record_producer(dwoptarg)) {
                fprintf(stderr, "Compiler table max %d exceeded, "
                    "limiting the tracked compilers to %d\n",
                    COMPILER_TABLE_MAX,COMPILER_TABLE_MAX);
            }
            break;
        }
    } else {
        glflags.gf_loc_flag = TRUE;
        suppress_check_dwarf();
    }
}

/*  option '-cs' */
void option_cs(void)
{
    /* -cs : Check SNC compiler */
    glflags.gf_check_snc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  option '-cg' */
void option_cg(void)
{
    /* -cg : Check GCC compiler */
    glflags.gf_check_gcc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  option '-C' */
void option_C(void)
{
    assert(option == 'C');

    glflags.gf_suppress_check_extensions_tables = TRUE;
}

/*  option '-d' */
void option_d(void)
{
    assert(option == 'd');

    glflags.gf_do_print_dwarf = TRUE;
    /*  This is sort of useless unless printing,
        but harmless, so we do not insist we
        are printing with suppress_check_dwarf(). */
    glflags.dense = TRUE;
}

/*  option '-D' */
void option_D(void)
{
    assert(option == 'D');

    /* Do not emit offset in output */
    glflags.gf_display_offsets = FALSE;
}

/*  option '-e' */
void option_e(void)
{
    assert(option == 'e');

    suppress_check_dwarf();
    glflags.ellipsis = TRUE;
}

/*  option '-E' */
void option_E(void)
{
    assert(option == 'E');

    /* Object Header information (but maybe really print) */
    glflags.gf_header_flag = TRUE;
    /* Selected printing of section info */
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 'a': option_Ea(); break;
        case 'd': option_Ed(); break;
        case 'f': option_Ef(); break;
        case 'h': option_Eh(); break;
        case 'i': option_Ei(); break;
        case 'I': option_EI(); break;
        case 'l': option_El(); break;
        case 'm': option_Em(); break;
        case 'o': option_Eo(); break;
        case 'p': option_Ep(); break;
        case 'r': option_Er(); break;
        case 'R': option_ER(); break;
        case 's': option_Es(); break;
        case 't': option_Et(); break;
        case 'x': option_Ex(); break;
        default: usage_error = TRUE; break;
        }
    } else {
        /* Display header and all sections info */
        set_all_sections_on();
    }
}

/*  option '-Ea' */
void option_Ea(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_ABBREV);
}

/*  option '-Ed' */
void option_Ed(void)
{
    /* case 'd', use the default section set */
    set_all_section_defaults();
}

/*  option '-Ef' */
void option_Ef(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_FRAME);
}

/*  option '-Eh' */
void option_Eh(void)
{
    enable_section_map_entry(DW_HDR_HEADER);
}

/*  option '-Ei' */
void option_Ei(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_INFO);
    enable_section_map_entry(DW_HDR_DEBUG_TYPES);
}

/*  option '-EI' */
void option_EI(void)
{
    enable_section_map_entry(DW_HDR_GDB_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_CU_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_TU_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_NAMES);
}

/*  option '-El' */
void option_El(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_LINE);
}

/*  option '-Em' */
void option_Em(void)
{
    /*  For both old macinfo and dwarf5  macro */
    enable_section_map_entry(DW_HDR_DEBUG_MACINFO);
}

/*  option '-Eo' */
void option_Eo(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_LOC);
}

/*  option '-Ep' */
void option_Ep(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_PUBNAMES);
}

/*  option '-Er' */
void option_Er(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_ARANGES);
}

/*  option '-ER' */
void option_ER(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_RANGES);
    enable_section_map_entry(DW_HDR_DEBUG_RNGLISTS);
}

/*  option '-Es' */
void option_Es(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_STR);
}

/*  option '-Et' */
void option_Et(void)
{
    enable_section_map_entry(DW_HDR_DEBUG_PUBTYPES);
}

/*  option '-Ex' */
void option_Ex(void)
{
    enable_section_map_entry(DW_HDR_TEXT);
}

/*  option '-f' */
void option_f(void)
{
    assert(option == 'f');

    glflags.gf_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-F' */
void option_F(void)
{
    assert(option == 'F');

    glflags.gf_eh_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-g' */
void option_g(void)
{
    assert(option == 'g');

    glflags.gf_use_old_dwarf_loclist = TRUE;
    /*info_flag = TRUE;  removed  from -g. Nov 2015 */
    suppress_check_dwarf();
}

/*  option '-G' */
void option_G(void)
{
    assert(option == 'G');

    glflags.gf_show_global_offsets = TRUE;
}

/*  option '-h' */
void option_h(void)
{
    assert(option == 'h');

    print_usage_message(glflags.program_name,usage_text);
    exit(OKAY);
}

/*  option '-H' */
void option_H(void)
{
    assert(option == 'H');

    int break_val =  atoi(dwoptarg);
    if (break_val > 0) {
        glflags.break_after_n_units = break_val;
    }
}

/*  option '-i' */
void option_i(void)
{
    assert(option == 'i');

    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-I' */
void option_I(void)
{
    assert(option == 'I');

    glflags.gf_gdbindex_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-k' */
void option_k(void)
{
    assert(option == 'k');

    suppress_print_dwarf();
    switch (dwoptarg[0]) {
    case 'a': option_ka(); break;
    case 'b': option_kb(); break;
    case 'c': option_kc(); break;
    case 'd': option_kd(); break;
    case 'D': option_kD(); break;
    case 'e': option_ke(); break;
    case 'E': option_kE(); break;
    case 'f': option_kf(); break;
    case 'F': option_kF(); break;
    case 'g': option_kg(); break;
    case 'G': option_kG(); break;
    case 'i': option_ki(); break;
    case 'l': option_kl(); break;
    case 'm': option_km(); break;
    case 'M': option_kM(); break;
    case 'n': option_kn(); break;
    case 'r': option_kr(); break;
    case 'R': option_kR(); break;
    case 's': option_ks(); break;
    case 'S': option_kS(); break;
    case 't': option_kt(); break;
#ifdef HAVE_USAGE_TAG_ATTR
    case 'u': option_ku(); break;
#endif /* HAVE_USAGE_TAG_ATTR */
    case 'w': option_kw(); break;
    case 'x': option_kx(); break;
    case 'y': option_ky(); break;
    default: usage_error = TRUE; break;
    }
}

/*  option '-ka' */
void option_ka(void)
{
    glflags.gf_check_pubname_attr = TRUE;
    glflags.gf_check_attr_tag = TRUE;
    glflags.gf_check_tag_tree = TRUE;
    glflags.gf_check_type_offset = TRUE;
    glflags.gf_check_names = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_gdbindex_flag = TRUE;
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_check_macros = TRUE;
    glflags.gf_check_frames = TRUE;
    glflags.gf_check_frames_extended = FALSE;
    glflags.gf_check_locations = TRUE;
    glflags.gf_frame_flag = TRUE;
    glflags.gf_eh_frame_flag = TRUE;
    glflags.gf_check_ranges = TRUE;
    glflags.gf_check_lines = TRUE;
    glflags.gf_check_fdes = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_aranges = TRUE;
    glflags.gf_aranges_flag = TRUE;  /* Aranges section */
    glflags.gf_check_abbreviations = TRUE;
    glflags.gf_check_dwarf_constants = TRUE;
    glflags.gf_check_di_gaps = TRUE;
    glflags.gf_check_forward_decl = TRUE;
    glflags.gf_check_self_references = TRUE;
    glflags.gf_check_attr_encoding = TRUE;
    glflags.gf_print_usage_tag_attr = TRUE;
    glflags.gf_check_duplicated_attributes = TRUE;
}

/*  option '-kb' */
void option_kb(void)
{
    /* Abbreviations */
    glflags.gf_check_abbreviations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  option '-kc' */
void option_kc(void)
{
    /* DWARF constants */
    glflags.gf_check_dwarf_constants = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-kd' */
void option_kd(void)
{
    /* Display check results */
    glflags.gf_check_show_results = TRUE;
}

/*  option '-kD' */
void option_kD(void)
{
    /* Check duplicated attributes */
    glflags.gf_check_duplicated_attributes = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  option '-ke' */
void option_ke(void)
{
    glflags.gf_check_pubname_attr = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  option '-kE' */
void option_kE(void)
{
    /* Attributes encoding usage */
    glflags.gf_check_attr_encoding = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-kf' */
void option_kf(void)
{
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  option '-kF' */
void option_kF(void)
{
    /* files-lines */
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_check_lines = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-kg' */
void option_kg(void)
{
    /* Check debug info gaps */
    glflags.gf_check_di_gaps = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-kG' */
void option_kG(void)
{
    /* Print just global (unique) errors */
    glflags.gf_print_unique_errors = TRUE;
}

/*  option '-ki' */
void option_ki(void)
{
    /* Summary for each compiler */
    glflags.gf_print_summary_all = TRUE;
}

/*  option '-kl' */
void option_kl(void)
{
    /* Locations list */
    glflags.gf_check_locations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_loc_flag = TRUE;
}

/*  option '-km' */
void option_km(void)
{
    /* Ranges */
    glflags.gf_check_ranges = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-kM' */
void option_kM(void)
{
    /* Aranges */
    glflags.gf_check_aranges = TRUE;
    glflags.gf_aranges_flag = TRUE;
}

/*  option '-kn' */
void option_kn(void)
{
    /* invalid names */
    glflags.gf_check_names = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-kr' */
void option_kr(void)
{
    glflags.gf_check_attr_tag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
}

/*  option '-kR' */
void option_kR(void)
{
    /* forward declarations in DW_AT_specification */
    glflags.gf_check_forward_decl = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-ks' */
void option_ks(void)
{
    /* Check verbose mode */
    glflags.gf_check_verbose_mode = FALSE;
}

/*  option '-kS' */
void option_kS(void)
{
    /*  self references in:
        DW_AT_specification, DW_AT_type, DW_AT_abstract_origin */
    glflags.gf_check_self_references = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  option '-kt' */
void option_kt(void)
{
    glflags.gf_check_tag_tree = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

#ifdef HAVE_USAGE_TAG_ATTR
/*  option '-ku' */
void option_ku(void)
{
    /* Tag-Tree and Tag-Attr usage */
    glflags.gf_print_usage_tag_attr = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'f': option_kuf(); break;
        default: usage_error = TRUE; break;
        }
    }
}

/*  option '-kuf' */
void option_kuf(void)
{
    /* -kuf : Full report */
    glflags.gf_print_usage_tag_attr_full = TRUE;
}
#endif /* HAVE_USAGE_TAG_ATTR */

/*  option '-kw' */
void option_kw(void)
{
    glflags.gf_check_macros = TRUE;
    glflags.gf_macro_flag = TRUE;
    glflags.gf_macinfo_flag = TRUE;
}

/*  option '-kx' */
void option_kx(void)
{
    /* Frames check */
    glflags.gf_check_frames = TRUE;
    glflags.gf_frame_flag = TRUE;
    glflags.gf_eh_frame_flag = TRUE;
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'e': option_kxe(); break;
        default: usage_error = TRUE; break;
        }
    }
}

/*  option '-kxe' */
void option_kxe(void)
{
    /* -xe : Extended frames check */
    glflags.gf_check_frames = FALSE;
    glflags.gf_check_frames_extended = TRUE;
}

/*  option '-ky' */
void option_ky(void)
{
    glflags.gf_check_type_offset = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_pubtypes_flag = TRUE;
    glflags.gf_check_ranges = TRUE;
    glflags.gf_check_aranges = TRUE;
}

/*  option '-l' */
void option_l(void)
{
    assert(option == 'l');

    glflags.gf_line_flag = TRUE;
    suppress_check_dwarf();
    /* Enable to suppress offsets printing */
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 's': option_ls(); break;
        default: usage_error = TRUE; break;
        }
    }
}

/*  option '-ls' */
void option_ls(void)
{
  /* -ls : suppress <pc> addresses */
  glflags.gf_line_print_pc = FALSE;
}

/*  option '-m' */
void option_m(void)
{
    assert(option == 'm');

    glflags.gf_macinfo_flag = TRUE; /* DWARF2,3,4 */
    glflags.gf_macro_flag   = TRUE; /* DWARF5 */
    suppress_check_dwarf();
}

/*  option '-M' */
void option_M(void)
{
    assert(option == 'M');

    glflags.show_form_used =  TRUE;
}

/*  option '-n' */
void option_n(void)
{
    assert(option == 'n');

    glflags.gf_suppress_nested_name_search = TRUE;
}

/*  option '-N' */
void option_N(void)
{
    assert(option == 'N');

    glflags.gf_ranges_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-o' */
void option_o(void)
{
    assert(option == 'o');

    glflags.gf_reloc_flag = TRUE;
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 'a': option_oa(); break;
        case 'i': option_oi(); break;
        case 'l': option_ol(); break;
        case 'p': option_op(); break;
        case 'r': option_or(); break;
        case 'f': option_of(); break;
        case 'o': option_oo(); break;
        case 'R': option_oR(); break;
        default: usage_error = TRUE; break;
        }
    } else {
        set_all_reloc_sections_on();
    }
}

/*  option '-oa' */
void option_oa(void)
{
    /*  Case a has no effect, no relocations can point out
        of the abbrev section. */
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_ABBREV);
}

/*  option '-of' */
void option_of(void)
{
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_FRAME);
}

/*  option '-oi' */
void option_oi(void)
{
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_INFO);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_TYPES);
}

/*  option '-ol' */
void option_ol(void)
{
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LINE);
}

/*  option '-oo' */
void option_oo(void)
{
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LOC);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LOCLISTS);
}

/*  option '-op' */
void option_op(void)
{
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_PUBNAMES);
}

/*  option '-or' */
void option_or(void)
{
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_ARANGES);
}

/*  option '-oR' */
void option_oR(void)
{
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_RANGES);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_RNGLISTS);
}

/*  option '-O' */
void option_O(void)
{
    assert(option == 'O');

    /* Output filename */
    const char *path = 0;
    /*  -O name=<filename> */
    usage_error = TRUE;
    if (strncmp(dwoptarg,"file=",5) == 0) {
        path = do_uri_translation(&dwoptarg[5],"-O file=");
        if (strlen(path) > 0) {
            usage_error = FALSE;
            glflags.output_file = path;
        }
    }
}

/*  option '-p' */
void option_p(void)
{
    assert(option == 'p');

    glflags.gf_pubnames_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-P' */
void option_P(void)
{
    assert(option == 'P');

    /* List of CUs per compiler */
    glflags.gf_producer_children_flag = TRUE;
}

/*  option '-q' */
void option_q(void)
{
    assert(option == 'q');

    /* Suppress uri-did-transate notification */
    glflags.gf_do_print_uri_in_input = FALSE;
}

/*  option '-Q' */
void option_Q(void)
{
    assert(option == 'Q');

    /* Q suppresses section data printing. */
    glflags.gf_do_print_dwarf = FALSE;
}

/*  option '-r' */
void option_r(void)
{
    assert(option == 'r');

    glflags.gf_aranges_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-R' */
void option_R(void)
{
    assert(option == 'R');

    glflags.gf_generic_1200_regs = TRUE;
}

/*  option '-s' */
void option_s(void)
{
    assert(option == 's');

    glflags.gf_string_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-S' */
void option_S(void)
{
    assert(option == 'S');

    /* -S option: strings for 'any' and 'match' */
    const char *tempstr = 0;
    boolean serr = TRUE;
    glflags.gf_search_is_on = TRUE;
    /* 'v' option, to print number of occurrences */
    /* -S[v]match|any|regex=text*/
    if (dwoptarg[0] == 'v') {
        ++dwoptarg;
        glflags.gf_search_print_results = TRUE;
    }
    /* -S match=<text>*/
    if (strncmp(dwoptarg,"match=",6) == 0) {
        glflags.search_match_text = makename(&dwoptarg[6]);
        tempstr = remove_quotes_pair(glflags.search_match_text);
        glflags.search_match_text =
            do_uri_translation(tempstr, "-S match=");
        if (strlen(glflags.search_match_text) > 0) {
            serr = FALSE;
        }
    }
    else {
        /* -S any=<text>*/
        if (strncmp(dwoptarg,"any=",4) == 0) {
            glflags.search_any_text = makename(&dwoptarg[4]);
            tempstr = remove_quotes_pair(glflags.search_any_text);
            glflags.search_any_text =
                do_uri_translation(tempstr,"-S any=");
            if (strlen(glflags.search_any_text) > 0) {
                serr = FALSE;
            }
        }
#ifdef HAVE_REGEX
        else {
            /* -S regex=<regular expression>*/
            if (strncmp(dwoptarg,"regex=",6) == 0) {
                glflags.search_regex_text = makename(&dwoptarg[6]);
                tempstr = remove_quotes_pair(
                    glflags.search_regex_text);
                glflags.search_regex_text =
                    do_uri_translation(tempstr,
                    "-S regex=");
                if (strlen(glflags.search_regex_text) > 0) {
                    if (regcomp(glflags.search_re,
                        glflags.search_regex_text,
                        REG_EXTENDED)) {
                        fprintf(stderr,
                            "regcomp: unable to compile %s\n",
                            glflags.search_regex_text);
                    }
                    else {
                        serr = FALSE;
                    }
                }
            }
        }
#endif /* HAVE_REGEX */
    }
    if (serr) {
        fprintf(stderr,
            "-S any=<text> or -S match=<text> or"
            " -S regex=<text>\n");
        fprintf(stderr, "is allowed, not -S %s\n",dwoptarg);
        usage_error = TRUE;
    }
}

/*  option '-t' */
void option_t(void)
{
    assert(option == 't');

    switch (dwoptarg[0]) {
    case 'a': option_ta(); break;
    case 'f': option_tf(); break;
    case 'v': option_tv(); break;
    default: usage_error = TRUE; break;
    }
}

/*  option '-ta' */
void option_ta(void)
{
    /* all */
    glflags.gf_static_func_flag =  TRUE;
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-tf' */
void option_tf(void)
{
    /* .debug_static_func */
    glflags.gf_static_func_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-tv' */
void option_tv(void)
{
    /* .debug_static_var */
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-u' */
void option_u(void)
{
    assert(option == 'u');

    /* compile unit */
    const char *tstr = 0;
    glflags.gf_cu_name_flag = TRUE;
    tstr = do_uri_translation(dwoptarg,"-u<cu name>");
    esb_append(glflags.cu_name,tstr);
}

/*  option '-U' */
void option_U(void)
{
    assert(option == 'U');

    glflags.gf_uri_options_translation = FALSE;
}

/*  option '-v' */
void option_v(void)
{
    assert(option == 'v');

    glflags.verbose++;
}

/*  option '-V' */
void option_V(void)
{
    assert(option == 'V');

    /* Display dwarfdump compilation date and time */
    print_version_details(glflags.program_fullname,TRUE);
    exit(OKAY);
}

/*  option '-w' */
void option_w(void)
{
    assert(option == 'w');

    /* .debug_weaknames */
    glflags.gf_weakname_flag = TRUE;
    suppress_check_dwarf();
}

/*  option '-W' */
void option_W(void)
{
    assert(option == 'W');

    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 'c': option_Wc(); break;
        case 'p': option_Wp(); break;
        default: usage_error = TRUE; break;
        }
    }
    else {
        /* Search results in wide format */
        glflags.gf_search_wide_format = TRUE;
        /* -W : Display parent and children tree */
        glflags.gf_display_children_tree = TRUE;
        glflags.gf_display_parent_tree = TRUE;
    }
}

/*  option '-Wc' */
void option_Wc(void)
{
    /* Search results in wide format */
    glflags.gf_search_wide_format = TRUE;
    /* -Wc : Display children tree */
    glflags.gf_display_children_tree = TRUE;
}

/*  option '-Wp' */
void option_Wp(void)
{
    /* Search results in wide format */
    glflags.gf_search_wide_format = TRUE;
    /* -Wp : Display parent tree */
    glflags.gf_display_parent_tree = TRUE;
}

/*  option '-x' */
void option_x(void)
{
    assert(option == 'x');

    const char *path = 0;
    const char *abi = 0;
    /*  -x name=<path> meaning name dwarfdump.conf file -x
        abi=<abi> meaning select abi from dwarfdump.conf
        file. Must always select abi to use dwarfdump.conf */
    if (strncmp(dwoptarg, "name=", 5) == 0) {
        path = do_uri_translation(&dwoptarg[5],"-x name=");
        if (strlen(path) < 1) {
            goto badopt;
        }
        esb_empty_string(glflags.config_file_path);
        esb_append(glflags.config_file_path,path);
    } else if (strncmp(dwoptarg, "abi=", 4) == 0) {
        abi = do_uri_translation(&dwoptarg[4],"-x abi=");
        if (strlen(abi) < 1) {
            goto badopt;
        }
        config_file_abi = abi;
    } else if (strncmp(dwoptarg, "groupnumber=", 12) == 0) {
        /*  By default prints the lowest
            groupnumber in the object.
            Default is  -x groupnumber=0
            For group 1 (standard base dwarfdata)
                -x groupnumber=1
            For group 1 (DWARF5 .dwo sections and dwp data)
                -x groupnumber=2 */
            long int gnum = 0;
            int res = 0;

            res = get_number_value(dwoptarg+12,&gnum);
            if (res == DW_DLV_OK) {
                glflags.group_number = gnum;
            } else {
                goto badopt;
            }
    } else if (strncmp(dwoptarg, "tied=", 5) == 0) {
        const char *tiedpath = 0;
        tiedpath = do_uri_translation(&dwoptarg[5],"-x tied=");
        if (strlen(tiedpath) < 1) {
            goto badopt;
        }
        esb_empty_string(glflags.config_file_tiedpath);
        esb_append(glflags.config_file_tiedpath,tiedpath);
    } else if (strncmp(dwoptarg, "line5=", 6) == 0) {
        if (strlen(dwoptarg) < 6) {
            goto badopt;
        }
        if (!strcmp(&dwoptarg[6],"std")) {
            glflags.gf_line_flag_selection =  singledw5;
        } else if (!strcmp(&dwoptarg[6],"s2l")){
            glflags.gf_line_flag_selection= s2l;
        } else if (!strcmp(&dwoptarg[6],"orig")){
            glflags.gf_line_flag_selection= orig;
        } else if (!strcmp(&dwoptarg[6],"orig2l")) {
            glflags.gf_line_flag_selection= orig2l;
        } else {
            goto badopt;
        }
    } else if (strcmp(dwoptarg, "nosanitizestrings") == 0) {
        no_sanitize_string_garbage = TRUE;
    } else if (strcmp(dwoptarg,"noprintsectiongroups") == 0){
        glflags.gf_section_groups_flag = FALSE;
    } else {
    badopt:
        fprintf(stderr, "-x name=<path-to-conf> \n");
        fprintf(stderr, " and  \n");
        fprintf(stderr, "-x abi=<abi-in-conf> \n");
        fprintf(stderr, " and  \n");
        fprintf(stderr, "-x tied=<tied-file-path> \n");
        fprintf(stderr, " and  \n");
        fprintf(stderr, "-x line5={std,s2l,orig,orig2l} \n");
        fprintf(stderr, " and  \n");
        fprintf(stderr, "-x nosanitizestrings \n");
        fprintf(stderr, "are legal, not -x %s\n", dwoptarg);
        usage_error = TRUE;
    }
}

/*  option '-y' */
void option_y(void)
{
    assert(option == 'y');

    /* .debug_pubtypes */
    /* Also for SGI-only, and obsolete, .debug_typenames */
    suppress_check_dwarf();
    glflags.gf_pubtypes_flag = TRUE;
}

/*  option '-z' */
void option_z(void)
{
    assert(option == 'z');

    fprintf(stderr, "-z is no longer supported:ignored\n");
}
