%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "textparser.h"

int linenumber;

%}

%x comment 
%x quoted
%%


"/*"         BEGIN(comment);
<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>\n             ++linenumber;
<comment>"*"+"/"        BEGIN(INITIAL);

forward			return FORWARDTOK; 

address			return ADDRESSTOK;
password		return PASSWORDTOK;
quota                   return QUOTATOK;
[0-9]*k                 yylval.number=1000*atoi(yytext);return AMOUNTTOK;
[0-9]*m                 yylval.number=1000000*atoi(yytext);return AMOUNTTOK;
[0-9]*g                 yylval.number=1000000000*atoi(yytext);return AMOUNTTOK;
[0-9]*                  yylval.number=atoi(yytext);return AMOUNTTOK;

\"                      BEGIN(quoted);
<quoted>[^\"]*          yylval.string=strdup(yytext); return QUOTEDWORD;
<quoted>\"              BEGIN(INITIAL);

\{                      return OBRACE;
\}                      return EBRACE;
;                       return SEMICOLON;
\n                      linenumber++;
[ \t]*                  ;
\/\/.*$			;
\#.*$			;
.                       BEGIN(INITIAL);yyterminate();errorwrapper(yytext,linenumber);

%%
