%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <utility>
#include <errno.h>
#include <string>
#include "misc.hh"
#include "logger.hh"
#include "ahuexception.hh"
using namespace std;
#define YYDEBUG 1
extern int yydebug;
#include "textparser.hh"

extern "C" 
{
	int yyparse(void);
	int yylex(void);

	int yywrap()
	{
		return 1;
	}

}

extern int yydebug;

extern int linenumber;
static void yyerror(const char *str)
{

	throw AhuException("Error in textbase on line "+itoa(linenumber)+": "+str);
}

extern FILE *yyin;

TextBaseEntry d_addr;

%}


%union 
{
        int number;
        char *string;
}

%token <number> AMOUNTTOK
%token <string> QUOTEDWORD
%token ADDRESSTOK FORWARDTOK PASSWORDTOK QUOTATOK OBRACE EBRACE SEMICOLON 

%%

root_commands:
	|	 
	root_commands root_command SEMICOLON
  	;

root_command: address
	;

address: ADDRESSTOK QUOTEDWORD OBRACE address_statements EBRACE 
	{
		d_addr.address=$2;
		free($2);
		textCallback(d_addr);

		d_addr.address=d_addr.password=d_addr.forward="";
		d_addr.quotaKB=0;
	}	
	;

address_statements:
        |
        address_statements address_statement SEMICOLON
	;
	
address_statement: 
        password_statement | quota_statement | forward_statement
        ;

password_statement:  PASSWORDTOK QUOTEDWORD
	{
		d_addr.password=$2; 
		free($2);
	} 
        ; 

forward_statement:  FORWARDTOK QUOTEDWORD
	{
        	d_addr.forward=$2;
		free($2);
	}
        ; 

quota_statement: QUOTATOK AMOUNTTOK 
	{
	        d_addr.quotaKB=$2/1000;
	}
        ;
