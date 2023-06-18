# smsh
用法smsh.exe or smsh.exe file

**if 格式**
```
if command/[compare]
then
    command
else
    command
fi
```
可嵌套

command如:echo "123"

[compare]如:[ 1 -lt 2 ],其中-lt代表小于,括号之间的空格是必要的,否则报错

**while 格式**
```
while command/[compare]
do
    command
done
```
不支持嵌套

- smsh.c
总框架,接受1个外部参数或者0个,后者直接进入终端模式
    1. next_cmd:从stdin或文件中读取一行
    2. splitline:分割命令及其参数
    3. process:执行对应命令
    4. freelist:释放char **动态数组
    5. fatal:报错函数,为严重错误,立即退出程序
    6. hasError：项目全局变量,用来记录表达式计算是否出错
- splitline.c
读入文件或标准输入流中的一行,并将其分割为命令和参数
    1. next_cmd
    2. splitline
    3. newstr: 将一行中的某个字符串提取出来
    4. freelist
    5. emalloc:手写内存分配
    6. erealloc:手写内存重新分配

- process.c
判断当前是执行if/while/内置命令(set)/命令
  1. iscontrol_command:判断是否为if
  2. do_control_comamnd:执行if
  3. is_loop_command:判断是否为while
  4. do_control_command:执行while
  5. builtin_command:判断内置命令
  6. ok_to_execute:判断当前是否能执行命令
- controlflow.c
流程控制文件,负责while和if流程的处理
稍微复杂,后面再写
- builtin.c
变量赋值管理,可以将表达式计算之后的值赋给变量,如a=[ 458 / 2 + 3 - 5],只支持int类型的整数,不能保证整数不溢出
  1. builtin_command
  2. okname:判断变量名合法
  3. assign:给变量赋值
  4. sh_itoa:将数字转换为字符串

- varlib.c
变量存储,最简单的线性表存储,支持最多200个变量,c可以访问环境变量数组envs,但这了为了简便就没有做,global就是给envs准备的
  1. VLstore:存变量名和其对应的值
  2. new_string:创建字符串值
  3. VLlookup:查找相关变量
  4. find_item:遍历数组返回变量
  5. VList:输出已存在的变量值
  6. Judge_Cmp:是否存在二元比较符
  7. Judge_Op:是否存在二元运算符
  8. solve_CMP:处理比较表达式
  9. Judge_Expr:判断一个表达式是否存在错误
  10. Analyze_Expr:分析当前表达式,然后转去执行对应的比较或计算
  11. solve_CALC:处理计算表达式

- calc.c
处理运算表达式,是一个计算器,有一个运算数栈,一个符号栈,将中缀表达式转为逆波兰式计算,然后返回结果,加上一些错误处理
