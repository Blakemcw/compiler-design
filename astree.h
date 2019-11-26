// $Id: astree.h,v 1.10 2016-10-06 16:42:35-07 - - $

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <bitset>

using namespace std;

#include "auxlib.h"
#include "symbol.h"


using symbol_table = unordered_map<string*,symbol*>;
using symbol_entry = symbol_table::value_type;

struct astree {

   // Fields.
   int symbol;               // token code
   location lloc;            // source location
   const string* lexinfo;    // pointer to lexical information
   vector<astree*> children; // children of this n-way node
   // New fields for asg4:
   attr_bitset att;
   astree* parent = nullptr;
   string attributes = "";
   string declaration = "";
   symbol_table* scope_table = nullptr;
   int block = 0;

   // Functions.
   astree (int symbol, const location&, const char* lexinfo);
   ~astree();
   astree* adopt (astree* child1, astree* child2 = nullptr);
   astree* adopt_sym (astree* child, int symbol);
   astree* change_symbol (int symbol);
   astree* clear_lexinfo();
   astree* copy_lloc(astree* that);
   void dump_node (FILE*);
   void dump_tree (FILE*, int depth = 0);
   static void dump (FILE* outfile, astree* tree);
   static void print (FILE* outfile, astree* tree, int depth = 0);
};

void print_hierarchy_lines(FILE* outfile, int depth);
void destroy (astree* tree1, astree* tree2 = nullptr);

void errllocprintf (const location&, const char* format, const char*);

#endif

