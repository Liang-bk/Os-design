/* controlflow.c
 *
 * "if" processing is done with two state variables
 *    if_state and if_result
 */
#include	<stdio.h>
#include	<stdbool.h>
#include	"smsh.h"
#define BLOCK_SIZE 64
extern FILE *fp;
enum states   { NEUTRAL, WANT_THEN, THEN_BLOCK, ELSE_BLOCK, SKIP, WANT_DO, DO_BLOCK };
enum results  { SUCCESS, FAIL };
static int control_err = 0;
static char **body = NULL; 							// loop body
static char **condition = NULL;						// loop condition
static int line = 0;								// length of loop body
static int capacity = 0;							// capacity of loop body
static int loop_state = NEUTRAL;			  
typedef struct
{
	int if_state;
	int if_result;
	int last_stat;
} IF_CONTROL;
//stack
typedef struct 
{
	int maxSize;
	int top_p; //stack top pointer
	IF_CONTROL *elem;
} seqIfStack;
static IF_CONTROL curIF = {NEUTRAL,SUCCESS,0};
static seqIfStack stack = {0,-1,NULL};
void clearWHL()
{
	loop_state = NEUTRAL;
	if(body != NULL){
		freelist(body);
		body = NULL;
	}
	condition = NULL;
	line = capacity = 0;
}
void init_IfStack(seqIfStack *s,int Size){
	s->elem = (IF_CONTROL *)emalloc(sizeof(IF_CONTROL) * Size);
	s->maxSize = Size;
	s->top_p = -1;
}
void free_IfStack(seqIfStack *s){
	if(s->elem != NULL)
		free(s->elem);
	s->elem = NULL;
	s->maxSize = 0;
	s->top_p = -1;
}
void More_IfStack(seqIfStack *s){
	s->elem = (IF_CONTROL *)erealloc(s->elem,2 * sizeof(IF_CONTROL) * s->maxSize);
}

void IfStack_push(seqIfStack *s,IF_CONTROL x){
	if(s->top_p == s->maxSize)
		More_IfStack(s);
	s->elem[++(s->top_p)] = x;
}
bool IfStack_isEmpty(seqIfStack *s){
	return s->top_p == -1;
}

void IfStack_pop(seqIfStack *s){
	if(s->top_p == -1)return;
	s->top_p--;
}
IF_CONTROL IfStack_top(seqIfStack *s){
	return s->elem[s->top_p];
}

int	syn_err(char *);

int ok_to_execute()
/*
 * purpose: determine the shell should execute a command
 * returns: 1 for yes, 0 for no
 * details: if in THEN_BLOCK and if_result was SUCCESS then yes
 *          if in THEN_BLOCK and if_result was FAIL    then no
 *          if in WANT_THEN  then syntax error (sh is different)
 */
{
	control_err = 0;
	int	rv = 1;		/* default is positive */


	if ( curIF.if_state == WANT_THEN ){
		syn_err((char *)"then expected");
		rv = 0;
	}
	else if ( curIF.if_state == THEN_BLOCK && curIF.if_result == SUCCESS )
		rv = 1;
	else if ( curIF.if_state == THEN_BLOCK && curIF.if_result == FAIL )
		rv = 0;
	else if( curIF.if_state == ELSE_BLOCK && curIF.if_result == FAIL)
		rv = 1;
	else if( curIF.if_state == ELSE_BLOCK && curIF.if_result == SUCCESS)
		rv = 0;
	else if( curIF.if_result == SKIP)
		rv = 0;
	return rv;
}

int is_control_command(char *s)
/*
 * purpose: boolean to report if the command is a shell control command
 * returns: 0 or 1
 */
{
    return (strcmp(s,"if")==0 || strcmp(s,"then")==0 || 
		strcmp(s,"fi")==0 || strcmp(s,"else")==0 );
}
int is_loop_command(char *s)
{
	return (strcmp(s,"while") == 0 || strcmp(s,"do") == 0 || strcmp(s,"done") == 0);   
}

int do_control_command(char **args)
/*
 * purpose: Process "if", "then", "fi" - change state or detect error
 * returns: 0 if ok, -1 for syntax error
 *   notes: I would have put returns all over the place, Barry says "no"
 */
{
	control_err = 0;
	char	*cmd = (char *)args[0];
	int	rv = -1;
	
	if( strcmp(cmd,"if")==0 ){
		// encouter the first "if"
		if(curIF.if_state == NEUTRAL){
			free_IfStack(&stack);
			init_IfStack(&stack,10);
		}
		// encounter "if" when needing then, error!
		if (curIF.if_state == WANT_THEN )
			rv = syn_err((char *)"if unexpected");
		else {
			//encounter nested if
			IfStack_push(&stack,curIF);
			if(ok_to_execute()){
				curIF.last_stat = process(args+1);
				curIF.if_result = (curIF.last_stat == 0 ? SUCCESS : FAIL);
			}
			else curIF.if_result = SKIP;
			curIF.if_state = WANT_THEN;
			rv = 0;
		}
	}
	else if ( strcmp(cmd,"then")==0 ){
		if ( curIF.if_state != WANT_THEN )
			rv = syn_err((char *)"then unexpected");
		else {
			curIF.if_state = THEN_BLOCK;
			rv = 0;
		}
	}
	else if ( strcmp(cmd,"fi")==0 ){
		if ( curIF.if_state != THEN_BLOCK && curIF.if_state != ELSE_BLOCK )
			rv = syn_err((char *)"fi unexpected");
		else {
			if(!IfStack_isEmpty(&stack)){
				// current "if" still in nesting
				curIF = IfStack_top(&stack);
				IfStack_pop(&stack);
			}
			else {
				// current "if" is first "if"
				curIF.if_state = NEUTRAL;
				curIF.if_result = SUCCESS;
				curIF.last_stat = 0;
			}
			rv = 0;
		}
	}
	else if( strcmp(cmd,"else")==0 ){
		if( curIF.if_state != THEN_BLOCK )
			rv = syn_err((char *)"else unexpeccted");
		else {
			curIF.if_state = ELSE_BLOCK;
			rv = 0;
		}
	}
	else 
		fatal((char *)"internal error processing:", cmd, 2);
	return rv;
}

int do_loop_command(char **args)
/*
* read next_cmd and execute lines
*/
{
	control_err = 0;
	int rv = -1;
	if(strcmp(args[0],"while") == 0){
		loop_state = WANT_DO;
		condition = args + 1;
		if(*condition == NULL)
			return syn_err((char *)"condition can't be NULL");
		
		// start to read next_cmd
		char *cmdline = NULL;
		body = (char **)emalloc(BLOCK_SIZE * sizeof(char **));			// 64 * (char **)
		capacity = BLOCK_SIZE;											// 64

		for(int i = 0; i < capacity; i++)body[i] = NULL;				// initialize

		while((cmdline = next_cmd((char *)"",fp)) != NULL)
		{
			// expand the while block
			if(line + 1 >= capacity)
			{
				body = (char **)erealloc(body,(capacity + BLOCK_SIZE) * sizeof(char **));
				for(int i = capacity; i < capacity + BLOCK_SIZE; i++)body[i] = NULL;
				capacity += BLOCK_SIZE;
			}

			char **arglist = splitline(cmdline);
			
			// recognize keywords
			if(strcmp(arglist[0],"while") == 0)
				rv = syn_err((char *)"nested loops are not supported");
			else if(strcmp(arglist[0],"do") == 0)
			{
				if(loop_state != WANT_DO)
					rv = syn_err((char *)"unexpected do");
				else {
					rv = 0;
					loop_state = DO_BLOCK;
				}
			}
			else if(strcmp(arglist[0],"done") == 0)
			{
				if(loop_state != DO_BLOCK)
					rv = syn_err((char *)"unexpected done");
				else {
					rv = 0;
					loop_state = NEUTRAL;
				}
			}
			else {
				if(loop_state != DO_BLOCK)
					rv = syn_err((char *)"do expected");
				else{
					rv = 0;
					int l = strlen(cmdline);
					body[line] = (char *)emalloc(l + 1);
					body[line][l] = '\0';							// it's important here to add '\0' to avoid overflow
					strncpy(body[line],cmdline,l);
					line += 1;
				}
			}
			freelist(arglist);
			free(cmdline);
			if(rv == -1 || loop_state == NEUTRAL)
				break;
		}

		if(rv == -1)											// error while reading block
			return -1;
		else if(rv == 0 && loop_state != NEUTRAL)				//	end of file lack of done
			return syn_err((char *)"done expected");
		else if(ok_to_execute())								// "while" could be in an if block, need to detect
		{
			while (execute(condition) == 0)						// while the condition is true
			{
				char **cp = body;
				for(; *cp != NULL; cp++)
				{
					char **arglist = splitline(*cp);
					process(arglist);
					freelist(arglist);
					if(control_err)break;						// means if error in while block, always a syntax err
				}
				if(control_err)break;
			}
		}

		if(!control_err)										// no error when processing
			clearWHL();
		
		return 0;
	}
	else {
		rv = syn_err((char *)"unexpected token");
		return rv;
	}

}
int syn_err(char *msg)
/* purpose: handles syntax errors in control structures
 * details: resets state to NEUTRAL
 * returns: -1 in interactive mode. Should call fatal in scripts
 */
{
	
	control_err = 1;
	//free the all the variable
	clearWHL();
	curIF.if_state = NEUTRAL;
	curIF.if_result = SUCCESS;
	free_IfStack(&stack);
	fprintf(stderr,"syntax error: %s\n", msg);
	return -1;
}
