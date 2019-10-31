// $Id: lyutils.h,v 1.8 2019-05-03 10:12:22-07 - - $

#ifndef __UTILS_H__
#define __UTILS_H__

// Lex and Yacc interface utility.

#include <string>
#include <vector>
using namespace std;

#include <stdio.h>

#include "astree.h"
#include "auxlib.h"

#define YYEOF 0

extern FILE* yyin;
extern char* yytext; 
extern int yy_flex_debug;
extern int yydebug;
extern int yyleng; 

int yylex();
int yylex_destroy();
int yyparse();
void yyerror (const char* message);

struct lexer {
   static bool interactive;
   static location lloc;
   static size_t last_yyleng;
   static vector<string> filenames;
   static const string* filename (int filenr);
   static void newfilename (const string& filename);
   static void advance();
   static void newline();
   static void badchar (unsigned char bad);
   static void include();
   static int token (int symbol);
   static int badtoken (int symbol);
   static void printtokens (FILE* outfile);
   static void delete_tokens();
};

struct parser {
   static astree* root;
   static const char* get_tname (int symbol);
   static astree* create_node (void);
};

#define YYSTYPE_IS_DECLARED
typedef astree* YYSTYPE;
#include "yyparse.h"

#endif

