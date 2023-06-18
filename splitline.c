/*splitline.c - command reading and parsing fucntions for smsh

*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"smsh.h"
//transform a line in fp to buf
char *next_cmd(char *prompt,FILE *fp)
{
    char *buf;          //content of a line
    int bufspace = 0;   //capacity of buf
    int pos = 0;        //current point of buf
    int c;              
    printf("%s",prompt);
    while((c = getc(fp)) != EOF){
        /*need space?*/
        if(pos + 1 >= bufspace){
            if(bufspace == 0)
                buf = (char *)emalloc(BUFSIZ);
            else buf = (char *)erealloc(buf,bufspace + BUFSIZ);
            bufspace += BUFSIZ;
        }
        if(c == '\n')
            break;
        
        buf[pos++] = c;  
    }
    if(c == EOF && pos == 0)
        return NULL;
    buf[pos] = '\0';
    return buf; 
}

#define is_delim(x) ((x)==' '||(x)=='\t')

//split the line to get arguments;
char **splitline(char *line)
{
    char *newstr(char *start,int len);
    char **args; 
    int spots = 0;          // Maxinum of arguments  
    int bufspace = 0;       // capacity of args
    int argnum = 0;         // argc
    char *cp = line;        // point of line
    char *start;            // beginning of an argument
    int len;                // length of an argument

    if(line == NULL)
        return NULL;
    //if the compiler is g++, then need to add (char **) before emalloc 
    args = (char **)emalloc(BUFSIZ);
    bufspace = BUFSIZ;
    spots = BUFSIZ/sizeof(char *);

    while(*cp != '\0')
    {
        while(is_delim(*cp))    //delete the blank and tab
            cp++;
        if(*cp == '\0')
            break;
        
        if(argnum + 1 >= spots){
            args = (char **)erealloc(args,bufspace + BUFSIZ);
            bufspace += BUFSIZ;
            spots += (BUFSIZ/sizeof(char *));
        }

        start = cp;
        len = 1;
        while(*(++cp) != '\0' && (!is_delim(*cp)))
            len++;
        args[argnum++] = newstr(start,len);     //get the argument
    }
    args[argnum] = NULL;
    return args;
}

char *newstr(char *s,int l)
{
    char *rv = (char*)emalloc(l + 1);
    rv[l] = '\0';
    strncpy(rv,s,l);
    return rv;
}

void freelist(char **list)
{
    char **cp = list;
    while(*cp){
        free(*cp);
        cp++;
    }
    free(list);
}

void *emalloc(size_t n)
{
    void *rv;
    if((rv = malloc(n)) == NULL)
        fatal((char *)"out of memory",(char *)"",1);
    return rv;
}
void *erealloc(void *p,size_t n)
{
    void *rv;
    if((rv = realloc(p,n)) == NULL)
        fatal((char *)"realloc() failed",(char *)"",1);
    return rv;
}