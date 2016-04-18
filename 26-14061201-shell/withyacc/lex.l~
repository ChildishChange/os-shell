%{
	#include <stdio.h>
	#include <stdlib.h>
	void yyerror();
	#include "global.h"
	#include "bison.tab.h"

#define YY_INPUT(buf,result,max_size) \
    if ( YY_CURRENT_BUFFER_LVALUE->yy_is_interactive ) \
        if (yyin==stdin) \
        { \
        int c = '*'; \
        size_t n; \
        for ( n = 0; n < max_size && \
                 (c = getc( yyin )) == c && c != '\n'; ++n ) \
            if (c>0) buf[n] = (char) c; else buf[n]=' '; \
        if ( c == '\n' ) \
            buf[n++] = (char) c; \
        if ( c == EOF && ferror( yyin ) ) \
            YY_FATAL_ERROR( "input in flex scanner failed" ); \
        result = n; \
        } \
        else { \
        int c = '*'; \
        size_t n; \
        for ( n = 0; n < max_size && \
                 (c = getc( yyin )) != EOF && c != '\n'; ++n ) \
            buf[n] = (char) c; \
        if ( c == '\n' ) \
            buf[n++] = (char) c; \
        if ( c == EOF && ferror( yyin ) ) \
            YY_FATAL_ERROR( "input in flex scanner failed" ); \
        result = n; \
        } \
    else \
        { \
        errno=0; \
        while ( (result = fread(buf, 1, max_size, yyin))==0 && ferror(yyin)) \
            { \
            if( errno != EINTR) \
                { \
                YY_FATAL_ERROR( "input in flex scanner failed" ); \
                break; \
                } \
            errno=0; \
            clearerr(yyin); \
            } \
        }\
    buf[result]='\0'; \
    inputBuff = strdup(buf); \
    if (inputBuff[strlen(inputBuff)-1]=='\n') inputBuff[strlen(inputBuff)-1] = '\0'; \
\

%}

%%

[^\n\t |<>&]+		{
						yylval = strdup(yytext);
						return STRING;
				};

[\n\|<>&]				return *yytext;
[\t ]				;
.                   yyerror();	

%%

int yywrap(void) {
	return 1;
}
