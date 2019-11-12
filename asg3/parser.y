%{
// $Id: parser.y,v 1.22 2019-04-23 14:07:52-07 - - $

#include <cassert>
#include <iostream>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%token TOK_VOID TOK_INT TOK_STRING TOK_NUMBER
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULLPTR TOK_ARRAY TOK_ARROW TOK_ALLOC TOK_PTR
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE TOK_NOT
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON
%token TOK_ROOT TOK_BLOCK TOK_CALL

%token TOK_TYPE_ID TOK_FUNCTION TOK_PARAM
%token TOK_POS TOK_NEG TOK_INDEX

%right TOK_IF TOK_ELSE
%right '='
%left  TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left  '+' '-'
%left  '*' '/' '%'
%left  TOK_POS TOK_NEG TOK_NOT
%left  '[' TOK_ARROW TOK_CALL TOK_ALLOC

%start start

%%

/* Grammar */


start          : program { 
                     parser::root = $1;}
               ;
program        : program structdef { 
                     $$ = $1->adopt ($2);}
               | program function { 
                     $$ = $1->adopt ($2);}
               | program statement { 
                     $$ = $1->adopt ($2);}
               | program error '}' { 
                     $$ = $1;}
               | program error ';' {
                     $$ = $1;}
               | /*empty*/ { 
                     $$ = new astree(TOK_ROOT, {0,0,0}, ""); }
               ;

/* * 
 * Definitions for a struct. The hierarchy follows,
 *    + struct
 *    |  + type_id
 *    |  + field definition
 *    |  :
 *    |  + field definition
 */

structdef      : struct_field '}' ';' { 
                     $$ = $1; }
               | TOK_STRUCT TOK_IDENT '{' '}' ';' { 
                     $$ = $1->adopt($2); }
               ;

struct_field   : TOK_STRUCT TOK_IDENT '{' type TOK_IDENT ';' {
                     // STRUCT IDENT
                     $1 = $1->adopt($2);

                     // FIELD DEFINITIONS
                     $3 = $3->change_symbol(TOK_TYPE_ID);
                     $3 = $3->clear_lexinfo();
                     $3 = $3->copy_lloc($4);
                     $3 = $3->adopt($4, $5);
                     $1 = $1->adopt($3);
                     
                     $$ = $1;}
               | struct_field type TOK_IDENT ';' {
                     // ADDITIONAL FIELD DEFINITIONS
                     $4 = $4->change_symbol(TOK_TYPE_ID);
                     $4 = $4->clear_lexinfo();
                     $4 = $4->copy_lloc($2);
                     $4 = $4->adopt($2, $3);
                     $1 = $1->adopt($4);

                     $$ = $1;}
               ;

/* * 
 * Definitions for types.
 */

type        :  plaintype {
                  $$ = $1;}
            |  TOK_ARRAY TOK_LT plaintype TOK_GT{
                  $$ = $1->adopt ($3);}
            ;

plaintype   :  TOK_VOID{ 
                  $$ = $1;}
            |  TOK_INT{ 
                  $$ = $1;}
            |  TOK_STRING{ 
                  $$ = $1;}
            |  TOK_PTR TOK_LT TOK_STRUCT TOK_IDENT TOK_GT{  
                  $$ = $1->adopt ($4);}
            ;

/* * 
 * Definitions for a function
 *    + function
 *    |  + type_id
 *    |  + parameters
 *    |  + block
 *    
 */

function    : type TOK_IDENT parameter ')' block {
                  $$ = new astree(TOK_FUNCTION, $1->lloc, "");

                  // TOK_TYPE_ID
                  $4 = $4->change_symbol(TOK_TYPE_ID);
                  $4->lloc = $1->lloc;
                  $4->lexinfo = $$->lexinfo;
                  $4 = $4->adopt($1);
                  $4 = $4->adopt($2);
                  $$ = $$->adopt($4);

                  // PARAMERTERS
                  $3 = $3->change_symbol(TOK_PARAM);
                  $3 = $3->clear_lexinfo();
                  $$ = $$->adopt($3, $5);}

            | type TOK_IDENT '(' ')' block {
                  $$ = new astree(TOK_FUNCTION, $1->lloc, "");
                  
                  // TOK_TYPE_ID
                  $4 = $4->change_symbol(TOK_TYPE_ID);
                  $4->lloc = $1->lloc;
                  $4->lexinfo = $$->lexinfo;
                  $4 = $4->adopt($1);
                  $4 = $4->adopt($2);
                  $$ = $$->adopt($4);

                  // PARAMETERS
                  $3 = $3->change_symbol(TOK_PARAM);
                  $3 = $3->clear_lexinfo();
                  $$ = $$->adopt($3);
                  
                  //BLOCK
                  $$ = $$->adopt($5);}
            | type TOK_IDENT parameter ')' ';' { 
                  $$ = new astree(TOK_FUNCTION, $1->lloc, "");
                  
                  // TOK_TYPE_ID
                  $4 = $4->change_symbol(TOK_TYPE_ID);
                  $4->lloc = $1->lloc;
                  $4->lexinfo = $$->lexinfo;
                  $4 = $4->adopt($1);
                  $4 = $4->adopt($2);
                  $$ = $$->adopt($4);
                  
                  // PARAMETERS
                  $3 = $3->change_symbol(TOK_PARAM);
                  $3 = $3->clear_lexinfo();
                  $$ = $$->adopt($3);}

            | type TOK_IDENT '(' ')' ';' {
                  $$ = new astree(TOK_FUNCTION, $1->lloc, "");

                  // TOK_TYPE_ID
                  $4 = $4->change_symbol(TOK_TYPE_ID);
                  $4->lloc = $1->lloc;
                  $4->lexinfo = $$->lexinfo;
                  $4 = $4->adopt($1);
                  $4 = $4->adopt($2);
                  $$ = $$->adopt($4);

                  // PARAMETERS
                  $3 = $3->change_symbol(TOK_PARAM);
                  $3 = $3->clear_lexinfo();
                  $$ = $$->adopt($3);}
            | type TOK_IDENT '(' ')' '{' '}' {
                  $$ = new astree(TOK_FUNCTION, $1->lloc, "");

                  // TOK_TYPE_ID
                  $4 = $4->change_symbol(TOK_TYPE_ID);
                  $4->lloc = $1->lloc;
                  $4->lexinfo = $$->lexinfo;
                  $4 = $4->adopt($1);
                  $4 = $4->adopt($2);
                  $$ = $$->adopt($4);

                  // PARAMETERS
                  $3 = $3->change_symbol(TOK_PARAM);
                  $3 = $3->clear_lexinfo();
                  $$ = $$->adopt($3);}
            ;

/* * 
 * Definition for parameters
 *    + parameters
 *    |  + type_id
 *    |  :
 *    |  + type_id
 *    
 */

parameter   : '(' type TOK_IDENT { 
                  $1 = $1->change_symbol(TOK_PARAM);

                  // PARAMETERS
                  astree* type_id = new astree(TOK_TYPE_ID, 
                                            $2->lloc, 
                                            "");
                  type_id = type_id->adopt($2, $3);

                  $$ = $1->adopt(type_id);}
            | parameter ',' type TOK_IDENT {
               
                  // ADDITIONAL PARAMETERS
                  $2 = $2->change_symbol(TOK_TYPE_ID);
                  $2 = $2->copy_lloc($3);
                  $2 = $2->adopt($3, $4);

                  $$ = $1->adopt($2);}
            ;

/* * 
 * Definition for block
 *    + block
 *    |  + statement
 *    |  :
 *    |  + statement
 *    
 */

block                   : '{' '}'               {}
                        | block_statements '}'  {$$ = $1;}
                        | ';'                   {$$ = $1;}

block_statements        : '{' statement { 
                              $1 = $1->change_symbol(TOK_BLOCK); 
                              $$ = $1->adopt($2);}
                        | block_statements statement { 
                              $$ = $1->adopt($2);}
                        ;

statement   : vardecl   { $$ = $1; }
            | block     { $$ = $1; }
            | while     { $$ = $1; }
            | ifelse    { $$ = $1; }
            | return    { $$ = $1; }
            | expr ';'  { $$ = $1; }
            ;

vardecl     : type TOK_IDENT ';' { 
                  $3->adopt_sym ($1, TOK_TYPE_ID); 
                  $$ = $3->adopt ($2);}
            | type TOK_IDENT '=' expr ';' { 
                  $3->adopt_sym ($1, TOK_TYPE_ID); 
                  $$ = $3->adopt ($2, $4);}
            ;

/* * 
 * Definition for while
 *    + block
 *    |  + expr
 *    |  + statement
 *    
 */

while       : TOK_WHILE '(' expr ')' statement { 
                  $$ = $1->adopt($3, $5);}
            ;

/* * 
 * Definition for ifelse
 *    + if
 *    |  + expr
 *    |  + statement
 *    |  + statement (if `else` is present)
 *    
 */

ifelse      : TOK_IF '(' expr ')' statement { 
                  $$ = $1->adopt($3, $5); }
            | TOK_IF '(' expr ')' statement TOK_ELSE statement { 
                  $1 = $1->adopt($3, $5); 
                  $$ = $1->adopt ($7);}

/* * 
 * Definition for return
 *    + return
 *    |  + expr (optional)
 *    
 */                  

return      : TOK_RETURN ';'        { $$ = $1; }
            | TOK_RETURN expr ';'   { $$ = $1->adopt ($2); }
            ;

expr        : expr binop expr { $$ = $2->adopt ($1, $3); }
            | unop expr       { $$ = $1->adopt ($2); }
            | allocator       { $$ = $1; }
            | call            { $$ = $1; }
            | '(' expr ')'    { $$ = $2; }
            | variable        { $$ = $1; }
            | constant        { $$ = $1; }
            ;

/* * 
 * Definition for binop
 *    + binop
 *    |  + expr (left)
 *    |  + expr (right)
 *    
 */  

binop       : '='       { $$ = $1; }
            | TOK_EQ    { $$ = $1; }
            | TOK_NE    { $$ = $1; }
            | TOK_LT    { $$ = $1; }
            | TOK_LE    { $$ = $1; }
            | TOK_GT    { $$ = $1; }
            | TOK_GE    { $$ = $1; }
            | '+'       { $$ = $1; }
            | '-'       { $$ = $1; }
            | '*'       { $$ = $1; }
            | '/'       { $$ = $1; }
            | '%'       { $$ = $1; }
            | '['       {
                  // HANDLE INDEX OPERATOR
                  $1 = $1->change_symbol(TOK_INDEX);
                  $$ = $1;}
            | TOK_ARROW { $$ = $1; }
            | TOK_CALL  { $$ = $1; }
            | TOK_ALLOC { $$ = $1; }
            ;

unop        : '+'       { $1->symbol = TOK_POS; $$ = $1; }
            | '-'       { $1->symbol = TOK_NEG; $$ = $1; }
            | TOK_NOT   { $$ = $1; }
            | TOK_ALLOC { $$ = $1; }
            ;


allocator   : TOK_ALLOC TOK_LT TOK_STRING TOK_GT '(' expr ')' {
                  $$ = $1->adopt($3, $6);}
            | TOK_ALLOC TOK_LT TOK_STRUCT TOK_IDENT TOK_GT '(' ')'{
                  $$ = $1->adopt($4);}
            | TOK_ALLOC TOK_LT TOK_ARRAY TOK_LT plaintype TOK_GT TOK_GT 
              '(' expr ')' {
                  $$ = $1->adopt($3->adopt($5), $9);}
            ;

call        : TOK_IDENT '(' ')' { 
                  $2 = $2->change_symbol(TOK_CALL); $$ = $2->adopt($1);}
            | arguments ')' { 
                  $$ = $1;}
            ;

arguments   : TOK_IDENT '(' expr { 
                  $2 = $2->change_symbol(TOK_CALL); 
                  $$ = $2->adopt($1, $3);}
            | arguments ',' expr { 
                  $$ = $1->adopt($3);}

variable    : TOK_IDENT                { $$ = $1; }
            | expr '[' expr ']'        { 
               $2 = $2->change_symbol(TOK_INDEX);
               $$ = $2->adopt ($1, $3);}
            | expr TOK_ARROW TOK_IDENT { $$ = $2->adopt ($1, $3); }
            ;

constant    : TOK_INTCON      { $$ = $1; }
            | TOK_CHARCON     { $$ = $1; }
            | TOK_STRINGCON   { $$ = $1; }
            | TOK_NULLPTR     { $$ = $1; }
            ;

%%


const char *parser::get_tname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

