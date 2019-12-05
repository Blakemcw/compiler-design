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

inline bool exists (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

string get_suffix(string filename) {
   // Finds last period from end of filename and gets ".[suffix]"
   string suffix = "";
   for (int i = filename.length(); i > -1; --i) {
      if ('.' == filename[i]){
         return "." + suffix;
      }
      suffix = filename[i] + suffix;
   }
   return "";
}

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
   opterr        = 0;
   yy_flex_debug = 0;
   yydebug       = 0;
   
   lexer::interactive = isatty (fileno (stdin))
                    and isatty (fileno (stdout));

   for(;;) {
      int opt = getopt (argc, argv, "@:lyD:");
      if (opt == EOF) break;
      switch (opt) {
         case '@': set_debugflags (optarg);                 break;
         case 'l': yy_flex_debug = 1;                       break;
         case 'y': yydebug       = 1;                       break;
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

   // Check if program has .oc suffix
   if (strcmp(get_suffix(basename(argv[optind])).c_str(), ".oc") != 0){
      errprintf ("File: '%s' should have suffix '.oc'\n", 
                 argv[optind]);
      exit (exec::exit_status);
   }
   
   cpp_popen (filename);
}

int main (int argc, char** argv) {
   exec::execname = basename (argv[0]);
   if (yydebug and yy_flex_debug) {
      fprintf (stderr, "Command:");
      for (char** arg = &argv[0]; arg < &argv[argc]; ++arg) {
            fprintf (stderr, " %s", *arg);
      }
      fprintf (stderr, "\n");
   }

   if (!exists(argv[argc-1])) {
      errprintf ("No such file '%s'.\n", argv[argc-1]);
      exit(1);
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
   } else {

      // Get .oc filename without extenstion ------------------------//
      string oc_filename = static_cast<string>(basename(argv[optind]));
      oc_filename = oc_filename.substr(0, oc_filename.length() - 3);

      // Write .str file --------------------------------------------//
      string str = oc_filename + ".str";
      FILE*  str_out = fopen(str.c_str(), "w");
      string_set::dump (str_out);
      fclose (str_out);

      // Write .tok file --------------------------------------------//
      string tok = oc_filename + ".tok";
      FILE*  tok_out = fopen(tok.c_str(), "w");
      lexer::printtokens (tok_out);
      fclose (tok_out);

      // Write .sym file --------------------------------------------//
      string sym = oc_filename + ".sym";
      FILE*  sym_out = fopen(sym.c_str(), "w");
      emit_sm_code(parser::root, sym_out);
      fclose (sym_out);

      // Write .ast file --------------------------------------------//
      string ast = oc_filename + ".ast";
      FILE*  ast_out = fopen(ast.c_str(), "w");
      if (parser::root != nullptr) 
         parser::root->print(ast_out, parser::root);
      fclose (ast_out);

      // Clean up ---------------------------------------------------//
      delete parser::root;
      lexer::delete_tokens();
   }

   return exec::exit_status;
}
