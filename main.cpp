#include <string>
using namespace std;

#include <iostream>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <libgen.h>
#include <wait.h>
#include <unistd.h>

#include "astree.h"
#include "auxlib.h"
#include "emitter.h"
#include "lyutils.h"
#include "string_set.h"

string CPP = "/usr/bin/cpp -nostdinc";
constexpr size_t LINESIZE = 1024;
// string cpp_command;

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}

void cpp_popen (const char* filename) {
   CPP = CPP + " " + filename;
   yyin = popen (CPP.c_str(), "r");
   if (yyin == nullptr) {
      syserrprintf (CPP.c_str());
      exit (exec::exit_status);
   }else {
      if (yy_flex_debug) {
         fprintf (stderr, "-- popen (%s), fileno(yyin) = %d\n",
                  CPP.c_str(), fileno (yyin));
      }
      lexer::newfilename (CPP);
   }
}

void cpp_pclose() {
   int pclose_rc = pclose (yyin);
   eprint_status (CPP.c_str(), pclose_rc);
   if (pclose_rc != 0) exec::exit_status = EXIT_FAILURE;
}

void scan_opts (int argc, char** argv) {
   opterr = 0;
   yy_flex_debug = 0;
   yydebug = 0;
   lexer::interactive = isatty (fileno (stdin))
                    and isatty (fileno (stdout));
   for(;;) {
      int opt = getopt (argc, argv, "@:lyD:");
      if (opt == EOF) break;
      switch (opt) {
         case '@': set_debugflags (optarg);                 break;
         case 'l': yy_flex_debug = 1;                       break;
         case 'y': yydebug = 1;                             break;
         case 'D': CPP = CPP + " -D" + optarg;              break;
         default:  errprintf ("bad option (%c)\n", optopt); break;
      }
   }
   if (optind > argc) {
      errprintf ("Usage: %s [-ly] [filename]\n",
                 exec::execname.c_str());
      exit (exec::exit_status);
   }
   const char* filename = optind == argc ? "-" : argv[optind];
   cpp_popen (filename);
}

int main (int argc, char** argv) {
   exec::execname = basename (argv[0]);
   if (yydebug or yy_flex_debug) {
      fprintf (stderr, "Command:");
      for (char** arg = &argv[0]; arg < &argv[argc]; ++arg) {
            fprintf (stderr, " %s", *arg);
      }
      fprintf (stderr, "\n");
   }
   scan_opts (argc, argv);
   int parse_rc = yyparse();
   cpp_pclose();
   yylex_destroy();
   if (yydebug or yy_flex_debug) {
      fprintf (stderr, "Dumping parser::root:\n");
      if (parser::root != nullptr) parser::root->dump_tree (stderr);
      fprintf (stderr, "Dumping string_set:\n");
      string_set::dump (stderr);
   }
   if (parse_rc) {
      errprintf ("parse failed (%d)\n", parse_rc);
   }else {
      // astree::print (stdout, parser::root);
      // if (parser::root != nullptr) parser::root->dump_tree (stderr);
      // cout << "asdfdsfs" << endl;
      
      // Write .str file --------------------------------------------//
      string str = static_cast<string>(basename(argv[optind])) + ".str";
      FILE* str_out = fopen(str.c_str(), "w");
      string_set::dump (str_out);
      fclose(str_out);
      //-------------------------------------------------------------//

      // Write .tok file --------------------------------------------//
      string tok = static_cast<string>(basename(argv[optind])) + ".tok";
      FILE* tok_out = fopen(tok.c_str(), "w");
      lexer::printtokens(tok_out);
      fclose(tok_out);
      lexer::delete_tokens();
      //-------------------------------------------------------------//
      
      //emit_sm_code (parser::root);
      delete parser::root;
   }
   
   return exec::exit_status;
}