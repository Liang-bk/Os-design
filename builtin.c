/*
* contains the switch and the functions for builtin commands
*/
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include"smsh.h"
#include"varlib.h"
extern int hasError;
int assign(char *,char **);
int okname(char *);

char* sh_itoa(int num,char* str,int radix)
{
    char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";	// alphabet
    unsigned unum;											// store the absolute value
    
	int i = 0,j,k;
 
    if(radix == 10 && num < 0)			
    {
        unum = (unsigned)(-num);			// unum = abs(num)
        str[i++] = '-';					// negtive symbol
    }
    else unum=(unsigned)num;
 
    // transform the num to character
    do
    {
        str[i++] = index[unum % (unsigned)radix];
        unum /= radix;
 
    }while(unum);
 
    str[i]='\0';
 
    // reverse the str to get the real value
    if(str[0] == '-') k = 1;				// start at k index
    else k = 0;
 
    char temp;
    for(j = k;j <= (i - 1) / 2;j++)
    {
        temp = str[j];
        str[j] = str[i - 1 + k - j];
        str[i - 1 + k - j] = temp;
    }

    return str;
 
}

int builtin_command(char **args, int *resultp)
/*
 * purpose: run a builtin command 
 * returns: 1 if args[0] is builtin, 0 if not
 * details: test args[0] against all known builtins.  Call functions
 */
{
	int rv = 0;

	if ( strcmp(args[0],"set") == 0 ){	     /* 'set' command? */
		VLlist();
		*resultp = 0;
		rv = 1;
	}
	else if ( strchr(args[0], '=') != NULL ){   /* assignment cmd */
		*resultp = assign(args[0],args);
		if ( *resultp != -1 )		    /* x-y=123 not ok */
			rv = 1;
	}
	// else if ( strcmp(args[0], "export") == 0 ){ /*environment solve, not necessary in here*/
	// 	if ( args[1] != NULL && okname(args[1]) )
	// 		*resultp = VLexport(args[1]);
	// 	else
	// 		*resultp = 1;
	// 	rv = 1;
	// }
	return rv;
}

int assign(char *str,char **args)
/*
 * purpose: execute name=val AND ensure that name is legal
 * returns: -1 for illegal lval, or result of VLstore 
 * warning: modifies the string, but retores it to normal
 */
{
	char	*cp;
	int	rv ;
	cp = strchr(str,'=');       //find '=' in str
	*cp = '\0';
	if(okname(str)){
		*cp = '=';
		int len = strlen(str);
		// if assign like a=[ $b + 3 / $c ], calculate the expression and return the value
		if(len >= 2 && str[len - 1] == '[' && str[len - 2] == '='){
			//expression
			int result = Analyze_Expr(args + 1);
			if(!hasError){
				char *num = (char *)calloc(35,sizeof(char *));
				sh_itoa(result,num,10);
				*cp = '\0';
				rv = VLstore(str,num);
				*cp = '=';
				return rv;
			}
		}
		*cp = '\0';
		rv = VLstore(str,cp + 1);
		*cp = '=';
	}
	else {
		*cp = '=';
		rv = -1;
	}
	return rv;
}
int okname(char *str)
/*
 * purpose: determines if a string is a legal variable name
 * returns: 0 for no, 1 for yes
 */
{
	char	*cp;

	for(cp = str; *cp; cp++ ){
        if(cp == str){
            if(!(isalnum(*cp) || *cp == '_'))
                return 0;
        }
        else{
            if(!(isalnum(*cp) || *cp == '_' || isdigit(*cp)))
                return 0;
        }
	}
	return ( cp != str );	/* no empty strings, either */
}
