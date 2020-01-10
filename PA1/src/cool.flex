/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
  if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
    YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */
int isInComments=0;
%}

%option noyywrap
NEWLINE \n
COMMENTS (\(\*(([^\*]*)|(([^\*]*)\*[^\)]([^\*]*))*)\*\))
COMMENTS_IN_LINE "--".*"\n"|"--".*
COMMENTS_NO_END \(\*(([^\*]*)|(([^\*]*)\*[^\)]([^\*]*))*)(\*?)
TYPE [A-Z][a-zA-Z_0-9]*
OBJECT [a-z][a-zA-Z_0-9]*
ANYCHAR .
digit       [0-9]
TRUE t[rR][uU][eE]
FALSE f[aA][lL][sS][eE]
STR_CONST \"([^\\\"\n\0]|(\\(.|\n)))*\"
STR_CONST_NO_END \"([^\\\"\n]|(\\(.|\n)))*\n
STR_CONST_WITH_EOF \"([^\\\"\n]|(\\(.|\n)))*
STR_CONST_WITH_NULLCHAR \"([^\\\"\n]|(\\(.|\n)))*\0([^\\\"\n]|(\\(.|\n)))*\"
INT_CONST [0-9]+
%%

 /*
  * Define regular expressions for the tokens of COOL here. Make sure, you
  * handle correctly special cases, like:
  *   - Nested comments
  *   - String constants: They use C like systax and can contain escape
  *     sequences. Escape sequence \c is accepted for all characters c. Except
  *     for \n \t \b \f, the result is c.
  *   - Keywords: They are case-insensitive except for the values true and
  *     false, which must begin with a lower-case letter.
  *   - Multiple-character operators (like <-): The scanner should produce a
  *     single token for every such operator.
  *   - Line counting: You should keep the global variable curr_lineno updated
  *     with the correct line number
  */

 /*
  * Comments
  */
{COMMENTS} {
  for(int i=0;i<strlen(yytext);i++){
    if(yytext[i]=='\n') curr_lineno++;
    if(yytext[i]=='('&&yytext[i+1]=='*') {
      isInComments++;
      i++;
    }
    if(yytext[i]=='*'&&yytext[i+1]==')') {
	      if(isInComments>0){
          isInComments--;
        }
        else printf("#%d ERROR \"Unmatched *)\"\n", curr_lineno);
        i++;
    }
  }
}
"(*" {isInComments++;}
{COMMENTS_IN_LINE} {
  if(isInComments!=0){
    for(int i=0;i<strlen(yytext);i++){
      if(yytext[i]=='('&&yytext[i+1]=='*') {
        isInComments++;
        i++;
      }
      if(yytext[i]=='*'&&yytext[i+1]==')') {
        if(isInComments>0){
          isInComments--;
        }
        else printf("#%d ERROR \"Unmatched *)\"\n", curr_lineno);
        i++;
      }
    }
  }
  for(int i=0;i<strlen(yytext);i++){
    if(yytext[i]=='\n')
    curr_lineno++;
  }
}

"*)" {
  if(isInComments>0){
    isInComments--;
  }
  else printf("#%d ERROR \"Unmatched *)\"\n", curr_lineno);
}
<<EOF>> {
  if(isInComments>0){
    printf("#%d ERROR \"EOF in comment\"\n", curr_lineno);
  }
  yyterminate();
}
 /*
  * Keywords
  */
[cC][lL][aA][sS][sS] {if(isInComments==0) printf("#%d CLASS\n", curr_lineno);}
[iI][nN][hH][eE][rR][iI][tT][sS] {if(isInComments==0) printf("#%d INHERITS\n", curr_lineno);}
[iI][fF] {if(isInComments==0) printf("#%d IF\n", curr_lineno);}
[fF][iI] {if(isInComments==0) printf("#%d FI\n", curr_lineno);}
[tT][hH][eE][nN] {if(isInComments==0) printf("#%d THEN\n", curr_lineno);}
[eE][lL][sS][eE] {if(isInComments==0) printf("#%d ELSE\n", curr_lineno);}
[wW][hH][iI][lL][eE] {if(isInComments==0) printf("#%d WHILE\n", curr_lineno);}
[lL][oO][oO][pP] {if(isInComments==0) printf("#%d LOOP\n", curr_lineno);}
[pP][oO][oO][lL] {if(isInComments==0) printf("#%d POOL\n", curr_lineno);}
[nN][eE][wW] {if(isInComments==0) printf("#%d NEW\n", curr_lineno);}
[lL][eE][tT] {if(isInComments==0) printf("#%d LET\n", curr_lineno);}
[iI][nN] {if(isInComments==0) printf("#%d IN\n", curr_lineno);}
[cC][aA][sS][eE] {if(isInComments==0) printf("#%d CASE\n", curr_lineno);}
[oO][fF] {if(isInComments==0) printf("#%d OF\n", curr_lineno);}
[eE][sS][aA][cC] {if(isInComments==0) printf("#%d ESAC\n", curr_lineno);}
[iI][sS][vV][oO][iI][dD] {if(isInComments==0) printf("#%d ISVOID\n", curr_lineno);}
[nN][oO][tT] {if(isInComments==0) printf("#%d NOT\n", curr_lineno);}

 /*
  * Data
  */
{INT_CONST} {if(isInComments==0) printf("#%d INT_CONST %s\n", curr_lineno, yytext);}
{STR_CONST} {
  int count=0;
  for(int i=0;i<strlen(yytext);i++){
    if(yytext[i]=='\n') curr_lineno++;
  }
  if(isInComments==0){
    for(int i=0;i<strlen(yytext);i++){
      count++;
      if(yytext[i]=='\\') {
        i++;
      }
    }
    if (count<MAX_STR_CONST+2)
    {
      printf("#%d STR_CONST ", curr_lineno);
      for(int i=0;i<strlen(yytext);i++){
        if(yytext[i]!='\\'){
          if(yytext[i]=='\n') printf("\\n");
          else if(yytext[i]=='\f') printf("\\f");
          else if(yytext[i]=='\t') printf("\\t");
          else if(yytext[i]=='\b') printf("\\b");
          else if(yytext[i]>=0&&yytext[i]<=31) printf("\\%.3o", yytext[i]);
          else if(yytext[i]<=-1) printf("\\%.3o", 256+yytext[i]);
          else printf("%c", yytext[i]);
        }
        if(yytext[i]=='\\'){
          if(yytext[i+1]=='b'||yytext[i+1]=='f'||yytext[i+1]=='n'||yytext[i+1]=='t'||yytext[i+1]=='\\'||yytext[i+1]=='"'){
            printf("%c%c", yytext[i], yytext[i+1]);
          }
          else if(yytext[i+1]=='\n') printf("\\n");
          else if(yytext[i+1]=='\f') printf("\\f");
          else if(yytext[i+1]=='\t') printf("\\t");
          else if(yytext[i+1]=='\b') printf("\\b");
          else if(yytext[i+1]>=0&&yytext[i+1]<=31&&yytext[i+1]!=28) printf("\\%.3o", yytext[i+1]);
          else if(yytext[i+1]<=-1&&yytext[i+1]!=28) printf("\\%.3o", 256+yytext[i+1]);
          else{
            printf("%c", yytext[i+1]);
          }
          i++;
        }
      }
      printf("\n");
    }
    else   
      printf("#%d ERROR \"String constant too long\"\n", curr_lineno);
  }
}
{STR_CONST_NO_END} {
  curr_lineno++;
  if(isInComments==0){
    printf("#%d ERROR \"Unterminated string constant\"\n", curr_lineno);
  }
}
{STR_CONST_WITH_EOF} {if(isInComments==0) printf("#%d ERROR \"EOF in string constant\"\n", curr_lineno);}
{STR_CONST_WITH_NULLCHAR} {if(isInComments==0) printf("#%d ERROR \"String contains null character.\"\n", curr_lineno);}
{TRUE} {if(isInComments==0) printf("#%d BOOL_CONST true\n", curr_lineno);}
{FALSE} {if(isInComments==0) printf("#%d BOOL_CONST false\n", curr_lineno);}

 /*
  * Id
  */
{TYPE} {if(isInComments==0) printf("#%d TYPEID %s\n", curr_lineno, yytext);}
{OBJECT} {if(isInComments==0) printf("#%d OBJECTID %s\n", curr_lineno, yytext);}

 /*
  * Trivia
  */
"<-" {if(isInComments==0) printf("#%d ASSIGN\n", curr_lineno);}
"=>" {if(isInComments==0) printf("#%d DARROW\n", curr_lineno);}
"<=" {if(isInComments==0) printf("#%d LE\n", curr_lineno);}
":"|";"|","|"{"|"}"|"("|")"|"+"|"-"|"*"|"/"|"="|"<"|"."|"@"|"~" {if(isInComments==0) printf("#%d '%s'\n", curr_lineno, yytext);}
{NEWLINE} {curr_lineno++;}
" "|"\t"|"\r"|"\f"|"\v" {}
\b {if(isInComments==0) printf("#%d ERROR \"\\b\"\n", curr_lineno);}
\\ {if(isInComments==0) printf("#%d ERROR \"\\\\\"\n", curr_lineno, yytext);}
\177 {if(isInComments==0) printf("#%d ERROR \"\\177\"\n", curr_lineno);}
{ANYCHAR} {
  if(isInComments==0){
    if(yytext[0]>=0&&yytext[0]<=31) printf("#%d ERROR \"\\%.3o\"\n", curr_lineno, yytext[0]);
    else if(yytext[0]<=-1) printf("#%d ERROR \"\\%.3o\"\n", curr_lineno, 256+yytext[0]);
    else printf("#%d ERROR \"%s\"\n", curr_lineno, yytext);
  }
}
%%
