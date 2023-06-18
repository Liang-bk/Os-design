#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include"varlib.h"
#include"smsh.h"

typedef enum{
    OPAREN,ADD,SUB,MULTI,DIV,EXP,CPAREN,VALUE,EOL
} token;

typedef struct
{
    int *elem;
    int top_p;
    int maxSize;
} seqIntStack;

typedef struct 
{
    token *elem;
    int top_p;
    int maxSize;
}seqTokStack;

static seqTokStack opStack;
static seqIntStack dataStack;
static char *expr = NULL;       // splice args as expr
extern int hasError;

void clear();

void init_IntStack(seqIntStack *s, int Size)
{
    s->elem = (int *)emalloc(Size * sizeof(int));
    s->maxSize = Size;
    s->top_p = -1;
}
void free_IntStack(seqIntStack *s)
{
    if(s->elem != NULL)
        free(s->elem);
    s->maxSize = 0;
    s->top_p = -1;
}
void More_IntStack(seqIntStack *s)
{
    s->elem = (int *)erealloc(s->elem, 2 * s->maxSize * sizeof(int));
}
int IntStack_isEmpty(seqIntStack *s) 
{
    return s->top_p == -1;
}

void IntStack_push(seqIntStack *s,const int x)
{
    if(s->top_p == s->maxSize)
        More_IntStack(s);
    s->elem[++(s->top_p)] = x;
}

void IntStack_pop(seqIntStack *s)
{
    if(s->top_p == -1)return;
    (s->top_p)--;
}

int IntStack_top(seqIntStack *s){
    return s->elem[s->top_p];
}

void init_TokStack(seqTokStack *s, int Size)
{
    s->elem = (token *)emalloc(Size * sizeof(token));
    s->maxSize = Size;
    s->top_p = -1;
}
void free_TokStack(seqTokStack *s)
{
    if(s->elem != NULL)
        free(s->elem);
    s->maxSize = 0;
    s->top_p = -1;
}
void More_TokStack(seqTokStack *s)
{
    s->elem = (token *)erealloc(s->elem,2 * s->maxSize * sizeof(token));
}
int TokStack_isEmpty(seqTokStack *s) 
{
    return s->top_p == -1;
}

void TokStack_push(seqTokStack *s,const token x)
{
    if(s->top_p == s->maxSize)
        More_TokStack(s);
    s->elem[++(s->top_p)] = x;
}

void TokStack_pop(seqTokStack *s)
{
    if(s->top_p == -1)return;
    (s->top_p)--;
}

token TokStack_top(seqTokStack *s){
    return s->elem[s->top_p];
}
void jointstr(char *args[])
/*
* joint the args to expr
* still need to replace the variable
*/
{
    expr = (char *)emalloc(BUFSIZ);
    expr[0] = '\0';
    int elen = 0;           // length of expr
    int esize = BUFSIZ;     // capacity of expr
    for(char **cp = args; *cp != NULL; cp ++){
        char *arg = replaceVal(*cp);
        if(strcmp(arg,"]") == 0)break;
        int len = strlen(arg);      // length of every args
        if(len > esize - elen + 1){
            expr = (char *)erealloc(expr,len + elen + 1);
            esize = len + elen + 1;
        }
        strcat(expr,arg);
        elen += len;
        free(arg);
    }
    expr[elen] = '\0';
}
void BinaryOp(token op, seqIntStack *data)
{
    int num1,num2;

    if(IntStack_isEmpty(data))
    {
        // lack right num
        hasError = 1;
        return;
    }
    else {
        num2 = IntStack_top(data);
        IntStack_pop(data);
    }

    if(IntStack_isEmpty(data))
    {
        //lack left num
        hasError = 1;
        return;
    }
    else {
        num1 = IntStack_top(data);
        IntStack_pop(data);
    }

    switch (op)
    {
    case ADD:IntStack_push(data,num1 + num2);break;
    case SUB:IntStack_push(data,num1 - num2);break;
    case MULTI:IntStack_push(data,num1 * num2);break;
    case DIV:IntStack_push(data,num1 / num2);break;
    case EXP:IntStack_push(data,pow(num1,num2));break;
    default:
        hasError = 1;
        return;
    }
}
token getOp(int *value)
{
    if(*expr == '\0')return EOL;

    if(*expr <= '9' && *expr >= '0')
    {
        *value = 0;
        while(*expr >= '0' && *expr <= '9'){
            *value = *value * 10 + (*expr - '0');
            expr++;
        }
        return VALUE;
    }

    switch (*expr)
    {
    case '(':++expr; return OPAREN;
    case ')':++expr; return CPAREN;
    case '+':++expr; return ADD;
    case '-':++expr; return SUB;
    case '*':++expr; return MULTI;
    case '/':++expr; return DIV;
    case '^':++expr; return EXP;
    default:
        hasError = 1;
        return EOL;
    }
}   
int solve_CALC(char *args[])
/*
* only integer calculations are supported
* use the RPN, a num stack and a symbol stack will be used
*/
{
    init_IntStack(&dataStack,10);
    init_TokStack(&opStack,10);
    jointstr(args);
    token lastOp,topOp;
    int result_value,CurrentValue;
    char *str = expr;
    while((lastOp = getOp(&CurrentValue)) != EOL)
    {
        if(hasError)break;
        switch (lastOp)
        {
        case VALUE:IntStack_push(&dataStack,CurrentValue); break;
        case CPAREN:
            while(!TokStack_isEmpty(&opStack) && (topOp = TokStack_top(&opStack)) != OPAREN){
                TokStack_pop(&opStack);
                BinaryOp(topOp,&dataStack);
                if(hasError){
                    expr = str;
                    clear();
                    return NULL;
                }
            }
            if(topOp != OPAREN)
                hasError = 1;
            break;
        case OPAREN:TokStack_push(&opStack,OPAREN);break;
        case EXP:TokStack_push(&opStack,EXP);break;
        case MULTI:
        case DIV:
            while(!TokStack_isEmpty(&opStack) && TokStack_top(&opStack) >= MULTI){
                token t = TokStack_top(&opStack);
                TokStack_pop(&opStack);
                BinaryOp(t,&dataStack);
                if(hasError){
                    expr = str;
                    clear();
                    return NULL;
                }
            }
            TokStack_push(&opStack,lastOp);
            break;
        case ADD:
        case SUB:
            while(!TokStack_isEmpty(&opStack) && TokStack_top(&opStack) != OPAREN){
                token t = TokStack_top(&opStack);
                TokStack_pop(&opStack);
                BinaryOp(t,&dataStack);
                if(hasError){
                    expr = str;
                    clear();
                    return NULL;
                }
            }
            TokStack_push(&opStack,lastOp);
            break;
        default:
            hasError = 1;
            break;
        }
    }
    
    expr = str;
    if(hasError)
    {
        clear();
        return NULL;
    }

    while(!TokStack_isEmpty(&opStack)){
        token t = TokStack_top(&opStack);
        TokStack_pop(&opStack);
        BinaryOp(t,&dataStack);
        if(hasError){
            clear();
            return NULL;
        }
    }

    if(IntStack_isEmpty(&dataStack)){
        hasError = 1;
        clear();
        return NULL;
    }

    result_value = IntStack_top(&dataStack);
    IntStack_pop(&dataStack);

    if(!IntStack_isEmpty(&dataStack)){
        hasError = 1;
        clear();
        return NULL;
    }

    clear();
    return result_value;
}

void clear(){
    if(expr != NULL){
        free(expr);
        expr = NULL;
    }
    free_IntStack(&dataStack);
    free_TokStack(&opStack);
}