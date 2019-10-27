%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

%}

%option 8bit
%option debug
%option nobackup
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn
/*%option verbose*/

/* SPECIAL SYMBOLS */
EQ              (==)
NE              (!=)
LT              (<)
LE              (<=)
GT              (>)
GE              (>=)
ARROW           (->)

/* RESERVED WORDS */
IF              (if)
ELSE            (else)
WHILE           (while)
RETURN          (return)
ALLOC           (alloc)
NULLPTR         (nullptr)
NOT             (not)
INT             (int)
STRING          (string)
STRUCT          (struct)
ARRAY           (array)
PTR             (ptr)
VOID            (void)

/* IDENTIFIERS/NUMBERS */
LETTER          [A-Za-z_]
DIGIT           [0-9]
MANTISSA        ({DIGIT}+\.?{DIGIT}*|\.{DIGIT}+)
EXPONENT        ([Ee][+-]?{DIGIT}+)
NUMBER          ({MANTISSA}{EXPONENT}?)
NOTNUMBER       ({MANTISSA}[Ee][+-]?)
IDENT           ({LETTER}({LETTER}|{DIGIT})*)

/* CONSTANTS */
CHARCON         (’([^\\’\n]|\\[\\’"0nt])’)
STRINGCON       (\"([^\\"\n]|\\[\\’"0nt])*\")

%%

"#".*           { lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }

{EQ}            { return lexer::token (TOK_EQ);    }
{NE}            { return lexer::token (TOK_NE);    }
{LT}            { return lexer::token (TOK_LT);    }
{LE}            { return lexer::token (TOK_LE);    }
{GT}            { return lexer::token (TOK_GT);    }
{GE}            { return lexer::token (TOK_GE);    }
{ARROW}         { return lexer::token (TOK_ARROW); }

{IF}            { return lexer::token (TOK_IF);      }
{ELSE}          { return lexer::token (TOK_ELSE);    }
{WHILE}         { return lexer::token (TOK_WHILE);   }
{RETURN}        { return lexer::token (TOK_RETURN);  }
{ALLOC}         { return lexer::token (TOK_ALLOC);   }
{NULLPTR}       { return lexer::token (TOK_NULLPTR); }
{NOT}           { return lexer::token (TOK_NOT);     }
{INT}           { return lexer::token (TOK_INT);     }
{STRING}        { return lexer::token (TOK_STRING);  }
{STRUCT}        { return lexer::token (TOK_STRUCT);  }
{ARRAY}         { return lexer::token (TOK_ARRAY);   }
{PTR}           { return lexer::token (TOK_PTR);     }
{VOID}          { return lexer::token (TOK_VOID);    }

{NUMBER}        { return lexer::token (TOK_NUMBER); }
{IDENT}         { return lexer::token (TOK_IDENT);  }

{CHARCON}       { return lexer::token (TOK_CHARCON);   }
{STRINGCON}     { return lexer::token (TOK_STRINGCON); }

"="             { return lexer::token ('='); }
"+"             { return lexer::token ('+'); }
"-"             { return lexer::token ('-'); }
"*"             { return lexer::token ('*'); }
"/"             { return lexer::token ('/'); }
"^"             { return lexer::token ('^'); }
"("             { return lexer::token ('('); }
")"             { return lexer::token (')'); }
"{"             { return lexer::token ('{'); }
"}"             { return lexer::token ('}'); }
","             { return lexer::token (','); }
";"             { return lexer::token (';'); }

{NOTNUMBER}     { return lexer::badtoken (TOK_NUMBER); }
.               { lexer::badchar (*yytext); }

%%