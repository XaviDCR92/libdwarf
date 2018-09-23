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

static const char *usage_long_text[] = {
"Usage: DwarfDump <options> <object file>",
" ",
"-----------------------------------------------------------------------------",
"Print Debug Sections",
"-----------------------------------------------------------------------------",
"-b   --print-abbrev      Print abbrev section",
"-r   --print-aranges     Print aranges section",
"-F   --print-eh-frame    Print gnu .eh_frame section",
"-I   --print-fission     Print fission sections:",
"                           .gdb_index, .debug_cu_index, .debug_tu_index",
"-f   --print-frame       Print dwarf frame section",
"-i   --print-info        Print info section",
"-l   --print-lines       Print line section",
"-ls  --print-lines-short Print line section, but do not print <pc> address",
"-c   --print-loc         Print loc section",
"-m   --print-macinfo     Print macinfo section",
"-P   --print-producers   Print list of compile units per producer",
"-p   --print-pubnames    Print pubnames section",
"-N   --print-ranges      Print ranges section",
"-ta  --print-static      Print both static sections",
"-tf  --print-static-func Print static func section",
"-tv  --print-static-var  Print static var section",
"-s   --print-strings     Print string section",
"     --print-str-offsets Print details from the .debug_str_offsets section",
"-y   --print-type        Print type section",
"-w   --print-weakname    Print weakname section",
" ",
"-----------------------------------------------------------------------------",
"Print Relocations Info",
"-----------------------------------------------------------------------------",
"-o   --reloc           Print relocation info [liaprfoR]",
"-oa  --reloc-abbrev    Print relocation .debug_abbrev section",
"-or  --reloc-aranges   Print relocation .debug_aranges section",
"-of  --reloc-frames    Print relocation .debug_frame section",
"-oi  --reloc-info      Print relocation .debug_info section",
"-ol  --reloc-line      Print relocation .debug_line section",
"-oo  --reloc-loc       Print relocation .debug_loc section",
"-op  --reloc-pubnames  Print relocation .debug_pubnames section",
"-oR  --reloc-ranges    Print relocation .debug_ranges section",
" ",
"-----------------------------------------------------------------------------",
"Print ELF sections header",
"-----------------------------------------------------------------------------",
"-E   --elf           Print object Header and/or section information",
"                       Same as -E[hliaprfoRstxd]",
"-Ea  --elf-abbrev    Print .debug_abbrev header",
"-Er  --elf-aranges   Print .debug_aranges header",
"-Ed  --elf-default   Same as -E and {liaprfoRstx}",
"-Ef  --elf-frames    Print .debug_frame header",
"-Eh  --elf-header    Print ELF header",
"-Ei  --elf-info      Print .debug_info header",
"-El  --elf-line      Print .debug_line header",
"-Eo  --elf-loc       Print .debug_loc header",
"-Ep  --elf-pubnames  Print .debug_pubnames header",
"-Et  --elf-pubtypes  Print .debug_types header",
"-ER  --elf-ranges    Print .debug_ranges header",
"-Es  --elf-strings   Print .debug_string header",
"-Ex  --elf-text      Print .text header",
" ",
"-----------------------------------------------------------------------------",
"Check DWARF Integrity",
"-----------------------------------------------------------------------------",
"-kb  --check-abbrev          Check abbreviations",
"-ka  --check-all             Do all checks",
"-kM  --check-aranges         Check ranges list (.debug_aranges)",
"-kD  --check-attr-dup        Check duplicated attributes",
"-kE  --check-attr-encodings  Examine attributes encodings",
"-kn  --check-attr-names      Examine names in attributes",
"-kc  --check-constants       Examine DWARF constants",
"-kF  --check-files-lines     Examine integrity of files-lines attributes",
"-kR  --check-forward-refs    Check forward references to DIEs (declarations)",
"-kx  --check-frame-basic     Basic frames check (.eh_frame, .debug_frame)",
"-kxe --check-frame-extended  Extensive frames check (.eh_frame, .debug_frame)",
"-kf  --check-frame-info      Examine frame information (use with -f or -F)",
"-kg  --check-gaps            Check debug info gaps",
"-kl  --check-loc             Check location list (.debug_loc)",
"-kw  --check-macros          Check macros",
"-ke  --check-pubnames        Examine attributes of pubnames",
"-km  --check-ranges          Check ranges list (.debug_ranges)",
"-kS  --check-self-refs       Check self references to DIEs",
"-kd  --check-show            Show check results",
"-ks  --check-silent          Perform checks in silent mode",
"-ki  --check-summary         Display summary for all compilers",
"-kr  --check-tag-attr        Examine tag-attr relation",
"-kt  --check-tag-tag         Examine tag-tag relations",
"                               Unless -C option given certain common tag-attr",
"                               and tag-tag extensions are assumed to be ok",
"                               (not reported).",
"-ky  --check-type            Eexamine type info",
"-kG  --check-unique          Print only unique errors",
#ifdef HAVE_USAGE_TAG_ATTR
"-ku  --check-usage           Print tag-tree & tag-attr usage (basic format)",
"-kuf --check-usage-extended  Print tag-tree & tag-attr usage (full format)",
#endif /* HAVE_USAGE_TAG_ATTR */
" ",
"-----------------------------------------------------------------------------",
"Print Output Qualifiers",
"-----------------------------------------------------------------------------",
"-M   --format-attr-name        Print the form name for each attribute",
"-d   --format-dense            One line per entry (info section only)",
"-e   --format-ellipsis         Short names for tags, attrs etc.",
"-G   --format-global-offsets   Show global die offsets",
"-R   --format-registers        Print frame register names as r33 etc",
"                                 and allow up to 1200 registers.",
"                                 Print using a 'generic' register set.",
"-Q   --format-suppress-data    Suppress printing section data",
"-D   --format-suppress-offsets do not show offsets",
"-n   --format-suppress-lookup  Suppress frame information function name lookup",
"                                 (when printing frame information from multi",
"                                 gigabyte, object files this option may save",
"                                 significant time).",
"-C   --format-extensions       Activate printing (with -i) of warnings about",
"                                 certain common extensions of DWARF.",
"-g   --format-loc              Use incomplete loclist support",
" ",
"-----------------------------------------------------------------------------",
"Print Output Limiters",
"-----------------------------------------------------------------------------",
"-u<file> --format-file=<file>    Print sections only for specified file",
"-H<num>  --format-limit=<num>    Limit output to the first <num> major units",
"                                   stop after <num> compilation units",
"-c<str>  --format-producer=<str> Check only specific compiler objects",
"                                   <str> is described by 'DW_AT_producer'",
"                                   -c'350.1' check only compiler objects with",
"                                   350.1 in the CU name",
"-cg      --format-gcc            Check only GCC compiler objects",
"-cs      --format-snc            Check only SNC compiler objects",
"-x<n>    --format-group=<n>      Groupnumber to print",
" ",
"-----------------------------------------------------------------------------",
"File Specifications",
"-----------------------------------------------------------------------------",
"-O file=<path>   --file-output=<path>  Name the output file",
"-x abi=<abi>     --file-abi=<abi>      Name abi in dwarfdump.conf",
"-x name=<path>   --file-config=<path>  Name dwarfdump.conf",
"-x tied=<path>   --file-tied=<path>    Name an associated object file",
"                                         (Split DWARF)",
" ",
"-----------------------------------------------------------------------------",
"Search text in attributes",
"-----------------------------------------------------------------------------",
"-S any=<text>    --search-any=<text>         Search any <text>",
"-Svany=<text>    --search-any-count=<text>     print number of occurrences",
"-S match=<text>  --search-match=<text>       Search matching <text>",
"-Svmatch=<text>  --search-match-count<text>    print number of occurrences",
#ifdef HAVE_REGEX
"-S regex=<text>  --search-regex=<text>       Use regular expression matching",
"-Svregex=<text>  --search-regex-count<text>    print number of occurrences",
#endif /* HAVE_REGEX */
"                                         only one -S option allowed, any= and",
"                                         regex= only usable if the functions",
"                                         required are found at configure time",
" ",
"-W   --search-print-tree     Print parent/children tree (wide format) with -S",
"-Wp  --search-print-parent   Print parent tree (wide format) with -S",
"-Wc  --search-print-children Print children tree (wide format) with -S",
" ",
"-----------------------------------------------------------------------------",
"Help & Version",
"-----------------------------------------------------------------------------",
"-h   --help          Print the dwarfdump help message",
"-v   --verbose       Show more information",
"-vv  --verbose-more  Show even more information",
"-V   --version       Print version information",
"",
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

/*  These functions implement the individual options. They are called from
    short names and long names options. */

/*  Handlers for the long names options. */
static void option_print_str_offsets(void);
static void option_print_debug_names(void);

static void option_trace(void);

static void option_a(void);
static void option_b(void);

/*  Option '-c[...]' */
static void option_c_0(void);
static void option_c(void);
static void option_cs(void);
static void option_cg(void);

static void option_C(void);
static void option_d(void);
static void option_D(void);
static void option_e(void);

/*  Option '-E[...]' */
static void option_E_0(void);
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

/*  Option '-k[...]' */
static void option_k_0(void);
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
/*  Option '-ku[...]' */
static void option_ku_0(void);
static void option_ku(void);
static void option_kuf(void);
#endif /* HAVE_USAGE_TAG_ATTR */

static void option_kw(void);

/*  Option '-kx[...]' */
static void option_kx_0(void);
static void option_kx(void);
static void option_kxe(void);

static void option_ky(void);

/*  Option '-l[...]' */
static void option_l_0(void);
static void option_l(void);
static void option_ls(void);

static void option_m(void);
static void option_M(void);
static void option_n(void);
static void option_N(void);

/*  Option '-o[...]' */
static void option_o_0(void);
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

/*  Option '-S[...]' */
static void option_S(void);
static void option_S_any(void);
static void option_S_match(void);
static void option_S_regex(void);
static void option_Sv(void);
static void option_S_badopt(void);

static void option_t(void);
static void option_ta(void);
static void option_tf(void);
static void option_tv(void);
static void option_u(void);
static void option_U(void);
static void option_v(void);
static void option_V(void);
static void option_w(void);

/*  Option '-W[...]' */
static void option_W_0(void);
static void option_W(void);
static void option_Wc(void);
static void option_Wp(void);

/*  Option '-x[...]' */
static void option_x_0(void);
static void option_x_badopt(void);
static void option_x_abi();
static void option_x_groupnumber();
static void option_x_line5();
static void option_x_name();
static void option_x_nosanitizestrings();
static void option_x_noprintsectiongroups();
static void option_x_tied();

static void option_y(void);
static void option_z(void);

/*  Extracted from 'process_args', as they are used by option handlers. */
static boolean usage_error = FALSE;
static int option = 0;

enum longopts_vals {
    OPTIONS_BEGIN = 999,

    /* Check DWARF Integrity                                                */
    CHECK_ABBREV,             /* -kb  --check-abbrev                        */
    CHECK_ALL,                /* -ka  --check-all                           */
    CHECK_ARANGES,            /* -kM  --check-aranges                       */
    CHECK_ATTR_DUP,           /* -kD  --check-attr-dup                      */
    CHECK_ATTR_ENCODINGS,     /* -kE  --check-attr-encodings                */
    CHECK_ATTR_NAMES,         /* -kn  --check-attr-names                    */
    CHECK_CONSTANTS,          /* -kc  --check-constants                     */
    CHECK_FILES_LINES,        /* -kF  --check-files-lines                   */
    CHECK_FORWARD_REFS,       /* -kR  --check-forward-refs                  */
    CHECK_FRAME_BASIC,        /* -kx  --check-frame-basic                   */
    CHECK_FRAME_EXTENDED,     /* -kxe --check-frame-extended                */
    CHECK_FRAME_INFO,         /* -kf  --check-frame-info                    */
    CHECK_GAPS,               /* -kg  --check-gaps                          */
    CHECK_LOC,                /* -kl  --check-loc                           */
    CHECK_MACROS,             /* -kw  --check-macros                        */
    CHECK_PUBNAMES,           /* -ke  --check-pubnames                      */
    CHECK_RANGES,             /* -km  --check-ranges                        */
    CHECK_SELF_REFS,          /* -kS  --check-self-refs                     */
    CHECK_SHOW,               /* -kd  --check-show                          */
    CHECK_SILENT,             /* -ks  --check-silent                        */
    CHECK_SUMMARY,            /* -ki  --check-summary                       */
    CHECK_TAG_ATTR,           /* -kr  --check-tag-attr                      */
    CHECK_TAG_TAG,            /* -kt  --check-tag-tag                       */
    CHECK_TYPE,               /* -ky  --check-type                          */
    CHECK_UNIQUE,             /* -kG  --check-unique                        */
#ifdef HAVE_USAGE_TAG_ATTR
    CHECK_USAGE,              /* -ku  --check-usage                         */
    CHECK_USAGE_EXTENDED,     /* -kuf --check-usage-extended                */
#endif /* HAVE_USAGE_TAG_ATTR */

    /* Print ELF sections header                                            */
    ELF,                      /* -E   --elf                                 */
    ELF_ABBREV,               /* -Ea  --elf-abbrev                          */
    ELF_ARANGES,              /* -Er  --elf-aranges                         */
    ELF_DEFAULT,              /* -Ed  --elf-default                         */
    ELF_FRAMES,               /* -Ef  --elf-frames                          */
    ELF_HEADER,               /* -Eh  --elf-header                          */
    ELF_INFO,                 /* -Ei  --elf-info                            */
    ELF_LINE,                 /* -El  --elf-line                            */
    ELF_LOC,                  /* -Eo  --elf-loc                             */
    ELF_PUBNAMES,             /* -Ep  --elf-pubnames                        */
    ELF_PUBTYPES,             /* -Et  --elf-pubtypes                        */
    ELF_RANGES,               /* -ER  --elf-ranges                          */
    ELF_STRINGS,              /* -Es  --elf-strings                         */
    ELF_TEXT,                 /* -Ex  --elf-text                            */

    /* File Specifications                                                  */
    FILE_ABI,                 /* -x abi=<abi>    --file-abi=<abi>           */
    FILE_CONFIG,              /* -x name=<path>  --file-config=<path>       */
    FILE_OUTPUT,              /* -O file=<path>  --file-output=<path>       */
    FILE_TIED,                /* -x tied=<path>  --file-tied=<path>         */

    /* Print Output Qualifiers                                              */
    FORMAT_ATTR_NAME,         /* -M   --format-attr-name                    */
    FORMAT_DENSE,             /* -d   --format-dense                        */
    FORMAT_ELLIPSIS,          /* -e   --format-ellipsis                     */
    FORMAT_EXTENSIONS,        /* -C   --format-extensions                   */
    FORMAT_GLOBAL_OFFSETS,    /* -G   --format-global-offsets               */
    FORMAT_LOC,               /* -g   --format-loc                          */
    FORMAT_REGISTERS,         /* -R   --format-registers                    */
    FORMAT_SUPPRESS_DATA,     /* -Q   --format-suppress-data                */
    FORMAT_SUPPRESS_OFFSETS,  /* -D   --format-suppress-offsets             */
    FORMAT_SUPPRESS_LOOKUP,   /* -n   --format-suppress-lookup              */

    /* Print Output Limiters                                                */
    FORMAT_FILE,              /* -u<file> --format-file=<file>              */
    FORMAT_GCC,               /* -cg      --format-gcc                      */
    FORMAT_GROUP,             /* -x<n>    --format-group=<n>                */
    FORMAT_LIMIT,             /* -H<num>  --format-limit=<num>              */
    FORMAT_PRODUCER,          /* -c<str>  --format-producer=<str>           */
    FORMAT_SNC,               /* -cs      --format-snc                      */

    /* Print Debug Sections                                                 */
    PRINT_ABBREV,             /* -b   --print-abbrev                        */
    PRINT_ARANGES,            /* -r   --print-aranges                       */
    PRINT_DEBUG_NAMES,        /*      --print-debug-name                    */
    PRINT_EH_FRAME,           /* -F   --print-eh-frame                      */
    PRINT_FISSION,            /* -I   --print-fission                       */
    PRINT_FRAME,              /* -f   --print-frame                         */
    PRINT_INFO,               /* -i   --print-info                          */
    PRINT_LINES,              /* -l   --print-lines                         */
    PRINT_LINES_SHORT,        /* -ls  --print-lines-short                   */
    PRINT_LOC,                /* -c   --print-loc                           */
    PRINT_MACINFO,            /* -m   --print-macinfo                       */
    PRINT_PRODUCERS,          /* -p   --print-producers                     */
    PRINT_PUBNAMES,           /* -p   --print-pubnames                      */
    PRINT_RANGES,             /* -N   --print-ranges                        */
    PRINT_STATIC,             /* -ta  --print-static                        */
    PRINT_STATIC_FUNC,        /* -tf  --print-static-func                   */
    PRINT_STATIC_VAR,         /* -tv  --print-static-var                    */
    PRINT_STRINGS,            /* -s   --print-strings                       */
    PRINT_STR_OFFSETS,        /*      --print-str-offsets                   */
    PRINT_TYPE,               /* -y   --print-type                          */
    PRINT_WEAKNAME,           /* -w   --print-weakname                      */

    /* Print Relocations Info                                               */
    RELOC,                    /* -o   --reloc                               */
    RELOC_ABBREV,             /* -oa  --reloc-abbrev                        */
    RELOC_ARANGES,            /* -or  --reloc-aranges                       */
    RELOC_FRAMES,             /* -of  --reloc-frames                        */
    RELOC_INFO,               /* -oi  --reloc-info                          */
    RELOC_LINE,               /* -ol  --reloc-line                          */
    RELOC_LOC,                /* -oo  --reloc-loc                           */
    RELOC_PUBNAMES,           /* -op  --reloc-pubnames                      */
    RELOC_RANGES,             /* -oR  --reloc-ranges                        */

    /* Search text in attributes                                            */
    SEARCH_ANY,               /* -S any=<text>   --search-any=<text>        */
    SEARCH_ANY_COUNT,         /* -Svany=<text>   --search-any-count=<text>  */
    SEARCH_MATCH,             /* -S match=<text> --search-match=<text>      */
    SEARCH_MATCH_COUNT,       /* -Svmatch=<text> --search-match-count<text> */
    SEARCH_PRINT_CHILDREN,    /* -Wc --search-print-children                */
    SEARCH_PRINT_PARENT,      /* -Wp --search-print-parent                  */
    SEARCH_PRINT_TREE,        /* -W  --search-print-tree                    */
#ifdef HAVE_REGEX
    SEARCH_REGEX,             /* -S regex=<text> --search-regex=<text>      */
    SEARCH_REGEX_COUNT,       /* -Svregex=<text> --search-regex-count<text> */
#endif /* HAVE_REGEX */

    /* Help & Version                                                       */
    HELP,                     /* -h  --help                                 */
    VERBOSE,                  /* -v  --verbose                              */
    VERBOSE_MORE,             /* -vv --verbose-more                         */
    VERSION,                  /* -V  --version                              */

    /* Trace                                                                */
    TRACE,                    /* -# --trace<num>                            */

    OPTIONS_END,
};

static struct dwoption longopts[] =  {

    /* Check DWARF Integrity */
    {"check-abbrev",            dwno_argument, 0, CHECK_ABBREV               },      
    {"check-all",               dwno_argument, 0, CHECK_ALL                  },      
    {"check-aranges",           dwno_argument, 0, CHECK_ARANGES              },
    {"check-attr-dup",          dwno_argument, 0, CHECK_ATTR_DUP             },
    {"check-attr-encodings",    dwno_argument, 0, CHECK_ATTR_ENCODINGS       },
    {"check-attr-names",        dwno_argument, 0, CHECK_ATTR_NAMES           },
    {"check-constants",         dwno_argument, 0, CHECK_CONSTANTS            },
    {"check-files-lines",       dwno_argument, 0, CHECK_FILES_LINES          },
    {"check-forward-refs",      dwno_argument, 0, CHECK_FORWARD_REFS         },
    {"check-frame-basic",       dwno_argument, 0, CHECK_FRAME_BASIC          },
    {"check-frame-extended",    dwno_argument, 0, CHECK_FRAME_EXTENDED       },
    {"check-frame-info",        dwno_argument, 0, CHECK_FRAME_INFO           },
    {"check-gaps",              dwno_argument, 0, CHECK_GAPS                 },
    {"check-loc",               dwno_argument, 0, CHECK_LOC                  },
    {"check-macros",            dwno_argument, 0, CHECK_MACROS               },
    {"check-pubnames",          dwno_argument, 0, CHECK_PUBNAMES             },
    {"check-ranges",            dwno_argument, 0, CHECK_RANGES               },
    {"check-self-refs",         dwno_argument, 0, CHECK_SELF_REFS            },
    {"check-show",              dwno_argument, 0, CHECK_SHOW                 },
    {"check-silent",            dwno_argument, 0, CHECK_SILENT               },
    {"check-summary",           dwno_argument, 0, CHECK_SUMMARY              },
    {"check-tag-attr",          dwno_argument, 0, CHECK_TAG_ATTR             },
    {"check-tag-tag",           dwno_argument, 0, CHECK_TAG_TAG              },
    {"check-type",              dwno_argument, 0, CHECK_TYPE                 },
    {"check-unique",            dwno_argument, 0, CHECK_UNIQUE               },
#ifdef HAVE_USAGE_TAG_ATTR
    {"check-usage",             dwno_argument, 0, CHECK_USAGE                },
    {"check-usage-extended",    dwno_argument, 0, CHECK_USAGE_EXTENDED       },
#endif /* HAVE_USAGE_TAG_ATTR */

    /* Print ELF sections header */
    {"elf",                     dwno_argument, 0, ELF                        },
    {"elf-abbrev",              dwno_argument, 0, ELF_ABBREV                 },
    {"elf-aranges",             dwno_argument, 0, ELF_ARANGES                },
    {"elf-default",             dwno_argument, 0, ELF_DEFAULT                },
    {"elf-frames",              dwno_argument, 0, ELF_FRAMES                 },
    {"elf-header",              dwno_argument, 0, ELF_HEADER                 },
    {"elf-info",                dwno_argument, 0, ELF_INFO                   },
    {"elf-line",                dwno_argument, 0, ELF_LINE                   },
    {"elf-loc",                 dwno_argument, 0, ELF_LOC                    },
    {"elf-pubnames",            dwno_argument, 0, ELF_PUBNAMES               },
    {"elf-pubtypes",            dwno_argument, 0, ELF_PUBTYPES               },
    {"elf-ranges",              dwno_argument, 0, ELF_RANGES                 },
    {"elf-strings",             dwno_argument, 0, ELF_STRINGS                },
    {"elf-text",                dwno_argument, 0, ELF_TEXT                   },

    /* File Specifications                                                  */
    {"file-abi",                dwrequired_argument, 0, FILE_ABI             },
    {"file-config",             dwrequired_argument, 0, FILE_CONFIG          },
    {"file-output",             dwrequired_argument, 0, FILE_OUTPUT          },
    {"file-tied",               dwrequired_argument, 0, FILE_TIED            },

    /* Print Output Qualifiers */
    {"format-attr-name",        dwno_argument, 0, FORMAT_ATTR_NAME           },
    {"format-dense",            dwno_argument, 0, FORMAT_DENSE               },
    {"format-ellipsis",         dwno_argument, 0, FORMAT_ELLIPSIS            },
    {"format-extensions",       dwno_argument, 0, FORMAT_EXTENSIONS          },
    {"format-global-offsets",   dwno_argument, 0, FORMAT_GLOBAL_OFFSETS      },
    {"format-loc",              dwno_argument, 0, FORMAT_LOC                 },
    {"format-registers",        dwno_argument, 0, FORMAT_REGISTERS           },
    {"format-suppress-data",    dwno_argument, 0, FORMAT_SUPPRESS_DATA       },
    {"format-suppress-offsets", dwno_argument, 0, FORMAT_SUPPRESS_OFFSETS    },
    {"format-suppress-lookup",  dwno_argument, 0, FORMAT_SUPPRESS_LOOKUP     },

    /* Print Output Limiters                                                */
    {"format-file",             dwrequired_argument, 0, FORMAT_FILE          },
    {"format-gcc",              dwno_argument,       0, FORMAT_GCC           },
    {"format-group",            dwrequired_argument, 0, FORMAT_GROUP         },
    {"format-limit",            dwrequired_argument, 0, FORMAT_LIMIT         },
    {"format-producer",         dwrequired_argument, 0, FORMAT_PRODUCER      },
    {"format-snc",              dwno_argument,       0, FORMAT_SNC           },

    /* Print Debug Sections */
    {"print-abbrev",            dwno_argument, 0, PRINT_ABBREV               },
    {"print-aranges",           dwno_argument, 0, PRINT_ARANGES              },
    {"print-debug-names",       dwno_argument, 0, PRINT_DEBUG_NAMES          },
    {"print-eh-frame",          dwno_argument, 0, PRINT_EH_FRAME             },
    {"print-fission",           dwno_argument, 0, PRINT_FISSION              },
    {"print-frame",             dwno_argument, 0, PRINT_FRAME                },
    {"print-info",              dwno_argument, 0, PRINT_INFO                 },
    {"print-lines",             dwno_argument, 0, PRINT_LINES                },
    {"print-lines-short",       dwno_argument, 0, PRINT_LINES_SHORT          },
    {"print-loc",               dwno_argument, 0, PRINT_LOC                  },
    {"print-macinfo",           dwno_argument, 0, PRINT_MACINFO              },
    {"print-producers",         dwno_argument, 0, PRINT_PRODUCERS            },
    {"print-pubnames",          dwno_argument, 0, PRINT_PUBNAMES             },
    {"print-ranges",            dwno_argument, 0, PRINT_RANGES               },
    {"print-static",            dwno_argument, 0, PRINT_STATIC               },
    {"print-static-func",       dwno_argument, 0, PRINT_STATIC_FUNC          },
    {"print-static-var",        dwno_argument, 0, PRINT_STATIC_VAR           },
    {"print-strings",           dwno_argument, 0, PRINT_STRINGS              },
    {"print-str-offsets",       dwno_argument, 0, PRINT_STR_OFFSETS          },
    {"print-type",              dwno_argument, 0, PRINT_TYPE                 },
    {"print-weakname",          dwno_argument, 0, PRINT_WEAKNAME             },

    /* Print Relocations Info */
    {"reloc",                   dwno_argument, 0, RELOC                      },
    {"reloc-abbrev",            dwno_argument, 0, RELOC_ABBREV               },
    {"reloc-aranges",           dwno_argument, 0, RELOC_ARANGES              },
    {"reloc-frames",            dwno_argument, 0, RELOC_FRAMES               },
    {"reloc-info",              dwno_argument, 0, RELOC_INFO                 },
    {"reloc-line",              dwno_argument, 0, RELOC_LINE                 },
    {"reloc-loc",               dwno_argument, 0, RELOC_LOC                  },
    {"reloc-pubnames",          dwno_argument, 0, RELOC_PUBNAMES             },
    {"reloc-ranges",            dwno_argument, 0, RELOC_RANGES               },

    /* Search text in attributes                                            */
    {"search-any",              dwrequired_argument, 0, SEARCH_ANY           },
    {"search-any-count",        dwrequired_argument, 0, SEARCH_ANY_COUNT     },
    {"search-match",            dwrequired_argument, 0, SEARCH_MATCH         },
    {"search-match-count",      dwrequired_argument, 0, SEARCH_MATCH_COUNT   },
    {"search-print-children",   dwno_argument,       0, SEARCH_PRINT_CHILDREN},
    {"search-print-parent",     dwno_argument,       0, SEARCH_PRINT_PARENT  },
    {"search-print-tree",       dwno_argument,       0, SEARCH_PRINT_TREE    },
#ifdef HAVE_REGEX
    {"search-regex",            dwrequired_argument, 0, SEARCH_REGEX         },
    {"search-regex-count",      dwrequired_argument, 0, SEARCH_REGEX_COUNT   },
#endif /* HAVE_REGEX */

    /* Help & Version                                                       */
    {"help",                    dwno_argument,       0, HELP                 },
    {"verbose",                 dwno_argument,       0, VERBOSE              },
    {"version",                 dwno_argument,       0, VERSION              },

    /* Trace                                                                */
    {"trace",                   dwrequired_argument, 0, TRACE                },

    {0,0,0,0}
};

/* process arguments and return object filename */
const char *
process_args(int argc, char *argv[])
{
    int longindex = 0;

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
        case '#': option_trace(); break;
        case 'a': option_a();     break;
        case 'b': option_b();     break;
        case 'c': option_c_0();   break;
        case 'C': option_C();     break;
        case 'd': option_d();     break;
        case 'D': option_D();     break;
        case 'e': option_e();     break;
        case 'E': option_E_0();   break;
        case 'f': option_f();     break;
        case 'F': option_F();     break;
        case 'g': option_g();     break;
        case 'G': option_G();     break;
        case 'h': option_h();     break;
        case 'H': option_H();     break;
        case 'i': option_i();     break;
        case 'I': option_I();     break;
        case 'k': option_k_0();   break;
        case 'l': option_l_0();   break;
        case 'm': option_m();     break;
        case 'M': option_M();     break;
        case 'n': option_n();     break;
        case 'N': option_N();     break;
        case 'o': option_o_0();   break;
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
        case 'W': option_W_0();   break;
        case 'x': option_x_0();   break;
        case 'y': option_y();     break;
        case 'z': option_z();     break;

        /* Check DWARF Integrity. */
        case CHECK_ABBREV:          break;
        case CHECK_ALL:             break;
        case CHECK_ARANGES:         break;
        case CHECK_ATTR_DUP:        break;
        case CHECK_ATTR_ENCODINGS:  break;
        case CHECK_ATTR_NAMES:      break;
        case CHECK_CONSTANTS:       break;
        case CHECK_FILES_LINES:     break;
        case CHECK_FORWARD_REFS:    break;
        case CHECK_FRAME_BASIC:     break;
        case CHECK_FRAME_EXTENDED:  break;
        case CHECK_FRAME_INFO:      break;
        case CHECK_GAPS:            break;
        case CHECK_LOC:             break;
        case CHECK_MACROS:          break;
        case CHECK_PUBNAMES:        break;
        case CHECK_RANGES:          break;
        case CHECK_SELF_REFS:       break;
        case CHECK_SHOW:            break;
        case CHECK_SILENT:          break;
        case CHECK_SUMMARY:         break;
        case CHECK_TAG_ATTR:        break;
        case CHECK_TAG_TAG:         break;
        case CHECK_TYPE:            break;
        case CHECK_UNIQUE:          break;
#ifdef HAVE_USAGE_TAG_ATTR
        case CHECK_USAGE:           break;
        case CHECK_USAGE_EXTENDED:  break;
#endif /* HAVE_USAGE_TAG_ATTR */

        /* Print ELF sections header. */
        case ELF:           break;
        case ELF_ABBREV:    break;
        case ELF_ARANGES:   break;
        case ELF_DEFAULT:   break;
        case ELF_FRAMES:    break;
        case ELF_HEADER:    break;
        case ELF_INFO:      break;
        case ELF_LINE:      break;
        case ELF_LOC:       break;
        case ELF_PUBNAMES:  break;
        case ELF_PUBTYPES:  break;
        case ELF_RANGES:    break;
        case ELF_STRINGS:   break;
        case ELF_TEXT:      break;

        /* File Specifications. */
        case FILE_ABI:    break;
        case FILE_CONFIG: break;
        case FILE_OUTPUT: break;
        case FILE_TIED:   break;

        /* Print Output Qualifiers. */
        case FORMAT_ATTR_NAME:        break;
        case FORMAT_DENSE:            break;
        case FORMAT_ELLIPSIS:         break;
        case FORMAT_EXTENSIONS:       break;
        case FORMAT_GLOBAL_OFFSETS:   break;
        case FORMAT_LOC:              break;
        case FORMAT_REGISTERS:        break;
        case FORMAT_SUPPRESS_DATA:    break;
        case FORMAT_SUPPRESS_OFFSETS: break;
        case FORMAT_SUPPRESS_LOOKUP:  break;

        /* Print Output Limiters. */
        case FORMAT_FILE:     break;
        case FORMAT_GCC:      break;
        case FORMAT_GROUP:    break;
        case FORMAT_LIMIT:    break;
        case FORMAT_PRODUCER: break;
        case FORMAT_SNC:      break;

        /* Print Debug Sections. */
        case PRINT_ABBREV:                                  break;
        case PRINT_ARANGES:                                 break;
        case PRINT_DEBUG_NAMES: option_print_debug_names(); break;
        case PRINT_EH_FRAME:                                break;
        case PRINT_FISSION:                                 break;
        case PRINT_FRAME:                                   break;
        case PRINT_INFO:                                    break;
        case PRINT_LINES:                                   break;
        case PRINT_LINES_SHORT:                             break;
        case PRINT_LOC:                                     break;
        case PRINT_MACINFO:                                 break;
        case PRINT_PRODUCERS:                               break;
        case PRINT_PUBNAMES:                                break;
        case PRINT_RANGES:                                  break;
        case PRINT_STATIC:                                  break;
        case PRINT_STATIC_FUNC:                             break;
        case PRINT_STATIC_VAR:                              break;
        case PRINT_STRINGS:                                 break;
        case PRINT_STR_OFFSETS: option_print_str_offsets(); break;
        case PRINT_TYPE:                                    break;
        case PRINT_WEAKNAME:                                break;

        /* Print Relocations Info. */
        case RELOC:           break;
        case RELOC_ABBREV:    break;
        case RELOC_ARANGES:   break;
        case RELOC_FRAMES:    break;
        case RELOC_INFO:      break;
        case RELOC_LINE:      break;
        case RELOC_LOC:       break;
        case RELOC_PUBNAMES:  break;
       case RELOC_RANGES:     break;

        /* Search text in attributes. */
        case SEARCH_ANY:            break;
        case SEARCH_ANY_COUNT:      break;
        case SEARCH_MATCH:          break;
        case SEARCH_MATCH_COUNT:    break;
        case SEARCH_PRINT_CHILDREN: break;
        case SEARCH_PRINT_PARENT:   break;
        case SEARCH_PRINT_TREE:     break;
#ifdef HAVE_REGEX
        case SEARCH_REGEX:          break;
        case SEARCH_REGEX_COUNT:    break;
#endif /* HAVE_REGEX */

        /* Help & Version. */
        case HELP: option_h();    break;
        case VERBOSE: option_v(); break;
        case VERSION: option_V(); break;

        /* Trace. */
        case TRACE: option_trace(); break;

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

/*  Option 'print_debug_names' */
void option_print_debug_names(void)
{
    glflags.gf_debug_names_flag = TRUE;
}

/*  Option '--print_str_offsets' */
void option_print_str_offsets(void)
{
    glflags.gf_print_str_offsets = TRUE;
}

/*  Option '-#' */
void option_trace(void)
{
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

/*  Option '-a' */
void option_a(void)
{
    suppress_check_dwarf();
    do_all();
}

/*  Option '-b' */
void option_b(void)
{
    glflags.gf_abbrev_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-c[...]' */
void option_c_0(void)
{
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
        option_c();
    }
}

/*  Option '-c' */
void option_c(void)
{
    glflags.gf_loc_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-cs' */
void option_cs(void)
{
    /* -cs : Check SNC compiler */
    glflags.gf_check_snc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  Option '-cg' */
void option_cg(void)
{
    /* -cg : Check GCC compiler */
    glflags.gf_check_gcc_compiler = TRUE;
    glflags.gf_check_all_compilers = FALSE;
}

/*  Option '-C' */
void option_C(void)
{
    glflags.gf_suppress_check_extensions_tables = TRUE;
}

/*  Option '-d' */
void option_d(void)
{
    glflags.gf_do_print_dwarf = TRUE;
    /*  This is sort of useless unless printing,
        but harmless, so we do not insist we
        are printing with suppress_check_dwarf(). */
    glflags.dense = TRUE;
}

/*  Option '-D' */
void option_D(void)
{
    /* Do not emit offset in output */
    glflags.gf_display_offsets = FALSE;
}

/*  Option '-e' */
void option_e(void)
{
    suppress_check_dwarf();
    glflags.ellipsis = TRUE;
}

/*  Option '-E[...]' */
void option_E_0(void)
{
    /* Object Header information (but maybe really print) */
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
        option_E();
    }
}

/*  Option '-E' */
void option_E(void)
{
    /* Display header and all sections info */
    glflags.gf_header_flag = TRUE;
    set_all_sections_on();
}

/*  Option '-Ea' */
void option_Ea(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_ABBREV);
}

/*  Option '-Ed' */
void option_Ed(void)
{
    /* case 'd', use the default section set */
    glflags.gf_header_flag = TRUE;
    set_all_section_defaults();
}

/*  Option '-Ef' */
void option_Ef(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_FRAME);
}

/*  Option '-Eh' */
void option_Eh(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_HEADER);
}

/*  Option '-Ei' */
void option_Ei(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_INFO);
    enable_section_map_entry(DW_HDR_DEBUG_TYPES);
}

/*  Option '-EI' */
void option_EI(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_GDB_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_CU_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_TU_INDEX);
    enable_section_map_entry(DW_HDR_DEBUG_NAMES);
}

/*  Option '-El' */
void option_El(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_LINE);
}

/*  Option '-Em' */
void option_Em(void)
{
    /*  For both old macinfo and dwarf5  macro */
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_MACINFO);
}

/*  Option '-Eo' */
void option_Eo(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_LOC);
}

/*  Option '-Ep' */
void option_Ep(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_PUBNAMES);
}

/*  Option '-Er' */
void option_Er(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_ARANGES);
}

/*  Option '-ER' */
void option_ER(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_RANGES);
    enable_section_map_entry(DW_HDR_DEBUG_RNGLISTS);
}

/*  Option '-Es' */
void option_Es(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_STR);
}

/*  Option '-Et' */
void option_Et(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_DEBUG_PUBTYPES);
}

/*  Option '-Ex' */
void option_Ex(void)
{
    glflags.gf_header_flag = TRUE;
    enable_section_map_entry(DW_HDR_TEXT);
}

/*  Option '-f' */
void option_f(void)
{
    glflags.gf_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-F' */
void option_F(void)
{
    glflags.gf_eh_frame_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-g' */
void option_g(void)
{
    /*info_flag = TRUE;  removed  from -g. Nov 2015 */
    glflags.gf_use_old_dwarf_loclist = TRUE;
    suppress_check_dwarf();
}

/*  Option '-G' */
void option_G(void)
{
    glflags.gf_show_global_offsets = TRUE;
}

/*  Option '-h' */
void option_h(void)
{
    print_usage_message(glflags.program_name,usage_text);
    exit(OKAY);
}

/*  Option '-H' */
void option_H(void)
{
    int break_val =  atoi(dwoptarg);
    if (break_val > 0) {
        glflags.break_after_n_units = break_val;
    }
}

/*  Option '-i' */
void option_i(void)
{
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-I' */
void option_I(void)
{
    glflags.gf_gdbindex_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-k[...]' */
void option_k_0(void)
{
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
    case 'u': option_ku_0(); break;
#endif /* HAVE_USAGE_TAG_ATTR */
    case 'w': option_kw(); break;
    case 'x': option_kx_0(); break;
    case 'y': option_ky(); break;
    default: usage_error = TRUE; break;
    }
}

/*  Option '-ka' */
void option_ka(void)
{
    suppress_print_dwarf();
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

/*  Option '-kb' */
void option_kb(void)
{
    /* Abbreviations */
    suppress_print_dwarf();
    glflags.gf_check_abbreviations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  Option '-kc' */
void option_kc(void)
{
    /* DWARF constants */
    suppress_print_dwarf();
    glflags.gf_check_dwarf_constants = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kd' */
void option_kd(void)
{
    /* Display check results */
    suppress_print_dwarf();
    glflags.gf_check_show_results = TRUE;
}

/*  Option '-kD' */
void option_kD(void)
{
    /* Check duplicated attributes */
    suppress_print_dwarf();
    glflags.gf_check_duplicated_attributes = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    /*  For some checks is worth trying the plain
        .debug_abbrev section on its own. */
    glflags.gf_abbrev_flag = TRUE;
}

/*  Option '-ke' */
void option_ke(void)
{
    suppress_print_dwarf();
    glflags.gf_check_pubname_attr = TRUE;
    glflags.gf_pubnames_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  Option '-kE' */
void option_kE(void)
{
    /* Attributes encoding usage */
    suppress_print_dwarf();
    glflags.gf_check_attr_encoding = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kf' */
void option_kf(void)
{
    suppress_print_dwarf();
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_fdes = TRUE;
}

/*  Option '-kF' */
void option_kF(void)
{
    /* files-lines */
    suppress_print_dwarf();
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_check_lines = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kg' */
void option_kg(void)
{
    /* Check debug info gaps */
    suppress_print_dwarf();
    glflags.gf_check_di_gaps = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kG' */
void option_kG(void)
{
    /* Print just global (unique) errors */
    suppress_print_dwarf();
    glflags.gf_print_unique_errors = TRUE;
}

/*  Option '-ki' */
void option_ki(void)
{
    /* Summary for each compiler */
    suppress_print_dwarf();
    glflags.gf_print_summary_all = TRUE;
}

/*  Option '-kl' */
void option_kl(void)
{
    /* Locations list */
    suppress_print_dwarf();
    glflags.gf_check_locations = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_loc_flag = TRUE;
}

/*  Option '-km' */
void option_km(void)
{
    /* Ranges */
    suppress_print_dwarf();
    glflags.gf_check_ranges = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kM' */
void option_kM(void)
{
    /* Aranges */
    suppress_print_dwarf();
    glflags.gf_check_aranges = TRUE;
    glflags.gf_aranges_flag = TRUE;
}

/*  Option '-kn' */
void option_kn(void)
{
    /* invalid names */
    suppress_print_dwarf();
    glflags.gf_check_names = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kr' */
void option_kr(void)
{
    suppress_print_dwarf();
    glflags.gf_check_attr_tag = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
    glflags.gf_check_harmless = TRUE;
}

/*  Option '-kR' */
void option_kR(void)
{
    /* forward declarations in DW_AT_specification */
    suppress_print_dwarf();
    glflags.gf_check_forward_decl = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-ks' */
void option_ks(void)
{
    /* Check verbose mode */
    suppress_print_dwarf();
    glflags.gf_check_verbose_mode = FALSE;
}

/*  Option '-kS' */
void option_kS(void)
{
    /*  self references in:
        DW_AT_specification, DW_AT_type, DW_AT_abstract_origin */
    suppress_print_dwarf();
    glflags.gf_check_self_references = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kt' */
void option_kt(void)
{
    suppress_print_dwarf();
    glflags.gf_check_tag_tree = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

#ifdef HAVE_USAGE_TAG_ATTR
/*  Option '-ku[...]' */
void option_ku_0(void)
{
    /* Tag-Tree and Tag-Attr usage */
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'f': option_kuf(); break;
        default: usage_error = TRUE; break;
        }
    } else {
      option_ku();
    }
}

/*  Option '-ku' */
void option_ku(void)
{
    suppress_print_dwarf();
    glflags.gf_print_usage_tag_attr = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_types_flag = TRUE;
}

/*  Option '-kuf' */
void option_kuf(void)
{
    option_ku();

    /* -kuf : Full report */
    glflags.gf_print_usage_tag_attr_full = TRUE;
}
#endif /* HAVE_USAGE_TAG_ATTR */

/*  Option '-kw' */
void option_kw(void)
{
    suppress_print_dwarf();
    glflags.gf_check_macros = TRUE;
    glflags.gf_macro_flag = TRUE;
    glflags.gf_macinfo_flag = TRUE;
}

/*  Option '-kx[...]' */
void option_kx_0(void)
{
    /* Frames check */
    if (dwoptarg[1]) {
        switch (dwoptarg[1]) {
        case 'e': option_kxe(); break;
        default: usage_error = TRUE; break;
        }
    } else {
      option_kx();
    }
}

/*  Option '-kx' */
void option_kx(void)
{
    suppress_print_dwarf();
    glflags.gf_check_frames = TRUE;
    glflags.gf_frame_flag = TRUE;
    glflags.gf_eh_frame_flag = TRUE;
}

/*  Option '-kxe' */
void option_kxe(void)
{
    option_kx();

    /* -xe : Extended frames check */
    glflags.gf_check_frames = FALSE;
    glflags.gf_check_frames_extended = TRUE;
}

/*  Option '-ky' */
void option_ky(void)
{
    suppress_print_dwarf();
    glflags.gf_check_type_offset = TRUE;
    glflags.gf_check_harmless = TRUE;
    glflags.gf_check_decl_file = TRUE;
    glflags.gf_info_flag = TRUE;
    glflags.gf_pubtypes_flag = TRUE;
    glflags.gf_check_ranges = TRUE;
    glflags.gf_check_aranges = TRUE;
}

/*  Option '-l[...]' */
void option_l_0(void)
{
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 's': option_ls(); break;
        default: usage_error = TRUE; break;
        }
    } else {
      option_l();
    }
}

/*  Option '-l' */
void option_l(void)
{
    /* Enable to suppress offsets printing */
    glflags.gf_line_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-ls' */
void option_ls(void)
{
  option_l();

  /* -ls : suppress <pc> addresses */
  glflags.gf_line_print_pc = FALSE;
}

/*  Option '-m' */
void option_m(void)
{
    glflags.gf_macinfo_flag = TRUE; /* DWARF2,3,4 */
    glflags.gf_macro_flag   = TRUE; /* DWARF5 */
    suppress_check_dwarf();
}

/*  Option '-M' */
void option_M(void)
{
    glflags.show_form_used =  TRUE;
}

/*  Option '-n' */
void option_n(void)
{
    glflags.gf_suppress_nested_name_search = TRUE;
}

/*  Option '-N' */
void option_N(void)
{
    glflags.gf_ranges_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-o[...]' */
void option_o_0(void)
{
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
        option_o();
    }
}

/*  Option '-o' */
void option_o(void)
{
    glflags.gf_reloc_flag = TRUE;
    set_all_reloc_sections_on();
}

/*  Option '-oa' */
void option_oa(void)
{
    /*  Case a has no effect, no relocations can point out
        of the abbrev section. */
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_ABBREV);
}

/*  Option '-of' */
void option_of(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_FRAME);
}

/*  Option '-oi' */
void option_oi(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_INFO);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_TYPES);
}

/*  Option '-ol' */
void option_ol(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LINE);
}

/*  Option '-oo' */
void option_oo(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LOC);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_LOCLISTS);
}

/*  Option '-op' */
void option_op(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_PUBNAMES);
}

/*  Option '-or' */
void option_or(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_ARANGES);
}

/*  Option '-oR' */
void option_oR(void)
{
    glflags.gf_reloc_flag = TRUE;
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_RANGES);
    enable_reloc_map_entry(DW_SECTION_REL_DEBUG_RNGLISTS);
}

/*  Option '-O' */
void option_O(void)
{
    /* Output filename */
    const char *path = 0;
    /*  -O file=<filename> */
    usage_error = TRUE;
    if (strncmp(dwoptarg,"file=",5) == 0) {
        path = do_uri_translation(&dwoptarg[5],"-O file=");
        if (strlen(path) > 0) {
            usage_error = FALSE;
            glflags.output_file = path;
        }
    }
}

/*  Option '-p' */
void option_p(void)
{
    glflags.gf_pubnames_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-P' */
void option_P(void)
{
    /* List of CUs per compiler */
    glflags.gf_producer_children_flag = TRUE;
}

/*  Option '-q' */
void option_q(void)
{
    /* Suppress uri-did-transate notification */
    glflags.gf_do_print_uri_in_input = FALSE;
}

/*  Option '-Q' */
void option_Q(void)
{
    /* Q suppresses section data printing. */
    glflags.gf_do_print_dwarf = FALSE;
}

/*  Option '-r' */
void option_r(void)
{
    glflags.gf_aranges_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-R' */
void option_R(void)
{
    glflags.gf_generic_1200_regs = TRUE;
}

/*  Option '-s' */
void option_s(void)
{
    glflags.gf_string_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-S' */
void option_S(void)
{
    /* 'v' option, to print number of occurrences */
    /* -S[v]match|any|regex=text*/
    if (dwoptarg[0] == 'v') {
        option_Sv();
    }

    if (strncmp(dwoptarg,"match=",6) == 0) {
        option_S_match();
    } else if (strncmp(dwoptarg,"any=",4) == 0) {
        option_S_any();
    }
#ifdef HAVE_REGEX
    else if (strncmp(dwoptarg,"regex=",6) == 0) {
        option_S_regex();
    }
#endif /* HAVE_REGEX */
    else {
        option_S_badopt();
    }
}

/*  Option '-S any=' */
void option_S_any(void)
{
    /* -S any=<text> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_any_text = makename(&dwoptarg[4]);
    const char *tempstr = remove_quotes_pair(glflags.search_any_text);
    glflags.search_any_text = do_uri_translation(tempstr,"-S any=");
    if (strlen(glflags.search_any_text) <= 0) {
        option_S_badopt();
    }
}

/*  Option '-S match=' */
void option_S_match(void)
{
    /* -S match=<text> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_match_text = makename(&dwoptarg[6]);
    const char *tempstr = remove_quotes_pair(glflags.search_match_text);
    glflags.search_match_text = do_uri_translation(tempstr, "-S match=");
    if (strlen(glflags.search_match_text) <= 0) {
        option_S_badopt();
    }
}

#ifdef HAVE_REGEX
/*  Option '-S regex=' */
void option_S_regex(void)
{
    /* -S regex=<regular expression> */
    glflags.gf_search_is_on = TRUE;
    glflags.search_regex_text = makename(&dwoptarg[6]);
    const char *tempstr = remove_quotes_pair(glflags.search_regex_text);
    glflags.search_regex_text = do_uri_translation(tempstr, "-S regex=");
    if (strlen(glflags.search_regex_text) > 0) {
        if (regcomp(glflags.search_re,
            glflags.search_regex_text,
            REG_EXTENDED)) {
            fprintf(stderr,
                "regcomp: unable to compile %s\n",
                glflags.search_regex_text);
        }
    } else {
        option_S_badopt();
    }
}
#endif /* HAVE_REGEX */

/*  Option '-Sv' */
void option_Sv(void)
{
    ++dwoptarg;
    glflags.gf_search_print_results = TRUE;
}

/*  Option '-t' */
void option_t(void)
{
    switch (dwoptarg[0]) {
    case 'a': option_ta(); break;
    case 'f': option_tf(); break;
    case 'v': option_tv(); break;
    default: usage_error = TRUE; break;
    }
}

/*  Option '-ta' */
void option_ta(void)
{
    /* all */
    glflags.gf_static_func_flag =  TRUE;
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-tf' */
void option_tf(void)
{
    /* .debug_static_func */
    glflags.gf_static_func_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-tv' */
void option_tv(void)
{
    /* .debug_static_var */
    glflags.gf_static_var_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-u' */
void option_u(void)
{
    /* compile unit */
    const char *tstr = 0;
    glflags.gf_cu_name_flag = TRUE;
    tstr = do_uri_translation(dwoptarg,"-u<cu name>");
    esb_append(glflags.cu_name,tstr);
}

/*  Option '-U' */
void option_U(void)
{
    glflags.gf_uri_options_translation = FALSE;
}

/*  Option '-v' */
void option_v(void)
{
    glflags.verbose++;
}

/*  Option '-V' */
void option_V(void)
{
    /* Display dwarfdump compilation date and time */
    print_version_details(glflags.program_fullname,TRUE);
    exit(OKAY);
}

/*  Option '-w' */
void option_w(void)
{
    /* .debug_weaknames */
    glflags.gf_weakname_flag = TRUE;
    suppress_check_dwarf();
}

/*  Option '-W[...]' */
void option_W_0(void)
{
    if (dwoptarg) {
        switch (dwoptarg[0]) {
        case 'c': option_Wc(); break;
        case 'p': option_Wp(); break;
        default: usage_error = TRUE; break;
        }
    } else {
        option_W();
    }
}

/*  Option '-W' */
void option_W(void)
{
    /* Search results in wide format */
    glflags.gf_search_wide_format = TRUE;

    /* -W : Display parent and children tree */
    glflags.gf_display_children_tree = TRUE;
    glflags.gf_display_parent_tree = TRUE;
}

/*  Option '-Wc' */
void option_Wc(void)
{
    /* -Wc : Display children tree */
    option_W();
    glflags.gf_display_children_tree = TRUE;
    glflags.gf_display_parent_tree = FALSE;
}

/*  Option '-Wp' */
void option_Wp(void)
{
    /* -Wp : Display parent tree */
    option_W();
    glflags.gf_display_children_tree = FALSE;
    glflags.gf_display_parent_tree = TRUE;
}

/*  Option '-x[...]' */
void option_x_0(void)
{
    if (strncmp(dwoptarg, "name=", 5) == 0) {
        option_x_name();
    } else if (strncmp(dwoptarg, "abi=", 4) == 0) {
        option_x_abi();
    } else if (strncmp(dwoptarg, "groupnumber=", 12) == 0) {
        option_x_groupnumber();
    } else if (strncmp(dwoptarg, "tied=", 5) == 0) {
        option_x_tied();
    } else if (strncmp(dwoptarg, "line5=", 6) == 0) {
        option_x_line5();
    } else if (strcmp(dwoptarg, "nosanitizestrings") == 0) {
        option_x_nosanitizestrings();
    } else if (strcmp(dwoptarg,"noprintsectiongroups") == 0) {
        option_x_noprintsectiongroups();
    } else {
        option_x_badopt();
    }
}

/*  Option '-x abi=' */
void option_x_abi(void)
{
    /*  -x abi=<abi> meaning select abi from dwarfdump.conf
        file. Must always select abi to use dwarfdump.conf */
    const char *abi = do_uri_translation(&dwoptarg[4],"-x abi=");
    if (strlen(abi) < 1) {
        option_x_badopt();
    } else {
        config_file_abi = abi;
    }
}

/*  Option '-x groupnumber=' */
void option_x_groupnumber(void)
{
    /*  By default prints the lowest groupnumber in the object.
        Default is  -x groupnumber=0
        For group 1 (standard base dwarfdata)
            -x groupnumber=1
        For group 1 (DWARF5 .dwo sections and dwp data)
            -x groupnumber=2 */
    long int gnum = 0;

    int res = get_number_value(dwoptarg+12,&gnum);
    if (res == DW_DLV_OK) {
        glflags.group_number = gnum;
    } else {
        option_x_badopt();
    }
}

/*  Option '-x line5=' */
void option_x_line5(void)
{
    if (strlen(dwoptarg) < 6) {
        option_x_badopt();
    } else if (!strcmp(&dwoptarg[6],"std")) {
        glflags.gf_line_flag_selection = singledw5;
    } else if (!strcmp(&dwoptarg[6],"s2l")) {
        glflags.gf_line_flag_selection= s2l;
    } else if (!strcmp(&dwoptarg[6],"orig")) {
        glflags.gf_line_flag_selection= orig;
    } else if (!strcmp(&dwoptarg[6],"orig2l")) {
        glflags.gf_line_flag_selection= orig2l;
    } else {
        option_x_badopt();
    }
}

/*  Option '-x name=' */
void option_x_name(void)
{
    /*  -x name=<path> meaning name dwarfdump.conf file. */
    const char *path = do_uri_translation(&dwoptarg[5],"-x name=");
    if (strlen(path) < 1) {
        option_x_badopt();
    } else {
        esb_empty_string(glflags.config_file_path);
        esb_append(glflags.config_file_path,path);
    }
}

/*  Option '-x noprintsectiongroups' */
void option_x_noprintsectiongroups(void)
{
    glflags.gf_section_groups_flag = FALSE;
}

/*  Option '-x nosanitizestrings' */
void option_x_nosanitizestrings(void)
{
    no_sanitize_string_garbage = TRUE;
}

/*  Option '-x tied=' */
void option_x_tied(void)
{
    const char *tiedpath = do_uri_translation(&dwoptarg[5],"-x tied=");
    if (strlen(tiedpath) < 1) {
        option_x_badopt();
    } else {
        esb_empty_string(glflags.config_file_tiedpath);
        esb_append(glflags.config_file_tiedpath,tiedpath);
    }
}

/*  Option '-y' */
void option_y(void)
{
    /* .debug_pubtypes */
    /* Also for SGI-only, and obsolete, .debug_typenames */
    suppress_check_dwarf();
    glflags.gf_pubtypes_flag = TRUE;
}

/*  Option '-z' */
void option_z(void)
{
    fprintf(stderr, "-z is no longer supported:ignored\n");
}

/*  Error message for invalid '-S' option. */
void option_S_badopt(void)
{
    fprintf(stderr,
        "-S any=<text> or -S match=<text> or"
        " -S regex=<text>\n");
    fprintf(stderr, "is allowed, not -S %s\n",dwoptarg);
    usage_error = TRUE;
}

/*  Error message for invalid '-x' option. */
void option_x_badopt(void)
{
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
