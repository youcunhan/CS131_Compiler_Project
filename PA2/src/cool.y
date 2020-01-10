/*
 *  cool.y
 *              Parser definition for the COOL language.
 *
 */
%{
#include <iostream>
#include "cool-tree.h"
#include "stringtab.h"
#include "utilities.h"

/* Add your own C declarations here */
char* class_name;
Expression temp_expr[1000];
int k = 0;
bool inwhile=false; 
bool inif=false;
/************************************************************************/
/*                DONT CHANGE ANYTHING IN THIS SECTION                  */

extern int yylex();           /* the entry point to the lexer  */
extern int curr_lineno;
extern char *curr_filename;
Program ast_root;            /* the result of the parse  */
Classes parse_results;       /* for use in semantic analysis */
int omerrs = 0;              /* number of errors in lexing and parsing */

/*
   The parser will always call the yyerror function when it encounters a parse
   error. The given yyerror implementation (see below) justs prints out the
   location in the file where the error was found. You should not change the
   error message of yyerror, since it will be used for grading puproses.
*/
void yyerror(const char *s);

/*
   The VERBOSE_ERRORS flag can be used in order to provide more detailed error
   messages. You can use the flag like this:

     if (VERBOSE_ERRORS)
       fprintf(stderr, "semicolon missing from end of declaration of class\n");

   By default the flag is set to 0. If you want to set it to 1 and see your
   verbose error messages, invoke your parser with the -v flag.

   You should try to provide accurate and detailed error messages. A small part
   of your grade will be for good quality error messages.
*/
extern int VERBOSE_ERRORS;

%}

/* A union of all the types that can be the result of parsing actions. */
%union {
  Boolean boolean;
  Symbol symbol;
  Program program;
  Class_ class_;
  Classes classes;
  Feature feature;
  Features features;
  Formal formal;
  Formals formals;
  Case case_;
  Cases cases;
  Expression expression;
  Expressions expressions;
  char *error_msg;
}

/* 
   Declare the terminals; a few have types for associated lexemes.
   The token ERROR is never used in the parser; thus, it is a parse
   error when the lexer returns it.

   The integer following token declaration is the numeric constant used
   to represent that token internally.  Typically, Bison generates these
   on its own, but we give explicit numbers to prevent version parity
   problems (bison 1.25 and earlier start at 258, later versions -- at
   257)
*/
%token CLASS 258 ELSE 259 FI 260 IF 261 IN 262 
%token INHERITS 263 LET 264 LOOP 265 POOL 266 THEN 267 WHILE 268
%token CASE 269 ESAC 270 OF 271 DARROW 272 NEW 273 ISVOID 274
%token <symbol>  STR_CONST 275 INT_CONST 276 
%token <boolean> BOOL_CONST 277
%token <symbol>  TYPEID 278 OBJECTID 279 
%token ASSIGN 280 NOT 281 LE 282 ERROR 283

/*  DON'T CHANGE ANYTHING ABOVE THIS LINE, OR YOUR PARSER WONT WORK       */
/**************************************************************************/
 
   /* Complete the nonterminal list below, giving a type for the semantic
      value of each non terminal. (See section 3.6 in the bison 
      documentation for details). */

/* Declare types for the grammar's non-terminals. */
%type <program> program
%type <classes> class_list
%type <class_> class
%type <features> feature_list
%type <feature> feature
%type <expressions> exprs_with_semicolon
%type <expressions> exprs_with_comma
%type <expression> expr_with_semicolon
%type <expression> expr
%type <expression> let_statement
%type <expression> priority
%type <formals> formals
%type <formal> formal
%type <case_> bran
%type <cases> brans

/* Precedence declarations go here. */
%left priority /*give priority to some rules to solve shift-reduce conflict*/
%left ','
%right ASSIGN
%left NOT
%left LE '<' '='
%left '+' '-'
%left '*' '/'
%left ISVOID
%left '~'
%left '@'
%left '.'
%token ':'

%%
/* 
   Save the root of the abstract syntax tree in a global variable.
*/
program : class_list { ast_root = program($1); }
        ;

class_list
        : class            /* single class */
                { $$ = single_Classes($1); }
        | class_list class /* several classes */
                { $$ = append_Classes($1,single_Classes($2)); }
        ;

/* If no parent is specified, the class inherits from the Object class. */
class  : CLASS TYPEID '{' feature_list '}' ';'
                { $$ = class_($2,idtable.add_string("Object"),$4,
                              stringtable.add_string(curr_filename)); }
        | CLASS TYPEID INHERITS TYPEID '{' feature_list '}' ';'
                { $$ = class_($2,$4,$6,stringtable.add_string(curr_filename)); }
         /* empty feature*/
        | CLASS TYPEID '{' '}' ';'
                { $$ = class_($2,idtable.add_string("Object"),nil_Features(),
                              stringtable.add_string(curr_filename)); }
        | CLASS TYPEID INHERITS TYPEID '{' '}' ';'
                { $$ = class_($2,$4,nil_Features(),stringtable.add_string(curr_filename)); }
        
        /*Error handling*/
        | CLASS TYPEID '{' feature_list '}' {class_name=(char*)$2->get_string();}
                {  yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "semicolon missing from end of declaration of class %s\n",class_name); }
        | CLASS TYPEID INHERITS TYPEID '{' feature_list '}' {class_name=(char*)$2->get_string();}
                {  yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "semicolon missing from end of declaration of class %s\n",class_name); }
        | CLASS TYPEID '{' '}' {class_name=(char*)$2->get_string();}
                {  yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "semicolon missing from end of declaration of class %s\n",class_name); }
        | CLASS TYPEID INHERITS TYPEID '{' '}' {class_name=(char*)$2->get_string();}
                {  yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "semicolon missing from end of declaration of class %s\n",class_name); }
        ;

feature_list:
        feature   {$$ = single_Features($1);}
        | feature_list feature {$$ = append_Features($1,single_Features($2));}


feature:
        OBJECTID '(' formals ')' ':' TYPEID '{' expr '}' ';'
                { $$ = method($1,$3,$6,$8); }
        | OBJECTID '(' ')' ':' TYPEID '{' expr '}' ';' 
                { $$ = method($1,nil_Formals(),$5,$7); }
        | OBJECTID ':' TYPEID ';' /* empty expr*/
                { $$ = attr($1,$3,no_expr()); }
        | OBJECTID ':' TYPEID ASSIGN expr ';'
                { $$ = attr($1,$3,$5); }
        
        /*Error handling*/
        | OBJECTID '(' formals ')' ':' TYPEID '{' expr '}'
                { yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "Semicolon missing from end of declaration of method set_var of class %s.\n",class_name); }
        | OBJECTID '(' ')' ':' TYPEID '{' expr '}'
                { yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "Semicolon missing from end of declaration of method set_var of class %s.\n",class_name); }
        | OBJECTID ':' TYPEID
                { yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "Syntax error in initialization of attribute var of class %s.\n",class_name); }
        | OBJECTID ':' TYPEID ASSIGN expr
                { yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "Syntax error in initialization of attribute var of class %s.\n",class_name); }
exprs_with_comma:
        expr
                {$$ = single_Expressions($1);}
        | exprs_with_comma ',' expr
                {$$ = append_Expressions($1,single_Expressions($3));}
        ;
exprs_with_semicolon:
        expr_with_semicolon
                {$$ = single_Expressions($1);}
        | exprs_with_semicolon expr_with_semicolon 
                {$$ = append_Expressions($1,single_Expressions($2));}
        ;

expr_with_semicolon:
        expr ';'
                { $$ = $1; }

        /*Error handling*/
        | expr '}'
                { yyerror("syntax error"); if (VERBOSE_ERRORS) fprintf(stderr, "Maybe you forgot a semicolon (;).\n"); }

expr:
        OBJECTID ASSIGN expr    { $$ = assign($1, $3); }
        | expr '.' OBJECTID '(' exprs_with_comma ')'
                                { $$ = dispatch($1, $3, $5); }
        | expr '@' TYPEID '.' OBJECTID '(' exprs_with_comma ')'
                                { $$ = static_dispatch($1,$3, $5, $7); }
        | OBJECTID '(' exprs_with_comma ')'
                                { $$ = dispatch(object(idtable.add_string("self")), $1, $3);}
        | expr '.' OBJECTID '(' ')'
                                { $$ = dispatch($1, $3, nil_Expressions()); }
        | expr '@' TYPEID '.' OBJECTID '(' ')'
                                { $$ = static_dispatch($1,$3, $5, nil_Expressions()); }
        | OBJECTID '(' ')'
                                { $$ = dispatch(object(idtable.add_string("self")), $1, nil_Expressions());}
        | IF {inif=true;} expr THEN expr ELSE expr FI {inif=false;}
                                { $$ = cond($3, $5, $7); }
        | WHILE {inwhile=true;} expr LOOP expr POOL {inwhile=false;}
                                { $$ = loop($3, $5); }
        | '{' exprs_with_semicolon '}'
                                { $$ = block($2);}
        /*let statement*/
        | LET OBJECTID ':' TYPEID {temp_expr[k]=no_expr(); k++;} IN expr %prec priority /*give priority to this rule to solve shift-reduce conflict*/
                { k--; $$ = let($2, $4, temp_expr[k], $7); }
        | LET OBJECTID ':' TYPEID ASSIGN expr IN expr %prec priority /*give priority to this rule to solve shift-reduce conflict*/
                { $$ = let($2, $4, $6, $8); }
        | LET OBJECTID ':' TYPEID {temp_expr[k]=no_expr(); k++;}',' let_statement
                { k--; $$ = let($2, $4, temp_expr[k], $7); }
        | LET OBJECTID ':' TYPEID ASSIGN expr ',' let_statement
                { $$ = let($2, $4, $6, $8); }

        | CASE expr OF brans ESAC
                                { $$ = typcase($2, $4); }
        | NEW TYPEID            { $$ = new_($2); }
        | ISVOID expr           { $$ = isvoid($2); }
        | expr '+' expr         { $$ = plus($1, $3); }
        | expr '-' expr         { $$ = sub($1, $3); }
        | expr '*' expr         { $$ = mul($1, $3); }
        | expr '/' expr         { $$ = divide($1, $3); }
        | '~' expr              { $$ = neg($2); }
        | expr '<' expr         { $$ = lt($1, $3); }
        | expr LE expr          { $$ = leq($1, $3); }
        | expr '=' expr         { $$ = eq($1, $3); }
        | NOT expr              { $$ = comp($2); }
        | '(' expr ')'          { $$ = $2; }
        | OBJECTID              { $$ = object($1); }
        | INT_CONST             { $$ = int_const($1); }
        | STR_CONST             { $$ = string_const($1); }
        | BOOL_CONST            { $$ = bool_const($1); }
        ;
formals:
        formal                  { $$ = single_Formals($1);}
        | formals ',' formal    { $$ = append_Formals($1,single_Formals($3)); }
        ;
formal:
        OBJECTID ':' TYPEID     { $$ = formal($1, $3); }
        ;
brans:
        brans bran ';'          { $$ = append_Cases($1,single_Cases($2)); }
        | bran ';'              { $$ = single_Cases($1);}
        ;
bran:
        OBJECTID ':' TYPEID DARROW expr
                                { $$ = branch($1, $3, $5); }
        ;
let_statement:
        OBJECTID ':' TYPEID {temp_expr[k]=no_expr(); k++;} let_statement
                { k--; $$ = let($1, $3, temp_expr[k], $5); }
        | OBJECTID ':' TYPEID {temp_expr[k]=no_expr(); k++;} IN expr %prec priority /*give priority to this rule to solve shift-reduce conflict*/
                { k--; $$ = let($1, $3, temp_expr[k], $6); }
        | OBJECTID ':' TYPEID ASSIGN expr let_statement
                { $$ = let($1, $3, $5, $6); }
        | OBJECTID ':' TYPEID ASSIGN expr IN expr %prec priority /*give priority to this rule to solve shift-reduce conflict*/
                { $$ = let($1, $3, $5, $7); }
        ;
/* end of grammar */
%%

/* This function is called automatically when Bison detects a parse error. */
void yyerror(const char *s)
{
  cerr << "\"" << curr_filename << "\", line " << curr_lineno << ": " \
    << s << " at or near ";
  print_cool_token(yychar);
  cerr << endl;
  if(inwhile) cerr<<"Syntax error in the while statement."<<endl;
  if(inif) cerr<<"Syntax error in the if statement."<<endl;
  omerrs++;

  if(omerrs>20) {
      if (VERBOSE_ERRORS)
         fprintf(stderr, "More than 20 errors\n");
      exit(1);
  }
}

