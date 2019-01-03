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
	//��һ�㿪ʼ��λ�ã��൱������������λ��
	n = SymTable.findextab[SymTable.ftotal];
	//��һ��εķ��Ž�β���൱�ڵ�ǰ��ķ��ſ�ʼ
	while (n < SymTable.index) {
		if (strcmp(SymTable.element[n].name, name) == 0) {
			return &(SymTable.element[n]);
		}
		n++;
	}
	//��ǰ���û���ҵ������������ȫ�ֱ���
	n = 0;
	while (n < m) {
		if (strcmp(SymTable.element[n].name, name) == 0) {
			return &(SymTable.element[n]);
		}
		n++;
	}
	//����㻹��û���ҵ����϶�Ϊ����
	//error:���ű�δ�ҵ�
	return NULL;
}

Sym* findSym(Sym* cur_func, char *name) {
	int n, n_end;
	int m = SymTable.findextab[1];
	//��һ�㿪ʼ��λ�ã��൱������������λ��
	if (cur_func) {
		if (strcmp(cur_func->name, "main") == 0) {
			n_end = SymTable.index;
			n = SymTable.findextab[SymTable.ftotal];
		}
		else {
			n_end = SymTable.findextab[cur_func->blockindex];
			n = SymTable.findextab[cur_func->blockindex-1];
		}
		//��ǰ��εķ��Ž�β���൱����һ��εķ��ſ�ʼ
		while (n < n_end) {
			if (strcmp(SymTable.element[n].name, name) == 0) {
				return &(SymTable.element[n]);
			}
			n++;
		}
	}
	//��ǰ���û���ҵ������������ȫ�ֱ���
	n = 0;
	while (n < m) {
		if (strcmp(SymTable.element[n].name, name) == 0) {
			return &(SymTable.element[n]);
		}
		n++;
	}
	//����㻹��û���ҵ����϶�Ϊ����
	//error:���ű�δ�ҵ�
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
		cout << "[FITAL] ���Ŷ��峬������ in " << line_no << endl;
		exit(0);
	}
	if (kind == FUNC) {
		for (iter = 1; iter <= SymTable.ftotal; iter++) {
			if (strcmp(SymTable.element[SymTable.findextab[iter]].name, 
				name) == 0) {
				//error: �����ظ�����
				sema_error("�����ظ�����");
				return NULL;
			}
		}
		SymTable.findextab[++SymTable.ftotal] = SymTable.index;
		SymTable.element[SymTable.index].blockindex = SymTable.ftotal + 1;
		//�������壬�����µĲ��
	}
	else {//�Ǻ������壬�ӵ�ǰ�㿪ʼ����
		iter = SymTable.findextab[SymTable.ftotal];
		for (; iter < SymTable.index; iter++) {
			if (strcmp(SymTable.element[iter].name, name) == 0) {
				//error:�����ظ�����
				sema_error("�����ظ�����");
				return NULL;
			}
		}
		//������ǲ�������ȫ�ֱ���������Ҫ��ȫ�ֱ����Ա�
		/*
		if (kind != PARA && non_globle) {
			iter = 0;
			while (iter < SymTable.findextab[1]) {
				if (strcmp(SymTable.element[iter].name, name) == 0) {
					//error�������ظ�����
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

/*�����ں���������������������ű�,
  ֻ�������ж���ĳ��������������飬�����������Ͳ�������
  �������������ڵ��ú���ʱ����������
  ����ջ�ռ��С
  Updata: �������ű�����*/
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