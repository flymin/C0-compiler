#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <assert.h>
#include <algorithm>
#include "mid_code.h"
#include "var.h"
#include "grammar.h"
#include "const.h"
#include "table.h"
#include "words.h"
#include "tar.h"
#include "reg_recorder.h"
#include "live_var.h"
#include "dag_opt.h"
#include "main.h"


using namespace std;

#ifdef NEW_TAR

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
const int max_temp_reg = 8;      // the number of registers
Sym* cur_func = NULL;
int temp_base_addr = 0;
int ptr = 0;
int cur_addr = 0;
int para_read_count = 0;	// 函数初始化部分计算para序号
STRINT_MAP offset_map;
STRINT_MAP global_addr_map;
REG_MAP reg_regmap;       // reg-recorder map，记录寄存器的分配情况
REG_MAP var_regmap;       // name-recorder map，记录变量的分配情况

string cur_label = ""; // need not init
vector<string> paras;
set<string> free_temp_set;	//空闲的临时寄存器池，依靠中间代码维护，中间代码中需要在指定位置产生@free动作

/*功能函数和判断函数*/

template <typename T>
T get_element(string name, map<string, T> from_map) {
	typename map<string, T>::iterator it = from_map.find(name);
	if (it != from_map.end()) {
		return it->second;
	}
	return NULL;
}

bool has_name(string name) {
	return (var_regmap.find(name) != var_regmap.end());
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

/* 标记为不使用这两个函数
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
*/

/************
**初始化函数**
************/

void init_global_regs() {
	map<string, Data_node*> *vn_map = &((*(func_fblock_map[cur_func->name]))[cur_label]->actives);
	map<string, Data_node*>::iterator it = vn_map->begin();
	while (it != vn_map->end())
	{
		if (it->second->regno != -1)
		{
			get_reg(it->first, false);
		}
		it++;
	}
}

//初始化寄存器使用记录
void init_reg_map(){
	for (int i = 0; i < 8; i++){
		stringstream ss;
		ss << "$s" << i;
		string regname = ss.str();
		reg_regmap.insert(REG_MAP::value_type(regname, new Reg_recorder(regname)));
	}
	for (int i = 0; i < 10; i++){
		stringstream ss;
		ss << "$t" << i;
		string regname = ss.str();
		reg_regmap.insert(REG_MAP::value_type(regname, new Reg_recorder(regname)));
	}
	/* a寄存器只用来传参，不作为变量寄存器使用
	for (int i = 0; i < 4; i++) {
		stringstream ss;
		ss << "$a" << i;
		string regname = ss.str();
		reg_regmap.insert(REG_MAP::value_type(regname, new Reg_recorder(regname)));
	}
	*/
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

void init_var(Sym* var_item) {
	if (var_item->kind == ARRAY) {
		if (cur_func == NULL) {
			var_item->global = true;
			global_addr_map[var_item->name] = ptr;
		}
		else {
			var_item->global = false;
			offset_map[var_item->name] = ptr;
		}
		ptr += 4 * var_item->paranum;
	}
	else {
		if (cur_func == NULL) {
			var_item->global = true;
			global_addr_map[var_item->name] = ptr;
		}
		else {
			var_item->global = false;
			offset_map[var_item->name] = ptr;
		}
		ptr += 4;
	}
	temp_base_addr = ptr;
}

/**************
**代码生成函数**
**************/

void assign_para_tar(Sym* para) {
	assert(cur_addr >= 12);
	int addr = cur_addr + para_read_count * 4;
	para->paranum = addr;
	if (para_read_count < 4){
		OUTPUT("move, " << get_reg(para->name, true) << ", $a" << para_read_count);
	}
	else {
		OUTPUT("lw " << get_reg(para->name, true) << ", -" << addr << "($fp)");
	}
	para_read_count++;
}

// 找到引用计数中最小的一个寄存器进行分配
Reg_recorder* get_min_use_reg(){
	int min_use_count = -1;
	Reg_recorder* rec_selected = NULL;
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end()) {
		Reg_recorder* reg_record = it->second;
		if (reg_record->state != OCCUPIED &&
			(min_use_count == -1 || reg_record->use_count < min_use_count))
		{
			min_use_count = reg_record->use_count;
			rec_selected = reg_record;
		}
		it++;
	}
	return rec_selected;
}

template <class T>
bool set_has_ele(const set<T> &s, const T ele) {
	return (s.find(ele) != s.end());
}

string free_name = "";

/* 获取变量对应的寄存器
 * 要求通过到达-定义分析指明调用是否为def
 */
string get_reg(string name, bool is_def)
{
	if (name == "0") {	//0使用$0，避免load指令
		return "$0";
	}
	if (name == "$at"){
		return "$at";
	}
	Reg_recorder* reg_record = NULL;
	// 该变量已经分配寄存器
	if (has_name(name)){
		// cout << "HAS:" << name << endl;
		reg_record = var_regmap[name];
		reg_record->name = name;
		reg_record->use_count = use_counter++;
		if (reg_record->state == INACTIVE && is_def) {// value may be modified
			reg_record->state = MODIFIED;
		}
		if (set_has_ele(free_temp_set, name)) {// @free
			reg_record->state = INACTIVE;
			//free_temp_set.erase(name);
			free_name = name;
		}
		return reg_record->regname;
	}
	// occupy $t
	int regno = -1;
	if (is_temp(name) &&
		(regno = get_temp_no(name)) < max_temp_reg) {
		stringstream ss;
		ss << "$t" << regno;
		reg_record = reg_regmap[ss.str()];
		reg_record->clear_and_init();
		reg_record->name = name; // "#?"
		reg_record->type = INT; // temp
		reg_record->global = false;
		reg_record->offset = temp_base_addr + get_temp_no(name) * 4;
		reg_record->state = OCCUPIED;
	}
	// occupy $s
	/* 根据函数名、基本块名找到当前变量对应的s寄存器 */
	else if ((regno = get_regno(cur_func->name, cur_label, name))
		!= -1) {
		stringstream ss;
		ss << "$s" << regno;
		reg_record = reg_regmap[ss.str()];
		reg_record->clear_and_init();
		reg_record->name = name;
		reg_record->type = findSym(cur_func, (char*)name.data())->type;
		reg_record->global = false;
		reg_record->offset = offset_map[name];
		reg_record->state = OCCUPIED;
	}
	// select one not be occupied
	else {
		Sym* var = findSym(cur_func, (char*)name.data());
		reg_record = get_min_use_reg();
		if (reg_record == NULL) {
			//error_debug("cannot find recorder~");
		}
		reg_record->clear_and_init();
		reg_record->name = name;
		reg_record->state = is_def ? MODIFIED : INACTIVE;
		if (is_temp(name)) {// temp
			reg_record->type = INT;
			reg_record->global = false;
			//reg_record->offset = temp_base_addr + (get_temp_no(name) - max_temp_reg) * 4;
			reg_record->offset = temp_base_addr + get_temp_no(name) * 4;
		}
		else if (is_num(name)) {// const value
			reg_record->type = INT;
			reg_record->global = false;
			reg_record->offset = -1;
		}
		else if(var) {	// var, 变量中对const变量的引用在中间代码已经消除
			if (!var->global) {// local var
				reg_record->type = var->type;
				reg_record->global = false;
				reg_record->offset = offset_map[name];
			}
			else if (var->global) {// global var
				reg_record->type = var->type;
				reg_record->global = true;
				reg_record->offset = global_addr_map[name];
			}
		}
		else {
			//error_debug("unknown value type in tar");
		}
	}
	if (!is_def && reg_record->state != OCCUPIED)
	{
		reg_record->load();
	}
	var_regmap.insert(REG_MAP::value_type(name, reg_record));
	return reg_record->regname;
}

//判断是否为2的整数幂
bool is_power(int number) {		
	return (number & number - 1) == 0;
}

//返回log2的值，求指数
int log2(int number) {	
	int result = 0;
	while (number > 1) {
		number >>= 1;
		result++;
	}
	return result;
}

// 使用移位操作的乘除法运算输出
void output_shift(string op, string tar, string str_cal, int num_cal) {
	bool neg = num_cal < 0;
	int pos_num_cal = neg ? -num_cal : num_cal;
	string usereg = get_reg(str_cal, false);
	string defreg = get_reg(tar, true);
	if (op == "DIV") {
		OUTPUT("sra " << defreg << ", " << usereg << ", " << log2(pos_num_cal));
	}
	else if (op == "MUL") {
		OUTPUT("sll " << defreg << ", " << usereg << ", " << log2(pos_num_cal));
	}
	if (neg) {
		OUTPUT("sub " << defreg << ", $0, " << defreg);
	}
}

/*	push参数，清空paras，保存现场（sp、fp、ra、s）
	调用者负责清理现场，保存使用的寄存器，标记所有变量都在内存中
	调用结束之后恢复现场，处理返回值
*/
void call_tar(string funcname) {
	Sym* var = NULL;	//实参的结构指针
	Sym* func = NULL;	//被调函数的结构指针
	func = findFunc((char*)funcname.data());
	int temp_addr;
	temp_addr = 12;//ra,fp,sp
	if (funcname != "main") {
		// store paras
		int len = func->paranum;
		int i;

		//开始保存寄存器
		list<string> reg_save_list;
		list<string> var_save_list;
		//map<string, Reg_recorder*> temp;
		//temp = var_regmap;
		Reg_recorder::record_occu_regs(&reg_save_list, &var_save_list);
		int store_count = 0;
		int stack_offset = (reg_save_list.size() + store_count) * 4;
		Reg_recorder::save_occu_regs(&reg_save_list, store_count * 4);
		OUTPUT("addi $sp, $sp, -" << stack_offset);
		//保存寄存器结束

		for (i = len-1; i >= 4; i--) {
			//addr用来参数被调函数中的offset
			//temp_addr应该指向被调函数栈中的参数开始区域
			int addr = temp_addr + i * 4;
			string paraname = paras.back();
			//paras.erase(paras.begin());
			paras.pop_back();
			OUTPUT("sw " << get_reg(paraname, false) <<", -"<< addr << "($sp)");
		}
		for (; i >= 0; i--){
			string paraname = paras.back();
			paras.pop_back();
			OUTPUT("move $a" << i << ", " << get_reg(paraname, false));
			//REG_MAP::iterator it = var_regmap.end();
		}
		// 这里需要清理变量到寄存器的映射
		Reg_recorder::clear_and_init_all();		//only use $fp and $gp, no $sp
		OUTPUT("sw $ra, 0($sp)");
		OUTPUT("sw $fp, -4($sp)");
		//Reg_recorder::save_global_modi_regs();
		


		// refresh $fp
		OUTPUT("add $fp, $sp, $0");
		OUTPUT("addi $sp, $sp, -" << temp_addr + 4 * func->paranum + func->psize);
		// jump
		OUTPUT("jal " << funcname << "_E");
		// load regs

		OUTPUT("addi $sp, $sp, " << temp_addr + 4 * func->paranum + func->psize);
		OUTPUT("lw $ra, 0($sp)");
		OUTPUT("lw $fp, -4($sp)");
		OUTPUT("addi $sp, $sp, " << stack_offset);
		OUTPUT("# BEGIN");
		init_global_regs(); // [fix]
		Reg_recorder::load_occu_regs(&reg_save_list, store_count * 4);
		while (!var_save_list.empty()) {
			get_reg(var_save_list.front(), true);
			var_save_list.pop_front();
		}
		//var_regmap = temp;
		OUTPUT("#END");
	}
	else {
		//global pointer有初始值，直接使用全局空间
		OUTPUT("add $fp, $sp, $0");
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



//计算指令
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
		if (is_immed1 && is_power(immed1))
		{
			output_shift(op, tar_str, cal_str2, immed1);
			return;
		}
		if (is_immed2 && is_power(immed2))
		{
			output_shift(op, tar_str, cal_str1, immed2);
			return;
		}
		mips << "mul";
		is_cal = true;

	}
	else if (op == "DIV") {
		if (is_immed2 && is_power(immed2)) {
			output_shift(op, tar_str, cal_str1, immed2);
			return;
		}
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

	string name1 = get_reg(cal_str1, false);
	string name2 = "";
	if (is_immed2 && is_cal) {
		stringstream ss;
		ss << immed2;
		name2 = ss.str();
	}
	else {
		name2 = get_reg(cal_str2, false);
	}
	if (is_num(tar_str)) {
		//error_debug("tar is number");
	}
	else {
		mips << " " << get_reg(tar_str, true) << ", " << name1 << ", " << name2;
	}
	OUTPUT(mips.str());
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
	reg = get_reg(var, !is_set);
	if (value_is_immed && !is_set) {
		//OUTPUT("li $t0, " << var);
		//if (!is_set) {
			//error_debug("array to a value");
		//}
	}

	// get op
	string op;
	op = is_set ? "sw" : "lw";

	int offset;
	string point_reg;
	STRINT_MAP::iterator it = offset_map.find(arr_str);
	if (it != offset_map.end()) {
		offset = it->second;
		point_reg = "$fp";
		if (offset_is_immed) {
			int ele_offset;
			sscanf_s(off_str.c_str(), "%d", &ele_offset);
			ele_offset *= 4;
			OUTPUT(op << " " << reg << ", -" << offset + ele_offset << "(" << point_reg << ")");

		}
		else {
			OUTPUT("sll $v0, " << get_reg(off_str, false) << ", 2");  // offset *= 4
			OUTPUT("sub $v0, " << point_reg << ", $v0");

			OUTPUT("addi $v0, $v0, -" << offset);  // add array base
			OUTPUT(op << " " << reg << ", 0($v0)");
		}
	}
	else {
		it = global_addr_map.find(arr_str);
		if (it != global_addr_map.end()) {
			offset = it->second;
			point_reg = "$gp";
			if (offset_is_immed) {
				int ele_offset;
				sscanf_s(off_str.c_str(), "%d", &ele_offset);
				ele_offset *= 4;
				OUTPUT(op << " " << reg << ", " << offset + ele_offset << "(" << point_reg << ")");

			}
			else {
				OUTPUT("sll $v0, " << get_reg(off_str, false) << ", 2");  // offset *= 4
				OUTPUT("addu $v0, " << point_reg << ", $v0");

				OUTPUT("addi $v0, $v0, " << offset);  // add array base
				OUTPUT(op << " " << reg << ", 0($v0)");
			}
		}
		else {
			//error_debug("cannot found array");
		}
	}
}

// 非动作指令，根据中间代码类型判断解析方式
void name_handle(vector<string> strs) {
	int len = strs.size();
	if (len <= 1) {
		//error_debug("too few strs");
	}
	else if (strs[1] == ":") {
		cur_label = strs[0];
		Reg_recorder::before_label();
		init_global_regs();
		OUTPUT(strs[0] << ":");    // MIPS标签，直接输出
	}
	else if (strs[1] == "ARRSET") {		//数组赋值语句
		array_tar(strs[0], strs[2], strs[3], true);
	}
	else if (strs[1] != "=") {			//其他语句都有等号
		//error_debug("without equal, " + strs[0]);
	}
	else if (len == 3) {  // 长度为3的只有赋值语句
		if (is_num(strs[0])) {
			//error_debug("assign to number");
		}
		if (is_num(strs[2])) {
			// MIPS li加载立即数赋值
			//OUTPUT("li $s" << get_reg(strs[0], &conf_names) << ", " << strs[2]);
			OUTPUT("li "<< get_reg(strs[0], true) <<", " << strs[2]);
		}
		else {
			// MIPS move 加载变量值
			OUTPUT("move "<< get_reg(strs[0], true) << ", " << get_reg(strs[2], false));
			//load(strs[2], "$t0");
			//save(strs[0], "$t0");
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
		else if (strs[0] == "@func") { // 初始化函数信息，生成标签
			init_func(strs[1]);

		}
		else if (strs[0] == "@label") {
			cur_label = strs[1];
			//init_global_regs();
		}
		else if (strs[0] == "@push") { // 将参数保存至vector中，不直接存储
			paras.push_back(strs[1]);

		}
		else if (strs[0] == "@call") {
			call_tar(strs[1]);

		}
		else if (strs[0] == "@get") { // 保存v寄存器的值
			if (is_num(strs[1])) {
				//error_debug("num get");
			}
			else {
				//save(strs[1], "$v0");
				OUTPUT("move " << get_reg(strs[1], true) << ", $v0");
			}

		}
		else if (strs[0] == "@ret") { // v寄存器赋值，跳转至ra
			if (strs.size() == 2) {
				if (is_num(strs[1])) {
					OUTPUT("li $v0, " << strs[1]);
				}
				else {
					OUTPUT("move $v0, " << get_reg(strs[1], false) );
				}
			}
			Reg_recorder::before_return();
			OUTPUT("jr $ra");

		}
		else if (strs[0] == "@be" || strs[0] == "@bne")
		{
			string br_op = "";
			string name1 = "";
			string name2 = "";
			if (strs[0] == "@be") br_op = "beq";
			else if (strs[0] == "@bne") br_op = "bne";
			if (!is_num(strs[2]))
			{
				name1 = get_reg(strs[1], false);
				name2 = get_reg(strs[2], false);
			}
			else if (strs[2] == "0")
			{
				name1 = get_reg(strs[1], false);
				name2 = "$0";
			}
			else
			{
				name1 = get_reg(strs[1], false);
				name2 = strs[2];
			}
			Reg_recorder::before_branch_jump();
			OUTPUT(br_op << " " << name1 << ", " << name2 << ", " << strs[3]);
		}
		else if (strs[0] == "@bz")
		{
			Reg_recorder::before_branch_jump();
			OUTPUT("beq " << get_reg(strs[1], false) << ", $0, " << strs[2]);
		}
		/*else if (strs[0] == "@bgtz" || strs[0] == "@bgez" ||
			strs[0] == "@blez" || strs[0] == "@bltz")
		{
			Reg_recorder::before_branch_jump();
			OUTPUT(strs[0].substr(1) << " " << get_reg(strs[1], false) << ", " << strs[2]);
		}*/
		else if (strs[0] == "@j") {
			Reg_recorder::before_branch_jump();
			OUTPUT("j " << strs[1]);

		}
		else if (strs[0] == "@jal") {
			OUTPUT("jal " << strs[1]);

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
						OUTPUT("move $a0, " << get_reg(strs[2], false));
					}
					OUTPUT("syscall");

				}
				else if (strs[1] == "CHAR") {
					OUTPUT("li $v0, 11");
					if (is_immed) {
						OUTPUT("li $a0, " << strs[2]);
					}
					else {
						OUTPUT("move $a0, " << get_reg(strs[2], false));
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
			OUTPUT("move " << get_reg(strs[2], true) << ", $v0");
		}
		else if (strs[0] == "@exit") {

		}
		else if (strs[0] == "@free") {
			// free temp 用来记录，真正的free操作在下一条中间代码解析完成之后
			free_temp_set.insert(strs[1]);
			continue;
		}
		else {
			name_handle(strs);
		}
		set<string>::iterator it = free_temp_set.begin();	// 释放free_temp
		while (it != free_temp_set.end()) {
			var_regmap.erase(*it);
			it++;
		}
		free_temp_set.clear();
	}
}

void setup_data() {
	OUTPUT(".data");
	int len = str_set.size();
	for (int i = 0; i < len; i++) {
		OUTPUT("S_" << i << ": .asciiz \"" << str_set[i] << "\"");
	}
	OUTPUT(".text");
}

void tar_code() {
	cout << "using new tar code generater" << endl;
	init_reg_map();
	setup_data();
	read_medis_tar();
}

#endif