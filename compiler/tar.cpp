#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <assert.h>
#include "mid_code.h"
#include "var.h"
#include "grammar.h"
#include "const.h"
#include "table.h"
#include "words.h"
#include "tar.h"


using namespace std;

#ifndef NEW_TAR

#if 0
#define OUT_LEFT cout
#define OUT_RIGHT endl
#else
#define OUT_LEFT tarfile
#define OUT_RIGHT endl
#endif // DEBUG
#define OUTPUT(x) OUT_LEFT << x << OUT_RIGHT

/*extern from main.cpp*/
extern ofstream tarfile;
extern fstream midfile;

/*本文件中的全局变量*/
typedef map<string, int> STRINT_MAP;
Sym* cur_func = NULL;
int temp_base_addr = 0;
int ptr = 0;
int cur_addr = 0;
int para_read_count = 0;
STRINT_MAP offset_map;
STRINT_MAP global_addr_map;

vector<string> paras;

template <typename T>
T get_element(string name, map<string, T> from_map) {
	typename map<string, T>::iterator it = from_map.find(name);
	if (it != from_map.end()) {
		return it->second;
	}
	return NULL;
}

bool is_temp(string name) {
	return name[0] == '#';
}

int get_temp_no(string name) {
	name.erase(0, 1);
	int i;
	sscanf_s(name.c_str(), "%d", &i);
	return i;
}

bool is_num(string str) {
	int len = str.size();
	int i;
	if (str.at(0) == '-') {
		i = 1;
	}
	else {
		i = 0;
	}
	for (; i < len; i++) {
		if (str.at(i) < '0' || str.at(i) > '9') {
			return false;
		}
	}
	return true;
}

void save(string varname, string reg) {
	int addr;
	Sym* var;
	if (is_temp(varname)) {
		addr = 4 * get_temp_no(varname) + temp_base_addr;
		OUTPUT("sw " << reg << ", -" << addr << "($fp)");
	}
	else {
		var = findSym(cur_func, (char*)varname.data());
		if (var->global) { // is global variable
			OUTPUT("sw " << reg << ", -" << global_addr_map[var->name] << "($gp)");
		}
		else {
			OUTPUT("sw " << reg << ", -" << offset_map[var->name] << "($fp)");
		}
	}
}

void load(string varname, string reg) {
	int addr;
	Sym* var;
	if (is_temp(varname)) {
		addr = 4* get_temp_no(varname) + temp_base_addr;
		OUTPUT("lw " << reg << ", -" << addr << "($fp)");
	}
	else {
		var = findSym(cur_func, (char*)varname.data());
		if (var->global) { // is global variable
			OUTPUT("lw " << reg << ", -" << global_addr_map[var->name] << "($gp)");
		}
		else {
			OUTPUT("lw " << reg << ", -" << offset_map[var->name] << "($fp)");
		}
	}
}

void assign_para_tar(Sym* para) {
	assert(cur_addr >= 12);
	//TODO 注意使用了s寄存器之后的分配调整
	int addr = cur_addr + para_read_count * 4;
	para_read_count++;
	para->paranum = addr;
}

void init_var(Sym* var_item){
	if (var_item->kind == ARRAY){
		//var_item->ref = ptr;
		if (cur_func == NULL) {
			var_item->global = true;
			global_addr_map[var_item->name] = ptr;
		}
		else {
			var_item->global = false;
			offset_map[var_item->name] = ptr;
		}
		ptr += 4 * var_item->paranum;
		//TODO 暂时所有变量都按照4字节计算，之后修改
		/*
		if (var_item->type == CHAR){
			if (int_ptr > char_ptr){
				char_ptr = int_ptr;
			}
			if (cur_func == NULL){
				global_addr_map[var_item->get_name()] = char_ptr;
			}
			else{
				offset_map[var_item->get_name()] = char_ptr;
			}
			char_ptr += var_item->get_len();
		}
		else if (var_item->type == INT){
			if (char_ptr > int_ptr){
				int_ptr = round_up(char_ptr, 4);
			}
			if (cur_func == NULL){
				global_addr_map[var_item->get_name()] = int_ptr;
			}
			else{
				offset_map[var_item->get_name()] = int_ptr;
			}
			int_ptr += 4 * var_item->paranum;
		}
		else{
			error_debug("unknown type");
		}
		*/
	}
	else{
		//var_item->ref = ptr;
		if (cur_func == NULL) {
			var_item->global = true;
			global_addr_map[var_item->name] = ptr;
		}
		else {
			var_item->global = false;
			offset_map[var_item->name] = ptr;
		}
		ptr += 4;
		/*
		if (var_item->get_type() == CHAR){
			if (char_ptr % 4 == 0 && int_ptr > char_ptr){
				char_ptr = int_ptr;
			}
			if (cur_func == NULL){
				global_addr_map[var_item->get_name()] = char_ptr;
			}
			else{
				offset_map[var_item->get_name()] = char_ptr;
			}
			//(cur_func == NULL ? global_addr_map : offset_map).insert(STRINT_MAP::value_type(var_item->get_name(), char_ptr));
			char_ptr += 1;
		}
		else if (var_item->get_type() == INT){
			if (char_ptr > int_ptr){
				int_ptr = round_up(char_ptr, 4);
			}
			if (cur_func == NULL){
				global_addr_map[var_item->get_name()] = int_ptr;
			}
			else{
				offset_map[var_item->get_name()] = int_ptr;
			}
			//(cur_func == NULL ? global_addr_map : offset_map).insert(STRINT_MAP::value_type(var_item->get_name(), int_ptr));
			int_ptr += 4;
		}
		else
		{
			//error_debug("unknown type");
		}
		*/
	}
	//temp_base_addr = round_up((int_ptr > char_ptr) ? int_ptr : char_ptr, 4);
	temp_base_addr = ptr;
}

/*	push参数，清空paras，保存现场（sp、fp、ra、s）
	TODO 由于没有分配寄存器，未保存s寄存器
*/
void call_tar(string funcname) {
	Sym* var = NULL;	//实参的结构指针
	Sym* func = NULL;	//被调函数的结构指针
	func = findFunc((char*)funcname.data());
	//这里保存s寄存器，并调整temp_addr
	int temp_addr;
	temp_addr = 12;//ra,fp,sp
	if (funcname != "main") {
		// store paras
		int len = func->paranum;
		for (int i = len-1; i >= 0; i--) {
			//addr用来参数被调函数中的offset
			//temp_addr应该指向被调函数栈中的参数开始区域
			int addr = temp_addr + i * 4;
			string paraname = paras.back();
			//paras.erase(paras.begin());
			paras.pop_back();
			if (is_num(paraname)) {
				OUTPUT("li $t0, " << paraname);
				OUTPUT("sw $t0, -" << addr << "($sp)");
			}
			else {
				load(paraname, "$t0");
				OUTPUT("sw $t0, -" << addr << "($sp)");
			}
		}

		OUTPUT("sw $ra, 0($sp)");
		OUTPUT("sw $fp, -4($sp)");
		//OUTPUT("sw $sp, -8($sp)");
		// refresh $fp
		OUTPUT("addu $fp, $sp, $0");
		OUTPUT("addi $sp, $sp, -" << temp_addr + 4 * func->paranum + func->psize);
		// jump
		OUTPUT("jal " << funcname << "_E");
		// load regs
		OUTPUT("addi $sp, $sp, " << temp_addr + 4 * func->paranum + func->psize);
		OUTPUT("lw $ra, 0($sp)");
		OUTPUT("lw $fp, -4($sp)");
	}
	else {
		// refresh $fp
		//OUTPUT("add $sp, $fp, $gp");
		//global pointer有初始值，直接使用全局空间
		OUTPUT("addu $fp, $sp, $0");
		OUTPUT("sw $ra, 0($sp)");
		OUTPUT("sw $fp, -4($sp)");
		//OUTPUT("sw $sp, 8($sp)");
		OUTPUT("addi $sp, $sp, -" << temp_addr + 4 * func->paranum + func->psize);
		// jump
		OUTPUT("jal " << funcname << "_E");
		OUTPUT("li $v0, 10");		//程序出口调用
		OUTPUT("syscall");
	}
}

void init_func(string funcname) {
	offset_map.clear();
	cur_func = findFunc((char*)funcname.data());
	//TODO ptr需要加上s寄存器的空间
	ptr = 12;
	temp_base_addr = ptr;
	cur_addr = 12;
	para_read_count = 0;
	OUTPUT(funcname << "_E:");
}

//目前使用t0, t1进行计算，结果存在t0
void cal_tar(string op, string tar_str, string cal_str1, string cal_str2) {
	stringstream mips;		//mips指令缓存
	bool is_cal = true;		//true计算或者false比较
	int immed1, immed2;
	bool is_immed1 = is_num(cal_str1);
	bool is_immed2 = is_num(cal_str2);
	vector<string> conf_names;
	conf_names.push_back(tar_str);
	conf_names.push_back(cal_str1);
	conf_names.push_back(cal_str2);
	if (is_immed1) {
		sscanf_s(cal_str1.c_str(), "%d", &immed1);  // get immediate
	}
	if (is_immed2) {
		sscanf_s(cal_str2.c_str(), "%d", &immed2);  // get immediate
	}

	if (op == "ADD") {
		mips << (is_immed2 ? "addi" : "addu");
		is_cal = true;

	}
	else if (op == "SUB") {
		mips << (is_immed2 ? "addi" : "subu");
		if (is_immed2) immed2 = -immed2;   // turn negative
		is_cal = true;

	}
	else if (op == "MUL") {
		mips << "mul";
		is_cal = true;

	}
	else if (op == "DIV") {
		mips << "div";
		is_cal = true;

	}
	else if (op == "LE") { // <=
		mips << "sle";
		is_cal = false;

	}
	else if (op == "GE") {
		mips << "sge";
		is_cal = false;

	}
	else if (op == "LT") {
		mips << "slt";
		is_cal = false;

	}
	else if (op == "GT") {
		mips << "sgt";
		is_cal = false;

	}
	else if (op == "NEQ") {
		mips << "sne";
		is_cal = false;

	}
	else if (op == "EQ") {
		mips << "seq";
		is_cal = false;

	}
	else {
		//error_debug((string)"unknown op \'" + op + "\'");
	}

	if (is_num(tar_str)) {
		//error_debug("tar is number");
	}
	else {
		//mips << " $s" << get_reg(tar_str, &conf_names);
		mips << " $t0";
	}

	if (is_immed1) {
		if (immed1 == 0) {
			mips << ", $0";
		}
		else if (!is_cal) {
			OUTPUT("li $t0, " << immed1);
			mips << ", $t0";
		}
		else {
			//error_debug("cal1 is a number");
		}
	}
	else {
		load(cal_str1, "$t0");
		//mips << ", $s" << get_reg(cal_str1, &conf_names);
		mips << ", $t0";
	}


	if (is_immed2) {
		if (is_cal) {
			mips << ", " << immed2;
		}
		else {
			OUTPUT("li $t1, " << immed2);
			mips << ", $t1";
		}
	}
	else {
		load(cal_str2, "$t1");
		//mips << ", $s" << get_reg(cal_str2, &conf_names);
		mips << ", $t1";
	}

	OUTPUT(mips.str());
	save(tar_str, "$t0");
}

/*true赋值，false取数*/
void array_tar(string arr_str, string off_str, string var, bool is_set) {
	Sym* item = findSym(cur_func, (char*)arr_str.data());
	Type type;
	if (item == NULL) {
		//error_debug("unknown array \'" + arr_str + "\'");
	}
	else {
		type = item->type;
	}
	bool offset_is_immed = is_num(off_str);
	bool value_is_immed = is_num(var);
	// get reg
	string reg;
	if (value_is_immed) {
		reg = "$t0";
		OUTPUT("li $t0, " << var);
		if (!is_set) {
			//error_debug("array to a value");
		}
	}
	else {
		//stringstream ss;
		//ss << "$s" << get_reg(var);
		//reg = ss.str();
		reg = "$t0";
		if (is_set) {
			load(var, "$t0");
		}
	}

	// get op
	string op;
	op = is_set ? "sw" : "lw";
	/*
	string op;
	if (type == INT) {
		op = is_set ? "sw" : "lw";
	}
	else {
		op = is_set ? "sb" : "lb";
	}
	*/

	int offset;
	string point_reg;
	STRINT_MAP::iterator it = offset_map.find(arr_str);
	if (it != offset_map.end()) {
		offset = it->second;
		point_reg = "$fp";
	}
	else {
		it = global_addr_map.find(arr_str);
		if (it != global_addr_map.end()) {
			offset = it->second;
			point_reg = "$gp";
		}
		else {
			//error_debug("cannot found array");
		}
	}

	if (offset_is_immed) {
		int ele_offset;
		sscanf_s(off_str.c_str(), "%d", &ele_offset);
		//if (type == INT) {
		//	ele_offset *= 4;
		//}
		ele_offset *= 4;
		OUTPUT(op << " " << reg << ", -" << offset + ele_offset << "(" << point_reg << ")");

	}
	else {
		//if (type == INT) {
		//	load(off_str, "$t1");
		//	OUTPUT("sll $t1, $t1, 2");  // offset *= 4
		//	OUTPUT("add $t1, $t1, " << point_reg);
		//}
		//else {
		//	OUTPUT("add $t1, $s" << get_reg(off_str) << ", " << point_reg);
		//}
		load(off_str, "$t1");
		OUTPUT("sll $t1, $t1, 2");  // offset *= 4
		OUTPUT("subu $t1, " << point_reg << ", $t1");

		OUTPUT("addi $t1, $t1, -" << offset);  // add array base
		OUTPUT(op << " " << reg << ", 0($t1)");
	}
	if (!is_set) {
		save(var, "$t0");
	}
}

void name_handle(vector<string> strs) {
	int len = strs.size();
	if (len <= 1) {
		//error_debug("too few strs");
	}
	else if (strs[1] == ":") {
		OUTPUT(strs[0] << ":");    //[MIPS]标签，直接输出
	}
	else if (strs[1] == "ARRSET") {		//数组赋值语句
		array_tar(strs[0], strs[2], strs[3], true);
	}
	else if (strs[1] != "=") {			//其他语句都有等号
		//error_debug("without equal, " + strs[0]);
	}
	else if (len == 3) {  // 长度为3的只有赋值语句
		vector<string> conf_names;
		conf_names.push_back(strs[0]);
		conf_names.push_back(strs[2]);
		if (is_num(strs[0])) {
			//error_debug("assign to number");
		}
		if (is_num(strs[2])) {
			// [MIPS] li加载立即数赋值
			//OUTPUT("li $s" << get_reg(strs[0], &conf_names) << ", " << strs[2]);
			OUTPUT("li $t0, " << strs[2]);
			save(strs[0], "$t0");
		}
		else {
			// [MIPS] move 加载变量值
			//OUTPUT("move $s" << get_reg(strs[0], &conf_names) << ", $s" << get_reg(strs[2], &conf_names));
			load(strs[2], "$t0");
			save(strs[0], "$t0");
		}
	}
	else if (len == 5) {  // 长度为5的是数组操作语句或者计算语句
		string op = strs[3];
		string tar_str = strs[0];
		string cal_str1 = strs[2];
		string cal_str2 = strs[4];
		if (op == "ARRGET") {
			array_tar(strs[2], strs[4], strs[0], false);
		}
		else {
			cal_tar(op, strs[0], strs[2], strs[4]);	//计算语句
		}
	}
	else {
		//error_debug("too many paras");
	}
}


void read_medis_tar() {
	string line;
	while (getline(midfile, line)) {
		OUTPUT("   # " << line);
		istringstream is(line);
		string str;
		vector<string> strs;
		while (is >> str) {
			strs.push_back(str);
		}
		// para: 从基址中按顺序读取数值，存储至参数对应的地址中(fp之前) OK
		if (strs[0] == "@var" || strs[0] == "@array") {
			init_var(findSym(cur_func, (char*)strs[2].data()));

		}
		else if (strs[0] == "@para") {
			assert(cur_func != NULL);
			Sym* temp;
			temp = findSym(cur_func, (char*)strs[2].data());
			assert(temp->kind == PARA);
			init_var(temp);
			assign_para_tar(temp);

		}
		else if (strs[0] == "@func") { // 初始化函数信息，生成标签    OK
			init_func(strs[1]);

		}
		else if (strs[0] == "@push") { // 将参数保存至vector中（不直接存储是因为还要走表达式）   OK
			paras.push_back(strs[1]);

		}
		else if (strs[0] == "@call") {
			call_tar(strs[1]);

		}
		else if (strs[0] == "@get") { // 保存v寄存器的值 OK
			if (is_num(strs[1])) {
				//error_debug("num get");
			}
			else {
				save(strs[1], "$v0");
				//OUTPUT("move $s" << get_reg(strs[1]) << ", $v0");
			}

		}
		else if (strs[0] == "@ret") { // v寄存器赋值，跳转至ra OK
			if (strs.size() == 2) {
				if (is_num(strs[1])) {
					OUTPUT("li $v0, " << strs[1]);
				}
				else {
					load(strs[1], "$v0");
				}
			}
			OUTPUT("jr $ra");
			OUTPUT("nop");

		}
		else if (strs[0] == "@be") {
			if (!is_num(strs[2])) {
				//error_debug("be not num");
			}
			else {
				load(strs[1], "$t0");
				OUTPUT("beq $t0, " << strs[2] << ", " << strs[3]);
				OUTPUT("nop");
			}

		}
		else if (strs[0] == "@bz") {
			load(strs[1], "$t0");
			OUTPUT("beq $t0, $0, " << strs[2]);
			OUTPUT("nop");

		}
		else if (strs[0] == "@j") {
			OUTPUT("j " << strs[1]);
			OUTPUT("nop");

		}
		else if (strs[0] == "@jal") {
			OUTPUT("jal " << strs[1]);
			OUTPUT("nop");

		}
		else if (strs[0] == "@printf") {
			if (strs[1] == "LINE") {
				OUTPUT("li $a0, 10");
				OUTPUT("li $v0, 11");
				OUTPUT("syscall");
			}
			else {
				bool is_immed = is_num(strs[2]);
				if (strs[1] == "STRING") {
					OUTPUT("li $v0, 4");
					OUTPUT("la $a0, " << strs[2]);
					OUTPUT("syscall");
				}
				else if (strs[1] == "INT") {
					OUTPUT("li $v0, 1");
					if (is_immed) {
						OUTPUT("li $a0, " << strs[2]);
					}
					else {
						load(strs[2], "$a0");
					}
					OUTPUT("syscall");

				}
				else if (strs[1] == "CHAR") {
					OUTPUT("li $v0, 11");
					if (is_immed) {
						OUTPUT("li $a0, " << strs[2]);
					}
					else {
						load(strs[2], "$a0");
					}
					OUTPUT("syscall");
				}
			}
		}
		else if (strs[0] == "@scanf") {
			if (strs[1] == "INT") {
				OUTPUT("li $v0, 5");
			}
			else if (strs[1] == "CHAR") {
				OUTPUT("li $v0, 12");
			}
			OUTPUT("syscall");
			save(strs[2], "$v0");
		}
		else if (strs[0] == "@exit") {

		}
		else {
			name_handle(strs);
		}
	}
}

void set_data_str() {
	OUTPUT(".data");
	int len = str_set.size();
	for (int i = 0; i < len; i++) {
		OUTPUT("S_" << i << ": .asciiz \"" << str_set[i] << "\"");
	}
	OUTPUT(".text");
}

void tar_code() {
	set_data_str();
	read_medis_tar();
}

#endif