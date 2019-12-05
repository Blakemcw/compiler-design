// $Id: emitter.cpp,v 1.5 2017-10-05 16:39:39-07 - - $

#include <string>
#include <assert.h>
#include <stdio.h>
#include <bitset>
#include <unordered_map>

#include "astree.h"
#include "emitter.h"
#include "auxlib.h"
#include "lyutils.h"
#include "string_set.h"

using symbol_table = unordered_map<string*,symbol*>;
using symbol_entry = symbol_table::value_type;


// SYMBOL TABLES ----------------------------------------------------//

// These variables are used in the handle_function method
static int next_block = 1;
static int local_variables_sequence = 0;
static bool is_local_scope = false;
static FILE* symfile;


// TYPE NAMES //
symbol_table type_names;
symbol_table* current_field_table;


// FUNCTION AND VARIABLE NAMES //
/* The first symbol table in the group is all global variables and
 * functions. All symbol tables from index [1] and on are symbol 
 * tables holding local variables indexed by block number.
 */
vector<symbol_table*> identifiers;
symbol_table* globals = new symbol_table{};

// SYMBOL LOOKUP FUNCTIONS -------------------------------------------------//

bool is_local_var (string* name) {
   return identifiers.back()->find(name) != identifiers.back()->end();
}

bool is_global_var (string* name) {
   return identifiers.front()->find(name) != identifiers.front()->end();
}

void allocate_struct (symbol* sym, astree* struc_name) {
   string* name = const_cast<string*>(struc_name->lexinfo);
   if (type_names.find(name) != type_names.end()) {
      sym->fields = type_names.at(name)->fields;
      sym->attributes.set(attr::STRUCT);
   } else {
      fprintf(symfile, "Type %s does not exist.", name->c_str());
   }
}

symbol* lookup_symbol_from_node(astree* node) {
   if (node->parent->symbol == TOK_ARROW &&
       node->parent->children[0] != node) { // If ident is a field
      // Get arrows arguments
      astree* struc = node->parent->children.at(0);
      astree* field = node->parent->children.at(1);

      // Get string pointers to lexinfo
      string* struc_name = const_cast<string*>(struc->lexinfo);
      string* field_name = const_cast<string*>(field->lexinfo);
      // Do lookups
      if (is_local_var(struc_name)) {
         symbol* temp = identifiers.back()->at(struc_name);
         return temp->fields->at(field_name);
      } else if (is_global_var(struc_name)) {
         return identifiers.front()->at(
            struc_name)->fields->at(field_name);
      }
   } else if (node->parent->symbol == TOK_INDEX) 
      { // if ident is a local variable
      // Get arrays arguments
      astree* array = node->parent->children.at(0);
      astree* index = node->parent->children.at(1);

      string* array_name = const_cast<string*>(array->lexinfo);
      string* index_name = const_cast<string*>(index->lexinfo);

      if (is_local_var(array_name)) {
         return identifiers.back()->at(array_name);
      } else if (is_global_var(array_name)) {
         return identifiers.front()->at(array_name);
      }

   } else { // Is a primitive local or global variable
      string* ident_name = const_cast<string*>(node->lexinfo);
      if (is_local_var(ident_name)) {
         return identifiers.back()->at(ident_name);
      } else if (is_global_var(ident_name)) {
         return identifiers.front()->at(ident_name);
      } else { // Type name
         return type_names.at(ident_name);
      }
   }
   return nullptr;
}

void update_ast(astree* ast, symbol* sym = nullptr) {
   if (sym != nullptr) { // If the ast has a corresponding symbol
      ast->attributes = attributes_to_string(sym);
      ast->block = sym->block_number;

      if (ast->symbol == TOK_FUNCTION || ast->symbol == TOK_STRUCT) {
         ast->scope_table = identifiers.front();
      } else if (sym->attributes[attr::FIELD]) {
         ast->scope_table = current_field_table;
      } else if (is_local_scope) {
         ast->scope_table = identifiers.back();
      } else {
         ast->scope_table = identifiers.front();
      }
   } else {
      ast->block = next_block;
      if (is_local_scope) {
         ast->scope_table = identifiers.back();
      } else {
         ast->scope_table = identifiers.front();
      }
      if (ast->symbol == TOK_IDENT) {
         symbol* lookup_symbol = lookup_symbol_from_node(ast);
         
         location decl = lookup_symbol->lloc;
         char buff[100];
         snprintf(buff, sizeof(buff), "(%zd.%zd.%zd)", 
            decl.filenr,
            decl.linenr,
            decl.offset);
         string declaration = buff;
         ast->declaration = declaration;
         ast->attributes = attributes_to_string(lookup_symbol);
      }
   }
}

// EMIT FUNCTIONS ---------------------------------------------------//

void emit (astree* root);

void emit_insn (const char* opcode, const char* operand, astree* tree){
   printf ("%-10s%-10s%-20s; %s %zd.%zd\n", "",
            opcode, operand,
            lexer::filename (tree->lloc.filenr)->c_str(),
            tree->lloc.linenr, tree->lloc.offset);
}

void postorder (astree* tree) {
   assert (tree != nullptr);
   for (size_t child = 0; child < tree->children.size(); ++child) {
      update_ast(tree->children.at(child));
      emit (tree->children.at(child));
   }
}

void postorder_emit_stmts (astree* tree) {
   postorder (tree);
}

void postorder_emit_oper (astree* tree, const char* opcode) {
   postorder (tree);
}

void postorder_emit_semi (astree* tree) {
   postorder (tree);
   emit_insn ("", "", tree);
}

void emit_push (astree* tree, const char* opcode) {
   emit_insn (opcode, tree->lexinfo->c_str(), tree);
}

void emit_assign (astree* tree) {
   assert (tree->children.size() == 2);
   astree* left = tree->children.at(0);
   emit (tree->children.at(1));
   if (left->symbol != TOK_IDENT) {
      errllocprintf (left->lloc, "%s\n",
                    "left operand of '=' not an identifier");
   }else{
      emit_insn ("popvar", left->lexinfo->c_str(), left);
   }
}

// ENCOUNTER FUNCTION TOKEN -----------------------------------------//

/*
 * When we enter a function, variables should have local scope so
 * we use a variable is_local_scope to keep track of where vars are
 * declared.
 */


void print_param (string* name, symbol* param_sym, FILE* outfile) {
   fprintf(outfile, "   %s (%i.%i.%i) {%i} %s%i\n",
            name->c_str(),
            param_sym->lloc.filenr,
            param_sym->lloc.linenr,
            param_sym->lloc.offset,
            param_sym->block_number,
            attributes_to_string(param_sym).c_str(),
            param_sym->sequence);
}

void print_function (string* name, symbol* function_sym,FILE* outfile){
   fprintf(outfile, "%s (%i.%i.%i) {%i} %s\n",
            name->c_str(),
            function_sym->lloc.filenr,
            function_sym->lloc.linenr,
            function_sym->lloc.offset,
            function_sym->block_number,
            attributes_to_string(function_sym).c_str());
}

bool is_prototype(astree* func) {
   return func->children.size() < 3;
}

int get_type_attr (int tok_symbol) {
   switch (tok_symbol) {
      case TOK_VOID     :  return attr::VOID;
      case TOK_INT      :  return attr::INT;
      case TOK_STRING   :  return attr::STRING;
      case TOK_NULLPTR  :  return attr::NULLPTR_T;
      case TOK_ARRAY    :  return attr::ARRAY;
      case TOK_PTR      :  return attr::VADDR;
      default           :  return 0;
   }
}

vector<symbol*>* get_params (astree* params_root) {
   vector<symbol*>* params = new vector<symbol*>;
   
   // Keeps track of the sequence of parameters
   size_t param_index = 0;

   for (astree* param: params_root->children) {
      // Define fields for param symbol
      astree* type = param->children[0];
      astree* id   = param->children[1];

      symbol* param_symbol = new symbol();
      param_symbol->attributes.set(get_type_attr(type->symbol));
      param_symbol->attributes.set(attr::VARIABLE);
      param_symbol->attributes.set(attr::PARAM);
      param_symbol->block_number = next_block;
      param_symbol->lloc = param->lloc;
      param_symbol->sequence = param_index;

      //Handle if array
      if (type->symbol == TOK_ARRAY) {
         param_symbol->attributes.set(attr::ARRAY);
         param_symbol->attributes.set(
            get_type_attr(
               type->children[0]->symbol));
      } else if (type_names.find(
         const_cast<string*>(id->lexinfo)) != type_names.end()) {
         allocate_struct(param_symbol, id);
      }

      // Insert param into local symbol table
      string* param_name = const_cast<string*>(id->lexinfo);
      symbol_entry param_entry(param_name, param_symbol);
      identifiers.back()->insert(param_entry);

      print_param(param_name, param_symbol, symfile);

      // Add to list of params to return to function
      params->push_back(param_symbol);
      param_index++;

      update_ast(id, param_symbol);
   }

   if (param_index > 0) {
      fprintf(symfile, "\n");
   }

   return params;
}

void handle_func (astree* tree) {
   // Set globals
   is_local_scope = true; // enter function block, leave global scope
   // Create a new local symbol table for the function block.
   symbol_table* locals = new symbol_table{};
   identifiers.push_back(locals);

   // Define variables
   symbol_table local_vars;
   symbol* func = new symbol{};
   
   // Get Type ID
   astree* type_id = tree->children[0];
   astree* type = type_id->children[0];
   astree* id   = type_id->children[1];
   
   // Get Params
   astree* params = tree->children[1];

   // Set function symbols fields
   func->block_number = 0;
   func->lloc         = tree->lloc;
   func->attributes.set(attr::FUNCTION);
   func->attributes.set(get_type_attr(type->symbol));

   // Insert function into global
   string* function_name = const_cast<string*>(id->lexinfo);
   symbol_entry function_entry(function_name, func);
   identifiers.front()->insert(function_entry);

   print_function(function_name, func, symfile);
   func->parameters = get_params(params);

   if (not (is_prototype(tree))) postorder(tree->children[2]);
   fprintf(symfile, "\n");

   update_ast(id, func);
   update_ast(tree, func);

   next_block++;
   local_variables_sequence = 0; // reset local var count
   is_local_scope = false; // exit function block, back to global scope
}


// ENCOUNTER STRUCT TOKEN -------------------------------------------//

void print_struct (string* name, symbol* struct_sym, FILE* outfile){
   fprintf(outfile, "%s (%i.%i.%i) {%i} %s%s\n",
            name->c_str(),
            struct_sym->lloc.filenr,
            struct_sym->lloc.linenr,
            struct_sym->lloc.offset,
            struct_sym->block_number,
            attributes_to_string(struct_sym).c_str(),
            name->c_str());
}

void print_field (string* name, symbol* field_sym, FILE* outfile) {
   fprintf(outfile, "   %s (%i.%i.%i) %s%i\n",
               name->c_str(),
               field_sym->lloc.filenr,
               field_sym->lloc.linenr,
               field_sym->lloc.offset,
               attributes_to_string(field_sym).c_str(),
               field_sym->sequence
            );
}

symbol_table* get_fields (astree* struct_tree) {
   symbol_table* fields = new symbol_table{};
   current_field_table = fields;

   for (size_t field = 1; field < struct_tree->children.size();
          ++field) {
      // Keeps track of the sequence of fields
      size_t current_sequence = field - 1;
      astree* curr_field = struct_tree->children.at(field);

      // Define fields for field symbol
      symbol* field_symbol = new symbol();
      field_symbol->attributes.set(attr::FIELD);
      field_symbol->block_number = 0;
      field_symbol->lloc = curr_field->lloc;
      field_symbol->sequence = current_sequence;

      // Insert field into table
      string* field_name = const_cast<string*>(
         curr_field->children[1]->lexinfo);
      symbol_entry field_entry(field_name, field_symbol);
      fields->insert(field_entry);

      // Print field
      print_field(field_name, field_symbol, symfile);
      update_ast(curr_field, field_symbol);
   }

   return fields;
}

void handle_struct (astree* struct_tree) {
   // Define variables
   symbol_table fields;
   symbol* struct_sym = new symbol{};

   // Set struct symbol fields
   struct_sym->attributes.set(attr::STRUCT);
   struct_sym->block_number = 0;
   struct_sym->lloc     = struct_tree->lloc;
   struct_sym->sequence = 0;

   // Insert struct into global symbol table
   string* struct_name = const_cast<string*>(
      struct_tree->children[0]->lexinfo);
   symbol_entry struct_entry(struct_name, struct_sym);
   type_names.insert(struct_entry);

   // Print struct out
   print_struct(struct_name, struct_sym, symfile);

   // We declare the structs fields after insertion
   // because a field might have the type of the struct
   struct_sym->fields = get_fields(struct_tree);

   // Add info to struct nodes in AST
   update_ast(struct_tree, struct_sym);

   fprintf(symfile, "\n");
}

// ENCOUNTER A VARIABLE DECLARATION ---------------------------------//

void handle_declaration (astree* type_id) {
   symbol* variable_symbol = new symbol{};

   astree* type = type_id->children[0];
   astree* id   = type_id->children[1];

   variable_symbol->attributes.set(attr::VARIABLE);
   variable_symbol->attributes.set(attr::LOCAL);
   variable_symbol->attributes.set(get_type_attr(type->symbol));
   variable_symbol->block_number = next_block;
   variable_symbol->lloc = type_id->lloc;

   if (type_id->children.size() == 3) {
      astree* val = type_id->children.at(2);
      if (val->symbol == TOK_ALLOC) {
         if (val->children[0]->symbol == TOK_IDENT) {
            allocate_struct(variable_symbol, type->children.at(0));
         } else if (val->children[0]->symbol == TOK_ARRAY) {
            variable_symbol->attributes.set(attr::ARRAY);
            variable_symbol->attributes.set(
               get_type_attr(
                  val->children[0]->children[0]->symbol));
         }
      }
   }

   string* variable_name = const_cast<string*>(id->lexinfo);
   symbol_entry struct_entry(variable_name, variable_symbol);
   
   if (is_local_scope) {
      variable_symbol->sequence = local_variables_sequence;
      identifiers.back()->insert(struct_entry);
      local_variables_sequence++;
   } else {
      variable_symbol->sequence = 0;
      identifiers.front()->insert(struct_entry);
   }
   
   postorder(type_id);
   update_ast(id, variable_symbol);

   print_param(variable_name, variable_symbol, symfile);
}

// ATTRIBUTES FOR OPERATORS -----------------------------------------//
void postorder_op_attr(astree* tree) {
   tree->attributes = "int vreg ";
   postorder(tree);
}

// MAIN EMITTER FUNCTION --------------------------------------------//

void emit (astree* tree) {
   switch (tree->symbol) {
      case ';'             : postorder_emit_stmts (tree);      break;
      case TOK_IDENT       : postorder_emit_stmts (tree);      break;

      // BINARY OPERATORS ------------------------------------------//
      case '='             : postorder_op_attr    (tree);      break;
      case '+'             : postorder_op_attr    (tree);      break;
      case '-'             : postorder_op_attr    (tree);      break;
      case '*'             : postorder_op_attr    (tree);      break;
      case '/'             : postorder_op_attr    (tree);      break;
      case '^'             : postorder_op_attr    (tree);      break;
      case '%'             : postorder_op_attr    (tree);      break;
      case TOK_INDEX       : postorder_emit_stmts (tree);      break;
      case TOK_ARROW       : postorder_emit_stmts (tree);      break;
      case TOK_EQ          : postorder_op_attr    (tree);      break;
      case TOK_NE          : postorder_op_attr    (tree);      break;
      case TOK_LT          : postorder_op_attr    (tree);      break;
      case TOK_LE          : postorder_op_attr    (tree);      break;
      case TOK_GT          : postorder_op_attr    (tree);      break;
      case TOK_GE          : postorder_op_attr    (tree);      break;

      // UNARY OPERATORS -------------------------------------------//
      case TOK_POS         : postorder_op_attr    (tree);      break;
      case TOK_NEG         : postorder_op_attr    (tree);      break;
      case TOK_NOT         : postorder_op_attr    (tree);      break;
      case TOK_ALLOC       : postorder_emit_stmts (tree);      break;
      
      // SYNTHETIC SYMBOLS -----------------------------------------//
      case TOK_ROOT        : /*------- ENTER HERE --------*/ 
                             identifiers.push_back(globals);
                             postorder_emit_stmts (tree);      break;
      case TOK_FUNCTION    : handle_func          (tree);      break;
      case TOK_TYPE_ID     : handle_declaration   (tree);      break;
      case TOK_PARAM       : postorder_emit_stmts (tree);      break;
      case TOK_BLOCK       : postorder_emit_stmts (tree);      break;

      // CONSTANTS -------------------------------------------------//
      case TOK_INTCON      : postorder_emit_stmts (tree);      break;
      case TOK_CHARCON     : postorder_emit_stmts (tree);      break;
      case TOK_STRINGCON   : postorder_emit_stmts (tree);      break;
      case TOK_CALL        : postorder_emit_stmts (tree);      break;
      
      // CONTROL FLOW ----------------------------------------------//
      case TOK_IF          : postorder_emit_stmts (tree);      break;
      case TOK_ELSE        : postorder_emit_stmts (tree);      break;
      case TOK_WHILE       : postorder_emit_stmts (tree);      break;
      case TOK_RETURN      : postorder_emit_stmts (tree);      break;
      case TOK_NULLPTR     : postorder_emit_stmts (tree);      break;
      
      // TYPES -----------------------------------------------------//
      case TOK_INT         : postorder_emit_stmts (tree);      break;
      case TOK_STRING      : postorder_emit_stmts (tree);      break;
      case TOK_STRUCT      : handle_struct        (tree);      break;
      case TOK_ARRAY       : postorder_emit_stmts (tree);      break;
      case TOK_PTR         : postorder_emit_stmts (tree);      break;
      case TOK_VOID        : postorder_emit_stmts (tree);      break;

      // UNDEFINED SYMBOL ------------------------------------------//
      default              : assert (false);                   break;
   }
}

void emit_sm_code (astree* tree, FILE* outfile) {
   symfile = outfile;
   if (tree) emit (tree);
}

