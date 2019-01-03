#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED
#include "const.h"
#include <string>

typedef struct {
	char name[TOKEN_MAX_LENTH];
	Kind kind;	//符号种类：CONST，VAR，FUNC，ARRAY，PARA
	Type type;	//符号类型：VOID，CHAR，INT
	int value;	//变量、常量的值；函数存储参数开始的index
	bool global;	//标识符存储的地址，全局或非全局
	int blockindex;	//当前分程序在分程序索引数组中的索引
	int paranum;	//函数参数个数或数组大小,形参的offset
	int psize;		//临时变量和局部变量在栈中所占存储区域的大小
	int vsize;		//局部变量在栈中所占存储空间的大小
	bool return_check_if;		// if 路径中的return
	bool return_check_else;		// else 路径
	bool return_check;			// 其他路径中的return
}Sym;

typedef struct {
	Sym element[maxTab];
	int index;		//符号表栈顶指针，即最后一个空element
	int findextab[MAX_BLOCK_NUM];	//分程序索引数组，指向当前分程序最后一个符号位置
	int ftotal;		//分程序总数
}SymTab;

extern SymTab SymTable;
Sym* findSym(char *name);
Sym* findSym(Sym* cur_func, char *name);
Sym* findFunc(char* name);
Sym* pushTable(char* name, Kind kind, Type type);
int ClearSymTable();
void sema_error(std::string info);
bool is_local_var(std::string curfunc, std::string name);

#endif
