#ifndef VARLIB_H
#define VARLIB_H

char	*VLlookup(char *);
void	VLlist();
int	VLstore( char *, char * );
char *replaceVal(char *argv);
int Analyze_Expr(char *argv[]);
int solve_CALC(char *argv[]);
#endif