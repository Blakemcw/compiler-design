// $Id: lyutils.cpp,v 1.6 2019-04-18 13:35:11-07 - - $

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h> // for basename
#include <algorithm> // for find

#include "auxlib.h"
#include "lyutils.h"

bool lexer::interactive = true;
location lexer::lloc = {0, 1, 0};
size_t lexer::last_yyleng = 0;
vector<string> lexer::filenames;

astree* parser::root = nullptr;
vector<astree*> tokens;

// Option -D: variables
string include_file;
int include_file_index;

const string* lexer::filename (int filenr) {
   return &lexer::filenames.at(filenr);
}

void lexer::newfilename (const string& filename) {
   lexer::lloc.filenr = lexer::filenames.size();
   lexer::filenames.push_back (filename);
}

void lexer::advance() {
   if (not interactive) {
      if (lexer::lloc.offset == 0) {
         printf (";%2zd.%3zd: ",
                 lexer::lloc.filenr, lexer::lloc.linenr);
      }
      printf ("%s", yytext);
   }
   lexer::lloc.offset += last_yyleng;
   last_yyleng = yyleng;
}

void lexer::newline() {
   ++lexer::lloc.linenr;
   lexer::lloc.offset = 0;
}

void lexer::badchar (unsigned char bad) {
   char buffer[16];
   snprintf (buffer, sizeof buffer,
             isgraph (bad) ? "%c" : "\\%03o", bad);
   errllocprintf (lexer::lloc, "invalid source character (%s)\n",
                  buffer);
}


void lexer::include() {
   size_t linenr;
   static char filename[0x1000];
   assert (sizeof filename > strlen (yytext));
   int scan_rc = sscanf (yytext, "# %zu \"%[^\"]\"", &linenr, filename);
   if (scan_rc != 2) {
      errprintf ("%s: invalid directive, ignored\n", yytext);
   } else {
      if (yy_flex_debug) {
         fprintf (stderr, "--included # %zd \"%s\"\n",
                  linenr, filename);
      }

      // Creates a node for an include file for writing to .tok file.
      yylval = new astree (-1, lexer::lloc, filename);
      tokens.push_back(yylval);

      lexer::lloc.linenr = linenr - 1;
      lexer::newfilename (filename);
   }
}

int lexer::token (int symbol) {
   yylval = new astree (symbol, lexer::lloc, yytext);
   tokens.push_back(yylval);
   return symbol;
}

int lexer::badtoken (int symbol) {
   errllocprintf (lexer::lloc, "invalid token (%s)\n", yytext);
   return lexer::token (symbol);
}

void lexer::printtokens(FILE* outfile) {
   for (auto x: tokens) {
      // Commented below is to deal with option -D:
      // its not included because i need to ask questions in class,
      //    1. should the include file tokens still be written to .tok?
      //       - in other words, are we just excluding debugging information?
      // if (filenames[x->lloc.filenr].find(include_file) != std::string::npos) {
      //    continue;
      // } else

      if (x->symbol == -1) { // Sentinel value means its an include statement
         fprintf(outfile, "# %2u \"%-10s\"\n",
         x->lloc.filenr,
         x->lexinfo->data());
      } else { // Otherwise it is a token
         // Format follows,
         // file# line#.offset tokenId tokenClass tokenText
         fprintf(outfile, "  %2u %2u.%-.3u  %3i  %-10s  %-10s\n",
         x->lloc.filenr,
         x->lloc.linenr,
         x->lloc.offset,
         x->symbol,
         parser::get_tname(x->symbol),
         x->lexinfo->data());
      }
   }
}

void lexer::delete_tokens(){
   // Used to delete tokens stored for .tok file.
   for (auto token: tokens)
   {
      delete token;
   }
}

void yyerror (const char* message) {
   assert (not lexer::filenames.empty());
   errllocprintf (lexer::lloc, "%s\n", message);
}
