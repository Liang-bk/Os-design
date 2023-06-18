/*to execute commands*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include<string.h>
#include<ctype.h>
#include<io.h>
#include<sys/stat.h>
#include<dirent.h>
#include<windows.h>
#include<tchar.h>
#include"smsh.h"
#include"varlib.h"

//#include<sys/wait.h> win doesn't has this one

int count_num(char **);         // calculate the number of args
char *getstr(char **);
int cd_func(int ,char **);
int cp_func(int ,char *);
int echo_func(int ,char **);
int pwd_func(int ,char **);
int ls_func(char *);
int cat_func(int ,char **);
int rm_func(char *);
int mv_func(int ,char *);
int mkdir_fun(int ,char **);
int rmdir_func(int ,char **);


int execute(char *argv[]){
    if(argv[0] == NULL)
        return 0;
    int ac = count_num(argv);
    char **rargv = (char **)emalloc((ac + 1) * sizeof(char **));
    for(int i = 0; i < ac; i++)
        rargv[i] = replaceVal(argv[i]);
    rargv[ac] = NULL;
    char *rav = getstr(rargv);
    //judge which command to execute according to argv[0]
    //and return the status code of the execution result
    if(strcmp(rargv[0],"cd") == 0){
        return cd_func(ac,rargv);
    }
    else if(strcmp(rargv[0],"cp") == 0){
        return cp_func(ac,rav);
    }
    else if(strcmp(rargv[0],"echo") == 0){
        return echo_func(ac,rargv);
    }
    else if(strcmp(rargv[0],"pwd") == 0){
        return pwd_func(ac,rargv);
    }
    else if(strcmp(rargv[0],"ls") == 0){
        if(ac == 1)
            return ls_func((char *)".");
        else {
            int rv = 0;
            for(int i = 1; i < ac; i++)
            {
                if(ls_func(rargv[i]) != 0)
                    rv = -1;
            }
            return rv;
        }
    }
    else if(strcmp(rargv[0],"cat") == 0){
        return cat_func(ac,rargv);
    }
    else if(strcmp(rargv[0],"rm") == 0){
        if(ac != 2){
            fprintf(stderr,"usage: rm dir/file\n");
            return -1;    
        }
        if(rm_func(rargv[1])){
            return 0;
        }
        else return -1;
    }
    else if(strcmp(rargv[0],"mv") == 0){
        return mv_func(ac,rav);
    }
    else if(strcmp(rargv[0],"mkdir") == 0){
        return mkdir_fun(ac,rargv);
    }
    else if(strcmp(rargv[0],"rmdir") == 0){
        return rmdir_func(ac,rargv);
    }
    else if(strcmp(rargv[0],"exit") == 0){
        exit(0);
    }
    else if(strcmp(rargv[0],"clear") == 0){
        return system("cls");
    }
    else if(strcmp(rargv[0],"[") == 0){
        return Analyze_Expr(argv + 1);
    }

    freelist(rargv);
    free(rav);
    fprintf(stderr,"no command like %s\n",argv[0]);
	return -1;
}

int count_num(char **argv)
{
    int num = 0;
    for(char **cp = argv; *cp != NULL; cp++)
        num++;
    return num;
}

char *getstr(char **av)
{
    char *buf = (char *)emalloc(BUFSIZ);
    int pos = 0;
    int bufspace = BUFSIZ;
    char **cp = av;
    for(;*cp != NULL; cp++)
    {
        int len = strlen(*cp);
        if(bufspace - pos <= len + 1){
            buf = (char *)erealloc(buf,bufspace + len + BUFSIZ);
            bufspace += len + BUFSIZ;
        }
        buf[pos] = '\0';
        strcat(buf,*cp);
        strcat(buf," ");
        pos += len + 1;
    }
    buf[pos] = '\0';
    return buf;
}
int system_call(char *command)
{
    char *cmd = (char *)emalloc(strlen(command) + 20);
    strcpy(cmd,"powershell.exe ");
    strcat(cmd,command);
    int rv = system(cmd);
    free(cmd);
    return rv;
}
int cd_func(int ac,char **av)
{
    int rv = 0;
    if(ac != 2){
        fprintf(stderr,"usage: cd destination\n");
        return -1;
    }
    char old_dir[4096] = "";
    getcwd(old_dir,4096);
    rv = chdir(av[1]);
    if(rv == -1)
        fprintf(stderr,"Error chaning directory\n");
    return rv;
}

/*echo*/
void print_escaped_string(const char *str)
{
    // add a little escape functionality 
    int escape = 0;
    for(int i = 0; str[i] != '\0'; i++){
        if(escape){
            switch (str[i])
            {
            case 'n':putchar('\n');break;
            case 't':putchar('\t');break;
            case '\\':putchar('\\');break;
            case '\'':putchar('\'');break;
            case '\"':putchar('\"');break;
            default:putchar(str[i]);break;
            }
            escape = 0;
        }
        else {
            if(str[i] == '\\')escape = 1;
            else putchar(str[i]);
        }
    }
}
int echo_func(int ac,char *av[]){
    for(int i = 1; i < ac; i++){
        print_escaped_string(av[i]);
        if(i < ac - 1)putchar(' ');
    }
    puts("");
    return 0;
}
/*echo*/

int cp_func(int ac,char *av){
    int rv = 0;
    if(ac != 3){
        fprintf(stderr,"usage: cp source destination\n");
        return -1;
    }
    rv = system_call(av);
    return rv;
}

int pwd_func(int ac,char *av[])
{
    if(ac != 1){
        fprintf(stderr,"usage: pwd\n");
        return -1;
    }
    char dir[4096] = "";
    getcwd(dir,4096);
    printf("%s\n",dir);
    return 0;
}

int ls_func(char *path)
{
    int rv = 0;
    WIN32_FIND_DATA find_data;
    HANDLE h_find;
    TCHAR search_path[MAX_PATH];
    SYSTEMTIME st_mtime;
    TCHAR file_permissions[4];
    // transform char * to TCHAR *
    #ifdef UNICODE
        MultiByteToWideChar(CP_ACP,0,path,-1,search_path,MAX_PATH);
    #else   
        strncpy(search_path,path,MAX_PATH);
    #endif
    _stprintf(search_path,_T("%s\\*"),search_path);
    h_find = FindFirstFile(search_path, &find_data);
    if (h_find == INVALID_HANDLE_VALUE) {
        _tprintf(_T("Error: %d, illegal path\n"), GetLastError());
        return -1;
    }

    do {
        // get the file permissions
        file_permissions[0] = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _T('d') : _T('-');
        file_permissions[1] = (find_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? _T('r') : _T('-');
        file_permissions[2] = (find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? _T('h') : _T('-');
        file_permissions[3] = _T('\0');

        // get the file size
        DWORD file_size = find_data.nFileSizeLow;

        // get the file modification time
        FileTimeToSystemTime(&find_data.ftLastWriteTime, &st_mtime);

        _tprintf(_T("%s %10u %02d-%02d-%04d %02d:%02d %s\n"),
                 file_permissions,
                 file_size,
                 st_mtime.wMonth,
                 st_mtime.wDay,
                 st_mtime.wYear,
                 st_mtime.wHour,
                 st_mtime.wMinute,
                 find_data.cFileName);

    } while (FindNextFile(h_find, &find_data) != 0);

    FindClose(h_find);
    return rv;
}
int cat_func(int ac,char *av[])
{
    int rv = 0;
    if(ac != 2){
        fprintf(stderr,"usage: cat file\n");
        return -1;
    }
    char buf[4096];
    int in_fd,n_chars = 0;
    if((in_fd = open(av[1],O_RDONLY)) == -1){
        fprintf(stderr,"Error: can not open %s\n",av[1]);
        return -1;
    }
    while((n_chars = read(in_fd,buf,4096)) > 0)
        fputs(buf,stdout);
    puts("");
    if(n_chars == -1){
        fprintf(stderr,"Error: read error from %s\n",av[1]);
        rv = -1;
    }
    if(close(in_fd) == -1){
        fprintf(stderr,"Error: close file %s error",av[1]);
        rv = -1;
    }
    return rv;
}
int rm_func(char *path)
/*
* recursively delete files or directories
* return 1 if success, 0 if fail
*/
{
    struct stat dir_stat;
    if(access(path,F_OK) != 0)
    {
        fprintf(stderr,"%s not exist\n",path);
        return 0;
    }
    if(stat(path,&dir_stat) < 0){
        fprintf(stderr,"%s stat problem\n",path);
        return 0;
    }
    if(S_ISREG(dir_stat.st_mode)){
        remove(path);
        return 1;
    }
    DIR *dir;
    struct dirent *entry;
    char file_path[PATH_MAX];

    dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr,"opendir %s error\n",path);
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);
        stat(file_path,&dir_stat);
        if(S_ISDIR(dir_stat.st_mode)){
            rm_func(file_path);
        } else {
            remove(file_path);
        }
    }
    closedir(dir);
    if (rmdir(path) != 0) {
        fprintf(stderr,"can not remove %s\n",path);
        return 0;
    }
    return 1;
}
int mv_func(int ac,char *av)
{
    int rv = 0;
    if(ac != 3){
        fprintf(stderr,"usage: mv source destination\n");
        return -1;
    }
    rv = system_call(av);
    return rv;
}
int mkdir_fun(int ac,char *av[])
{
    int rv = 0;
    if(ac != 2){
        fprintf(stderr,"usage: mkdir dir\n");
        return -1;
    }
    rv = mkdir(av[1]);
    if(rv != 0)
        fprintf(stderr,"can not create %s\n",av);
    return rv;
}
int rmdir_func(int ac,char *av[])
{
    int rv = 0;
    if(ac != 2){
        fprintf(stderr,"usage: rmdir dir\n");
        return -1;
    }
    rv = rmdir(av[1]);
    if(rv != 0)
        fprintf(stderr,"can not remove %s\n",av);
    return rv;
}
char *replaceVal(char *argv)
{
    char *buf;
    int bufspace = 0;
    int pos = 0;
    char *cp;
    int isVal = 0;
    
    char *name;
    int nSize = 0;
    int npos = 0;
    for(cp = argv; *cp; cp++){
        if(pos + 1 >= bufspace){
            if(bufspace == 0)
                buf = (char *)emalloc(BUFSIZ);
            else buf = (char *)erealloc(buf,bufspace + BUFSIZ);
            bufspace += BUFSIZ;
        }
        if(isVal){
            while(*cp){
                if(npos + 1 >= nSize){
                    if(nSize == 0)
                        name = (char *)emalloc(BUFSIZ);
                    else name = (char *)erealloc(name,nSize + BUFSIZ);
                    nSize += BUFSIZ;
                }
                if(isdigit(*cp) || isalnum(*cp) || *cp == '_')name[npos++] = *cp;
                else {
                    name[npos++] = '\0';
                    char *realV = VLlookup(name);
                    int len = strlen(realV);
                    if(len >= (bufspace - pos))
                        buf = (char *)erealloc(buf,bufspace + len + 1);
                    buf[pos] = '\0';
                    strcat(buf,realV);
                    pos += len;
                    free(name);
                    name = NULL;
                    isVal = nSize = npos = 0;
                    break;
                }

                if(*(cp + 1) == '\0'){
                    cp++;
                    name[npos++] = '\0';
                    char *realV = VLlookup(name);
                    int len = strlen(realV);
                    if(len >= (bufspace - pos))
                        buf = (char *)erealloc(buf,bufspace + len + 1);
                    buf[pos] = '\0';
                    strcat(buf,realV);
                    pos += len;
                    free(name);
                    name = NULL;
                    isVal = nSize = npos = 0;
                    break;
                }
                cp++;
            }
        }
        if(*cp == '\0')break;

        if(*cp != '$')
            buf[pos++] = *cp;
        
        if(*cp == '$'){
            isVal = 1;
        }
    }
    if(*cp == '\0' && pos == 0)
        return NULL;
    buf[pos] = '\0';
    return buf;
}