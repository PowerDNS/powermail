#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union 
{
        int number;
        char *string;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	AMOUNTTOK	257
# define	QUOTEDWORD	258
# define	ADDRESSTOK	259
# define	FORWARDTOK	260
# define	PASSWORDTOK	261
# define	QUOTATOK	262
# define	OBRACE	263
# define	EBRACE	264
# define	SEMICOLON	265


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
