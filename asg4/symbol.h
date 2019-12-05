#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <bitset>
#include <unordered_map>
#include <vector>

using namespace std;



// AST ATTRIBUTES ---------------------------------------------------//

enum attr {
   VOID, INT, NULLPTR_T, STRING, STRUCT, ARRAY, FUNCTION, VARIABLE,
   FIELD, TYPEID, PARAM, LOCAL, LVAL, CONST, VREG, VADDR, BITSET_SIZE,
};
using attr_bitset = bitset<attr::BITSET_SIZE>;

struct location {
   size_t filenr;
   size_t linenr;
   size_t offset;
};

// SYMBOL TABLES ----------------------------------------------------//


struct symbol {
   attr_bitset attributes;
   size_t sequence;
   unordered_map<string*,symbol*>* fields;
   location lloc;
   size_t block_number;
   vector<symbol*>* parameters;
};

// BITSET FUNCTIONS -------------------------------------------------//

string attributes_to_string (symbol* sym);

#endif