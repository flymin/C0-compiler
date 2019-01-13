#ifndef MID_CODE_H_INCLUDED
#define MID_CODE_H_INCLUDED

#include "var.h"
#include "const.h"
#include "table.h"
#include <vector>
#include <string>

using namespace std;

extern vector<int> temp_counts;
extern int branch;
extern vector<string> str_set;

typedef struct {
	bool  certain;
	int value;
	string name;
}Ret_item;

string new_temp();
void init_temp();
string new_label(Sym* func, string info, bool add);
string typeToString(Type type);

void declare_func_mid(Sym* func);
void declare_para_mid(Type type, Sym* para);
void declare_var_mid(Sym* var);

void call_func_mid(Sym* func);
void call_func_mid(string func);
void exit_mid();

void return_mid(string var);
void return_mid(int num);
void return_mid();

void label_mid(string label);

void cal_mid(Symbol op, string result, string a1, string a2);
void cal_mid(Symbol op, string result, string a1, int a2);
void cal_mid(Symbol op, string result, int a1, string a2);

void assign_mid(string n1, string n2);
void assign_mid(string name, int value);

void push_mid(string name, string temp);
void push_mid(int name);

void return_get_mid(string name);

void branch_zero_mid(string name, string label);
void branch_equal_mid(string name, int value, string label);

void jump_mid(string label);
void jump_link_mid(string label);

void array_get_mid(string array_name, string offset, string result);
void array_get_mid(string array_name, int offset, string result);

void array_set_mid(string array_name, string offset, string value);
void array_set_mid(string array_name, int offset, string value);
void array_set_mid(string array_name, string offset, int value);
void array_set_mid(string array_name, int offset, int value);

void printf_mid(Kind kind, Type type, string token);
void printf_mid(Kind kind, Type type, int v);
void printf_mid();

void scanf_mid(Type type, string v);

void mid(string line);

#endif
