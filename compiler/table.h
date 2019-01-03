#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED
#include "const.h"
#include <string>

typedef struct {
	char name[TOKEN_MAX_LENTH];
	Kind kind;	//�������ࣺCONST��VAR��FUNC��ARRAY��PARA
	Type type;	//�������ͣ�VOID��CHAR��INT
	int value;	//������������ֵ�������洢������ʼ��index
	bool global;	//��ʶ���洢�ĵ�ַ��ȫ�ֻ��ȫ��
	int blockindex;	//��ǰ�ֳ����ڷֳ������������е�����
	int paranum;	//�������������������С,�βε�offset
	int psize;		//��ʱ�����;ֲ�������ջ����ռ�洢����Ĵ�С
	int vsize;		//�ֲ�������ջ����ռ�洢�ռ�Ĵ�С
	bool return_check_if;		// if ·���е�return
	bool return_check_else;		// else ·��
	bool return_check;			// ����·���е�return
}Sym;

typedef struct {
	Sym element[maxTab];
	int index;		//���ű�ջ��ָ�룬�����һ����element
	int findextab[MAX_BLOCK_NUM];	//�ֳ����������飬ָ��ǰ�ֳ������һ������λ��
	int ftotal;		//�ֳ�������
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
