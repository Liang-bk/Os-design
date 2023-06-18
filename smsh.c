#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include"smsh.h"
#define DFL_PROMPT (char *)"> "
int hasError = 0;
FILE *fp = NULL;
int main(int argc,char *argv[]){
    char *cmdline,*prompt,**arglist;
    int result;
    if(argc == 1){
        fp = stdin;
        prompt = DFL_PROMPT;
    }
    else if (argc == 2){
        if((fp = fopen(argv[1],"r")) == NULL)
            fatal((char *)"can't open file!",(char *)"",2);
        prompt = (char *)"";
    }  
    else {
        fatal((char *)"too many arguments!",(char *)"",3);
    }
    
    //void setup();
    //setup();
    //get the input
    while((cmdline = next_cmd(prompt,fp)) != NULL){
        //split the line
        if((arglist = splitline(cmdline)) != NULL){
            //execute the command
            result = process(arglist);
            freelist(arglist);
        }
        free(cmdline);
        hasError = 0;
    }
    return 0;
}
/*
void setup(){
    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
}
*/
void fatal(char *s1,char *s2,int n)
{
    fprintf(stderr,"Error: %s,%s\n",s1,s2);
    exit(n);
}