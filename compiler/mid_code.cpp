#include <iostream>
#include <fstream>
#include "mid_code.h"
#include "var.h"
#include "grammar.h"
#include "const.h"
#include "table.h"
#include "words.h"
#include <sstream>
#include <vector>

extern fstream midfile;

#define LINE 1
//#define LEFT cout
#define LEFT midfile
#define RIGHT endl
#define OUTPUT(x) LEFT << x << RIGHT

using namespace std;

vector<int> temp_counts;
int branch;

string new_temp() {
	int last_index = temp_counts.size() - 1;
	int tempno = temp_counts[last_index];   // get last temp
	stringstream temp_name;
	temp_name << "#" << tempno;
	temp_counts[last_index] ++;
	return temp_name.str();
}

void init_temp() {
	if (temp_counts.size() != 1) {
		//error:temp_counts 错误
		exit(0);
	}
}

string new_label(Sym* func, string info, bool add) {
	string label;
	if (add) {
		label = string(func->name) + "_L_" + to_string(branch++) + "_" + info;
	}
	else {
		label = string(func->name) + "_L_" + to_string(branch) + "_" + info;
	}
	return label;
}

string typeToString(Type type) {
	switch (type) {
	case VOID:
		return "VOID";
	case CHAR:
		return "CHAR";
	case INT:
		return "INT";
	default:
		return "NOTYPE";
	}
}


void declare_func_mid(Sym* func) {
	string name = string(func->name);
	OUTPUT("@func " << name);
}

void declare_para_mid(Type type, Sym* para) {
	string name = string(para->name);
	OUTPUT("@para " << typeToString(type) << " " << name);
}

void declare_var_mid(Sym* var){
	if (var->kind == ARRAY)
	{
		OUTPUT("@array " << typeToString(var->type) << " " << string(var->name) << " " << var->paranum);
	}
	else
	{
		OUTPUT("@var " << typeToString(var->type) << " " << string(var->name));
	}
}

void call_func_mid(Sym* func){
	OUTPUT("@call " + string(func->name));
}

void call_func_mid(string func) {
	OUTPUT("@call " + func);
}

void exit_mid() {
	OUTPUT("@exit");
}

void return_mid(string var){
	OUTPUT("@ret " << string(var));
}

void return_mid(int num){
	OUTPUT("@ret " << num);
}

void return_mid() {
	OUTPUT("@ret ");
}

void label_mid(string label){
	OUTPUT(string(label) << " :");
}

void cal_mid(Symbol op, string result, string a1, string a2){
	OUTPUT(result << " = " << a1 << " " << symbol2string(op) << " " << a2);
}

void cal_mid(Symbol op, string result, string a1, int a2){
	stringstream ss;
	ss << a2;
	cal_mid(op, result, a1, (string)ss.str().data());
}

void cal_mid(Symbol op, string result, int a1, string a2){
	stringstream ss;
	ss << a1;
	cal_mid(op, result, (string)ss.str().data(), a2);
}

void assign_mid(string n1, string n2){
	OUTPUT(n1 << " = " << n2);
}

void assign_mid(string name, int value){
	OUTPUT(name << " = " << value);
}

/*@push动作用来完成函数传参过程*/
void push_mid(string name, string temp){
	OUTPUT(temp << " = " << name);
	OUTPUT("@push " << string(temp));
}

void push_mid(int name){
	OUTPUT("@push " << name);
}

/*接收有返回值的函数返回值
  在文法中又返回值函数调用只在因子解析中出现*/
void return_get_mid(string name){
	OUTPUT("@get " << string(name));
}

/*有条件跳转动作*/
void branch_zero_mid(string name, string label){
	OUTPUT("@bz " << string(name) << " " << string(label));
}

void branch_equal_mid(string name, int value, string label){
	OUTPUT("@be " << string(name) << " " << value << " " << string(label));
}

/*无条件跳转动作*/
void jump_mid(string label){
	OUTPUT("@j " << label);
}

void jump_link_mid(string label){
	OUTPUT("@jal " << label);
}

/*数组取值*/
void array_get_mid(string array_name, string offset, string result){
	OUTPUT(result << " = " << array_name << " ARRGET " << offset);
}

void array_get_mid(string array_name, int offset, string result){
	OUTPUT(result << " = " << array_name << " ARRGET " << offset);
}

/*数组赋值*/
void array_set_mid(string array_name, string offset, string value){
	OUTPUT(array_name << " ARRSET " << offset  << " " << value);
}

void array_set_mid(string array_name, int offset, string value){
	OUTPUT(array_name << " ARRSET " << offset << " " << value);
}

void array_set_mid(string array_name, string offset, int value){
	OUTPUT(array_name << " ARRSET " << offset << " " << value);
}

void array_set_mid(string array_name, int offset, int value){
	OUTPUT(array_name << " ARRSET " << offset << " " << value);
}

/*字符串输出处理*/
vector<string> str_set;
void printf_mid(Kind kind, Type type, string token){
	string v = string(token);
	if (kind == ARRAY && type == CHAR){
		int len = str_set.size();
		for (int i = 0; i < len; i++){
			if (str_set[i] == v){
				OUTPUT("@printf STRING S_" << i);
				return;
			}
		}
		str_set.push_back(v);
		OUTPUT("@printf STRING S_" << len);
	}
	else{
		OUTPUT("@printf " << typeToString(type) << " " << v);
	}
}

void printf_mid() {
	if (LINE) {
		OUTPUT("@printf LINE");
	}
}

void printf_mid(Kind kind, Type type, int v){
	OUTPUT("@printf " << typeToString(type) << " " << v);
}

/*输入处理*/
void scanf_mid(Type type, string v){
	OUTPUT("@scanf " << typeToString(type) << " " << v);
}

void mid(string line){
	OUTPUT(line);
}