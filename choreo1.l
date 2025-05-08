%option noyywrap
%{
#include "choreo1.tab.h"
#include <cstdlib>
%}

%%
"("             { return tok_lparen; }
")"             { return tok_rparen; }
","             { return tok_comma; }
":"                    { return tok_colon; }
"REPEAT"                { return tok_REPEAT; }
"TIMES"                 { return tok_TIMES; }
"ENDREPEAT"             { return tok_ENDREPEAT; }
"SPIN"                  { return tok_SPIN; }
"THEN"                  { return tok_THEN; }
"MOVE TO"               { return tok_moveto; }
"ENSEMBLE"             { return tok_ENSEMBLE; }
"["                   { return tok_lbracket; }
"]"           { return tok_rbracket; }
"<"                     { return tok_less; }
">"                     { return tok_greater; }
"+"                     {  return '+'; }
"-"                     {  return '-'; }
"/"                     {return '/';}
"*"                     {  return '*'; }
";"                     { return ';'; }
"ECCO_D"                { yylval.identifier = strdup(yytext); return tok_ecco_d; }
"ECCO"                  { return tok_ecco; }
"ENTER"                 { return tok_ENTER; }
"EXIT"                  { return tok_EXIT; }
[ \t]+                  { /* skip */ }
"="                     { return '='; }

[0-9]+(\.[0-9]+)?      { yylval.double_literal = atof(yytext); return tok_double_literal; }

\"[^"]*\"              { yylval.string_literal = strdup(yytext+1); yytext[yyleng-1]=0; return tok_string_literal; }
[a-zA-Z_][a-zA-Z0-9_]*:  {
    yytext[yyleng-1] = '\0';             // drop the trailing colon
    yylval.identifier = strdup(yytext);
    return tok_label;
}

[a-zA-Z_][a-zA-Z0-9_]*  { yylval.identifier = strdup(yytext); return tok_identifier; }

\n                      { ++yylineno; /* just count the line, then skip */}
.                       { fprintf(stderr,"Unexpected `%s` on line %d\n", yytext, yylineno); exit(1); }
%%
