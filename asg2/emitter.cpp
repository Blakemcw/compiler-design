// $Id: emitter.cpp,v 1.5 2017-10-05 16:39:39-07 - - $

#include <assert.h>
#include <stdio.h>

#include "astree.h"
#include "emitter.h"
#include "auxlib.h"
#include "lyutils.h"

void emit (astree* root);

void emit_insn (const char* opcode, const char* operand, astree* tree) {
   printf ("%-10s%-10s%-20s; %s %zd.%zd\n", "",
            opcode, operand,
            lexer::filename (tree->lloc.filenr)->c_str(),
            tree->lloc.linenr, tree->lloc.offset);
}

void postorder (astree* tree) {
   assert (tree != nullptr);
   for (size_t child = 0; child < tree->children.size(); ++child) {
      emit (tree->children.at(child));
   }
}

void postorder_emit_stmts (astree* tree) {
   postorder (tree);
}

void postorder_emit_oper (astree* tree, const char* opcode) {
   postorder (tree);
   emit_insn (opcode, "", tree);
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


void emit (astree* tree) {
   switch (tree->symbol) {
      case TOK_ROOT  : postorder_emit_stmts (tree);       break;
      case ';'       : postorder_emit_semi (tree);        break;
      case '='       : emit_assign (tree);                break;
      case '+'       : postorder_emit_oper (tree, "add"); break;
      case '-'       : postorder_emit_oper (tree, "sub"); break;
      case '*'       : postorder_emit_oper (tree, "mul"); break;
      case '/'       : postorder_emit_oper (tree, "div"); break;
      case '^'       : postorder_emit_oper (tree, "pow"); break;
      //case TOK_POS   : postorder_emit_oper (tree, "pos"); break;
      //case TOK_NEG   : postorder_emit_oper (tree, "neg"); break;
      case TOK_IDENT : emit_push (tree, "pushvar");       break;
      // case TOK_NUMBER: emit_push (tree, "pushnum");       break;
      default    : assert (false);                        break;
   }
}

void emit_sm_code (astree* tree) {
   printf ("\n");
   if (tree) emit (tree);
}

