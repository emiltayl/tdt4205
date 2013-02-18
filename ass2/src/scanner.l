%{
#include "parser.h"
#ifdef DUMP_TOKENS
    #define RETURN(t) do {                                      \
        fprintf ( stderr, "TOKEN ( %d,\t'%s' )\n", t, yytext );    \
        return t;                                               \
    } while ( 0 )
#else
    #define RETURN(t) return t
#endif
%}

%option array
%option noyywrap
%option yylineno

%%
"//".*\n    {}
[\ \t\n]    {}
[0-9]+      { RETURN(NUMBER); }
"FUNC"      { RETURN(FUNC); }
"PRINT"     { RETURN(PRINT); }
"RETURN"    { RETURN(RETURN); }
"CONTINUE"  { RETURN(CONTINUE); }
"IF"        { RETURN(IF); }
"THEN"      { RETURN(THEN); }
"ELSE"      { RETURN(ELSE); }
"FI"        { RETURN(FI); }
"WHILE"     { RETURN(WHILE); }
"DO"        { RETURN(DO); }
"DONE"      { RETURN(DONE); }
"FOR"       { RETURN(FOR); }
"TO"        { RETURN(TO); }
"VAR"       { RETURN(VAR); }
":="        { RETURN(ASSIGN); }
"<="        { RETURN(LEQUAL); }
">="        { RETURN(GEQUAL); }
"=="        { RETURN(EQUAL); }
"!="        { RETURN(NEQUAL); }
\"[^\n]*\"  { RETURN(STRING); }
[_a-zA-Z][_0-9a-zA-Z]*  { RETURN(IDENTIFIER); }
.           { RETURN( yytext[0] ); }
%%
