/* varlib.c
 *
 * a simple storage system to store name=value pairs
 * with facility to mark items as part of the environment
 *
 * interface:
 *     VLstore( name, value )    returns 1 for 0k, 0 for no
 *     VLlookup( name )          returns string or NULL if not there
 *     VLlist()			 prints out current table
 *
 * the environment variables under the Win system are
 * not the same as under Linux, and we are not covered
 * here for the time being
 * which means the global in the var is unnecessary
 * 
 * add expression processing
 * 
 */
#include<stdio.h>
#include<stdlib.h>
#include"varlib.h"
#include<string.h>

#define MAXVARS 200 /* a linked list wold be nicer */
extern int hasError;
static const char CMP[6][10] = {
    "-eq",
    "-ne",
    "-gt",
    "-lt",
    "-ge",
    "-le"
};
static const char OPERATOR[5][3] = {
    "+","-","*","/","^"
};
struct var{
    char *str;  /* name=val string */
    int global; /* a boolean */
};
static struct var tab[MAXVARS];

static char *new_string(char *,char *);
static struct var *find_item(char *,int);
int syn_expr_err(char *msg);
int VLstore(char *name,char *val)
/*
 * traverse list, if found, replace it, else add at end
 * since there is no delete, a blank one is a free one
 * return 1 if trouble, 0 if ok (like a command)
 */
{
    struct var *itemp;
    char *s;
    int rv = 1;

    /* find spot to put the var                 and make new string */
    if((itemp = find_item(name,1)) != NULL && (s = new_string(name,val)) != NULL)
    {
        if(itemp->str)          // has a val?
            free(itemp->str);   // remove it
        itemp->str = s;
        rv = 0;                 //assign success
    }
    return rv;
}

char * new_string(char *name,char *val)
/*
* returns new string of form name=value or NULL on error
*/
{
    char *retval;
    retval = (char *)malloc(strlen(name) + strlen(val) + 2);
    if(retval != NULL)
        sprintf(retval, "%s=%s",name,val);
    return retval;
}

char * VLlookup(char *name)
/*
* returns value of var or empty string if not there
*/
{
    struct var *itemp;
    
    if((itemp = find_item(name,0)) != NULL)
        return itemp->str + 1 + strlen(name);
    return (char *)"";
}

static struct var * find_item(char *name,int first_blank)
/*
 * searches table for an item
 * returns ptr to struct or NULL if not found
 * OR if (first_blank) then ptr to first blank one
 */
{
    int i;
    int len = strlen(name);
    char *s;
    for(i = 0; i < MAXVARS && tab[i].str != NULL; i++)
    {
        s = tab[i].str;
        if(strncmp(s,name,len) == 0 && s[len] == '='){
            return &tab[i];
        }
    }
    if(i < MAXVARS && first_blank)
        return &tab[i];
    return NULL;
}

void VLlist()
/*
 * performs the shell's  `set'  command
 * Lists the contents of the variable table, marking each
 * exported variable with the symbol  '*' 
 */
{
    int i;
    for(i = 0; i < MAXVARS && tab[i].str != NULL; i++)
    {
        if(tab[i].global)
            printf("  * %s\n",tab[i].str);
        else 
            printf("    %s\n",tab[i].str);
    }
}
int Judge_Cmp(char *str){
    for(int i = 0; i < 6; i++){
        if(strcmp(str,CMP[i]) == 0)
            return 1;
    }
    return 0;
}
int Judge_Op(char *str){
    for(int i = 0; i < 5; i++){
        if(strcmp(str,OPERATOR[i]) == 0)
            return 1;
    }
    return 0;
}
int solve_CMP(char *args[])
/*
* accept format like [ str1 op str 2 ]
* return -1 if errors, 0 if expression true, 1 if expression false
*/
{
    char **cp = args;
    int argnum = 0;
    for(; *cp != NULL; cp++){
        argnum++;
    }

    if(argnum != 4 || strcmp(args[3],"]") != 0){
        syn_expr_err((char *)"format error,please refer to the following: [ str1 op str2 ]");
        return -1;
    }

    char *str1 = replaceVal(args[0]);
    char *str2 = replaceVal(args[2]);
    char *op = args[1];
    if(str1 == NULL || str2 == NULL){
        syn_expr_err((char *)"can't compare, please check the expression!");
        return -1;
    }
    int res = strcmp(str1,str2);
    if(!strcmp(op,"-eq")){
        return res != 0;
    }
    else if(!strcmp(op,"-ne")){
        return res == 0;
    }
    else if(!strcmp(op,"-gt")){
        return res <= 0;
    }
    else if(!strcmp(op,"-lt")){
        return res >= 0;
    }
    else if(!strcmp(op,"-ge")){
        return res < 0;
    }
    else if(!strcmp(op,"-le")){
        return res > 0;
    }
    else {
        syn_expr_err((char *)"format error,please refer to the following: [ str1 op str2 ]");
        return -1;
    }
}

int Judge_Expr(char *argv[],int *c1, int *c2)
/*
* determine whether an expression is valid
* 1 is ok, 0 is wrong
*/
{
    int lack_right_parten = 1;      // lack ']'
    int unexpect = 0;

    char **cp = argv;
    for(;*cp != NULL; cp++){
        if(strcmp(*cp,"[") == 0){
            return 0;
        }
        else if(Judge_Cmp(*cp)){
            *c1 = 1;
        }
        else if(Judge_Op(*cp)){
            *c2 = 1;
        }
        else if(strcmp(*cp,"]") == 0){
            if(*(cp + 1) != NULL)       // ']' is not in the right place 
                return 0;
            lack_right_parten--;
        }
    }
    if(lack_right_parten != 0)return 0; // multiple ']' were found;
    return 1;
}
int Analyze_Expr(char *argv[])
/*
* analyze the expr like [ $a -lt $b ] or [ $a + $b / $c]
* nesting is not supported like [ [ $a -lt $b ] + [ $a + $b ] ]
*/
{
    int compare = 0;    // it's a compare expr when compare == 1
    int calc = 0;       // it's a calc expr

    int legal = Judge_Expr(argv, &compare, &calc);
    int rv = 0;
    if(legal == 0){
        hasError = 1;
        syn_expr_err((char *)"found unexpected '[' or ']' in expression");
        return -1;
    }
    if(compare == calc){
        hasError = 1;
        syn_expr_err((char *)"found two different types of expressions");
        return -1;
    }
    if(compare){
        rv = solve_CMP(argv);
        if(rv == -1)hasError = 1;
    }
    else if(calc){
        rv = solve_CALC(argv);
    }
    return rv;
}

int syn_expr_err(char *msg){
    fprintf(stderr,"syntax error: %s\n", msg);
	return -1;
}