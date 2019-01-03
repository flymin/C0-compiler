#include<iostream>
#include<fstream>
#include<string>
#include "var.h"
#include "const.h"
#include "words.h"
#include "grammar.h"
#include "table.h"
#include <stdarg.h>
#include <assert.h>

SymTab SymTable;
bool non_globle = true;
extern bool success;
extern int line_no;
extern char line_buffer[LINE_MAX_LENGTH];

bool is_local_var(string curfunc, string name) {
	Sym *func = findFunc((char *)curfunc.data());
	Sym *var = findSym(func, (char *)name.data());
	if (!var) {
		return false;
	}
	else {
		return !var->global;
	}
}

void sema_error(string info){
	success = false;
	cout << line_buffer << endl;
	cout << "[SEMA ERROR] " << info << " in " << line_no << endl;
}

Sym* findSym(char *name) {
	int n;
	int m = SymTable.findextab[1];
	//第一层开始的位置，相当于最外层结束的位置
	n = SymTable.findextab[SymTable.ftotal];
	//上一层次的符号结尾，相当于当前层的符号开始
	while (n < SymTable.index) {
		if (strcmp(SymTable.element[n].name, name) == 0) {
			return &(SymTable.element[n]);
		}
		n++;
	}
	//当前层次没有找到，到最外层找全局变量
	n = 0;
	while (n < m) {
		if (strcmp(SymTable.element[n].name, name) == 0) {
			return &(SymTable.element[n]);
		}
		n++;
	}
	//最外层还是没有找到，认定为错误
	//error:符号表未找到
	return NULL;
}

Sym* findSym(Sym* cur_func, char *name) {
	int n, n_end;
	int m = SymTable.findextab[1];
	//第一层开始的位置，相当于最外层结束的位置
	if (cur_func) {
		if (strcmp(cur_func->name, "main") == 0) {
			n_end = SymTable.index;
			n = SymTable.findextab[SymTable.ftotal];
		}
		else {
			n_end = SymTable.findextab[cur_func->blockindex];
			n = SymTable.findextab[cur_func->blockindex-1];
		}
		//当前层次的符号结尾，相当于上一层次的符号开始
		while (n < n_end) {
			if (strcmp(SymTable.element[n].name, name) == 0) {
				return &(SymTable.element[n]);
			}
			n++;
		}
	}
	//当前层次没有找到，到最外层找全局变量
	n = 0;
	while (n < m) {
		if (strcmp(SymTable.element[n].name, name) == 0) {
			return &(SymTable.element[n]);
		}
		n++;
	}
	//最外层还是没有找到，认定为错误
	//error:符号表未找到
	return NULL;
}

Sym* findFunc(char* name) {
	int n = 1;
	while (n <= SymTable.ftotal) {
		if (strcmp(SymTable.element[SymTable.findextab[n]].name, name) == 0) {
			return &SymTable.element[SymTable.findextab[n]];
		}
		n++;
	}
	return NULL;
}
Sym* pushTable(char* name, Kind kind, Type type) {
	int iter;
	if (SymTable.index >= maxTab) {
		cout << line_buffer << endl;
		cout << "[FITAL] 符号定义超过限制 in " << line_no << endl;
		exit(0);
	}
	if (kind == FUNC) {
		for (iter = 1; iter <= SymTable.ftotal; iter++) {
			if (strcmp(SymTable.element[SymTable.findextab[iter]].name, 
				name) == 0) {
				//error: 函数重复定义
				sema_error("函数重复定义");
				return NULL;
			}
		}
		SymTable.findextab[++SymTable.ftotal] = SymTable.index;
		SymTable.element[SymTable.index].blockindex = SymTable.ftotal + 1;
		//函数定义，创建新的层次
	}
	else {//非函数定义，从当前层开始查找
		iter = SymTable.findextab[SymTable.ftotal];
		for (; iter < SymTable.index; iter++) {
			if (strcmp(SymTable.element[iter].name, name) == 0) {
				//error:符号重复定义
				sema_error("符号重复定义");
				return NULL;
			}
		}
		//如果不是参数或者全局变量，还需要和全局变量对比
		/*
		if (kind != PARA && non_globle) {
			iter = 0;
			while (iter < SymTable.findextab[1]) {
				if (strcmp(SymTable.element[iter].name, name) == 0) {
					//error：符号重复定义
					return NULL;
				}
				iter++;
			}
		}
		*/
	}

	strcpy_s(SymTable.element[SymTable.index].name, name);
	SymTable.element[SymTable.index].kind = kind;
	SymTable.element[SymTable.index].type = type;
	SymTable.index++;
	return &(SymTable.element[SymTable.index - 1]);
}

/*用来在函数分析结束后清理本层符号表,
  只清理函数中定义的常量、变量和数组，保留函数名和参数定义
  参数定义用来在调用函数时检查参数类型
  返回栈空间大小
  Updata: 保留符号表内容*/
int ClearSymTable() {
	int i = SymTable.index - 1;
	int size = 0;
	while (SymTable.element[i].kind == CONST ||
		SymTable.element[i].kind == VAR ||
		SymTable.element[i].kind == ARRAY
		) {
		if (SymTable.element[i].kind == ARRAY) {
			size += SymTable.element[i].paranum * 4;
		}
		else {
			size += 4;
		}
		//SymTable.element[i].kind = NOKIND;
		//SymTable.element[i].ref = 0;
		//SymTable.element[i].paranum = 0;
		//SymTable.element[i].type = NOTYPE;
		//SymTable.element[i].value = 0;
		//SymTable.element[i].name[0] = '\0';
		i--;
	}
	//SymTable.index = i + 1;
	while (SymTable.element[i].kind == PARA) {
		//SymTable.element[i].ref = 0;
		//SymTable.element[i].paranum = 0;
		//SymTable.element[i].value = 0;
		//SymTable.element[i].name[0] = '\0';
		i--;
	}
	return size;
}