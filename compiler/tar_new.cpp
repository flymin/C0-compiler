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


using namespace std;

#ifdef NEW_TAR

#if 0
#define MIPS_LEFT cout
#define MIPS_RIGHT endl
#else
#define MIPS_LEFT tarfile
#define MIPS_RIGHT endl
#endif // DEBUG
#define MIPS_OUTPUT(x) MIPS_LEFT << x << MIPS_RIGHT

/*extern from main.cpp*/
extern ofstream tarfile;
extern fstream midfile;

/*本文件中的全局变量*/
//const int reg_count = 8;      // the number of registers
const int temp_max = 8;      // the number of registers
Sym* cur_func = NULL;
int temp_base_addr = 0;
int ptr = 0;
int cur_addr = 0;
int para_read_count = 0;	// 函数初始化部分计算para序号
STRINT_MAP offset_map;
STRINT_MAP global_addr_map;
REG_MAP reg_regmap;       // reg-recorder map，记录寄存器的分配情况
REG_MAP name_regmap;       // name-recorder map，记录变量的分配情况


vector<string> paras;
set<string> free_temp_set;	//空闲的临时寄存器池，依靠中间代码维护，中间代码中需要在指定位置产生@free动作

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

/* 标记为不使用这两个函数
void save(string varname, string reg) {
	int addr;
	Sym* var;
	if (is_temp(varname)) {
		addr = 4 * get_temp_no(varname) + temp_base_addr;
		MIPS_OUTPUT("sw " << reg << ", -" << addr << "($fp)");
	}
	else {
		var = findSym(cur_func, (char*)varname.data());
		if (var->global) { // is global variable
			MIPS_OUTPUT("sw " << reg << ", -" << global_addr_map[var->name] << "($gp)");
		}
		else {
			MIPS_OUTPUT("sw " << reg << ", -" << offset_map[var->name] << "($fp)");
		}
	}
}

void load(string varname, string reg) {
	int addr;
	Sym* var;
	if (is_temp(varname)) {
		addr = 4* get_temp_no(varname) + temp_base_addr;
		MIPS_OUTPUT("lw " << reg << ", -" << addr << "($fp)");
	}
	else {
		var = findSym(cur_func, (char*)varname.data());
		if (var->global) { // is global variable
			MIPS_OUTPUT("lw " << reg << ", -" << global_addr_map[var->name] << "($gp)");
		}
		else {
			MIPS_OUTPUT("lw " << reg << ", -" << offset_map[var->name] << "($fp)");
		}
	}
}
*/

bool has_name(string name){
	return (name_regmap.find(name) != name_regmap.end());
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
	for (int i = 0; i < 4; i++) {
		stringstream ss;
		ss << "$a" << i;
		string regname = ss.str();
		reg_regmap.insert(REG_MAP::value_type(regname, new Reg_recorder(regname)));
	}
}

// 找到引用计数中最小的一个寄存器进行分配
Reg_recorder* get_min_use_recorder(){
	int min_use_count = -1;
	Reg_recorder* rec_selected = NULL;
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		if (rec->regname[1] == 'a') {
			it++;
			continue;
		}
		if (rec->state != OCCUPIED &&
			(min_use_count == -1 || rec->use_count < min_use_count))
		{
			min_use_count = rec->use_count;
			rec_selected = rec;
		}
		it++;
	}
	return rec_selected;
}

template <class T>
bool set_has_ele(const set<T> &s, const T ele)
{
	return (s.find(ele) != s.end());
}

void erase_reg_regmap() {
	name_regmap.clear();
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
	Reg_recorder* rec = NULL;
	// 该变量已经分配寄存器
	if (has_name(name)){
		// cout << "HAS:" << name << endl;
		rec = name_regmap[name];
		rec->use_count = use_counter++;
		if (rec->state == INACTIVE && is_def) {// value may be modified
			rec->state = MODIFIED;
		}
		if (set_has_ele(free_temp_set, name)) {// @free
			rec->state = INACTIVE;
			//free_temp_set.erase(name);
			free_name = name;
		}
		return rec->regname;
	}
	// occupy $t
	int regno = -1;
	if (is_temp(name) &&
		(regno = get_temp_no(name)) < temp_max)
	{
		stringstream ss;
		ss << "$t" << regno;
		// cout << "========" << ss.str() << "========\n";
		rec = reg_regmap[ss.str()];
		rec->clear_and_init();
		rec->name = name; // "#?"
		rec->type = INT; // temp
		rec->global = false;
		rec->offset = temp_base_addr + get_temp_no(name) * 4;
		rec->state = is_def ? MODIFIED : INACTIVE;
	}
	// occupy $s
	/* 根据函数名、基本块名找到当前变量对应的s寄存器
	else if ((regno = get_regno(cur_func->name, cur_label, name))
		!= -1)
	{
		stringstream ss;
		ss << "$s" << regno;
		rec = reg_regmap[ss.str()];
		rec->clear_and_init();
		rec->name = name;
		rec->type = cur_func->get_var(name)->get_type();
		rec->global = false;
		rec->offset = offset_map[name];
		rec->state = OCCUPIED;
	}
	*/
	// select one not be occupied
	else
	{
		Sym* var = findSym(cur_func, (char*)name.data());
		rec = get_min_use_recorder();
		if (rec == NULL)
		{
			//error_debug("cannot find recorder~");
		}
		rec->clear_and_init();
		rec->name = name;
		rec->state = is_def ? MODIFIED : INACTIVE;
		if (is_temp(name)) // temp
		{
			rec->type = INT;
			rec->global = false;
			//rec->offset = temp_base_addr + (get_temp_no(name) - temp_max) * 4;
			rec->offset = temp_base_addr + get_temp_no(name) * 4;
		}
		else if (is_num(name)) {// const value
			rec->type = INT;
			rec->global = false;
			rec->offset = -1;
		}
		else if(var) {	// var, 变量中对const变量的引用在中间代码已经消除
			if (!var->global) // local var
			{
				rec->type = var->type;
				rec->global = false;
				rec->offset = offset_map[name];
			}
			else if (var->global) // global var
			{
				rec->type = var->type;
				rec->global = true;
				rec->offset = global_addr_map[name];
			}
		}
		else
		{
			//error_debug("unknown value type in tar");
		}
	}
	if (!is_def && rec->state != OCCUPIED)
	{
		rec->load();
	}
	name_regmap.insert(REG_MAP::value_type(name, rec));
	return rec->regname;
}

//判断是否为2的整数幂
bool is_power(int number) {		
	return (number & number - 1) == 0;
}

//返回log2的值，求指数
int log2(int number) {	
	int result = 0;
	while (number > 1)
	{
		number >>= 1;
		result++;
	}
	return result;
}

// 使用移位操作的乘除法运算输出
void output_shift_mul_div(string op, string tar, string str_cal, int num_cal) {
	bool nega = num_cal < 0;
	int pos_num_cal = nega ? -num_cal : num_cal;
	string usereg = get_reg(str_cal, false);
	string defreg = get_reg(tar, true);
	if (op == "DIV")
	{
		MIPS_OUTPUT("sra " << defreg << ", " << usereg << ", " << log2(pos_num_cal));
	}
	else if (op == "MUL")
	{
		MIPS_OUTPUT("sll " << defreg << ", " << usereg << ", " << log2(pos_num_cal));
	}
	if (nega)
	{
		MIPS_OUTPUT("sub " << defreg << ", $0, " << defreg);
	}
}

void assign_para_tar(Sym* para){
	assert(cur_addr >= 12);
	int addr = cur_addr + para_read_count * 4;
	para->paranum = addr;
	
	if (para_read_count < 4){
		string reg_name = "$a" + to_string(para_read_count);
		Reg_recorder *rec = reg_regmap[reg_name];
		rec->clear_and_init();
		rec->global = false;
		rec->offset = addr;
		rec->name = para->name;
		rec->state = MODIFIED;
		rec->type = para->type;
		rec->use_count = 0;
		name_regmap.insert(REG_MAP::value_type(para->name, rec));
	}
	para_read_count++;
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
	调用者负责清理现场，保存使用的寄存器，标记所有变量都在内存中
	调用结束之后不必回复现场，只处理返回值即可
*/
void call_tar(string funcname) {
	Sym* var = NULL;	//实参的结构指针
	Sym* func = NULL;	//被调函数的结构指针
	func = findFunc((char*)funcname.data());
	int temp_addr;
	temp_addr = 12;//ra,fp,sp
	//这里保存寄存器，并调整temp_addr
	list<string> reg_save_list;
	Reg_recorder::save_modi_regs(&reg_save_list);
	int store_count = reg_save_list.size();
	//保存到了当前（父级）函数的堆栈中，不需要调整堆栈空间
	//保存寄存器结束
	if (funcname != "main") {
		// store paras
		int len = func->paranum;
		int i;
		for (i = len-1; i >= 4; i--) {
			//addr用来参数被调函数中的offset
			//temp_addr应该指向被调函数栈中的参数开始区域
			int addr = temp_addr + i * 4;
			string paraname = paras.back();
			//paras.erase(paras.begin());
			paras.pop_back();
			MIPS_OUTPUT("sw " << get_reg(paraname, false) <<", -"<< addr << "($sp)");
		}
		for (; i >= 0; i--){
			string paraname = paras.back();
			paras.pop_back();
			MIPS_OUTPUT("move $a" << i << ", " << get_reg(paraname, false));
			REG_MAP::iterator it = name_regmap.end();
			it = find_if(name_regmap.begin(), name_regmap.end(), map_value_finder("$a"+to_string(i)));
			if (it != name_regmap.end()) {
				name_regmap.erase(it);
			}
		}
		// 这里需要清理变量到寄存器的映射
		Reg_recorder::clear_and_init_all();
		erase_reg_regmap();
		MIPS_OUTPUT("sw $ra, 0($sp)");
		MIPS_OUTPUT("sw $fp, -4($sp)");
		//MIPS_OUTPUT("sw $sp, -8($sp)");
		// refresh $fp
		MIPS_OUTPUT("add $fp, $sp, $0");
		MIPS_OUTPUT("addi $sp, $sp, -" << temp_addr + 4 * func->paranum + func->psize);
		// jump
		MIPS_OUTPUT("jal " << funcname << "_E");
		// load regs
		MIPS_OUTPUT("addi $sp, $sp, " << temp_addr + 4 * func->paranum + func->psize);
		MIPS_OUTPUT("lw $ra, 0($sp)");
		MIPS_OUTPUT("lw $fp, -4($sp)");
	}
	else {
		Reg_recorder::clear_and_init_all();
		// refresh $fp
		//MIPS_OUTPUT("add $sp, $fp, $gp");
		//global pointer有初始值，直接使用全局空间
		MIPS_OUTPUT("add $fp, $sp, $0");
		MIPS_OUTPUT("sw $ra, 0($sp)");
		MIPS_OUTPUT("sw $fp, -4($sp)");
		//MIPS_OUTPUT("sw $sp, 8($sp)");
		MIPS_OUTPUT("addi $sp, $sp, -" << temp_addr + 4 * func->paranum + func->psize);
		// jump
		MIPS_OUTPUT("jal " << funcname << "_E");
		MIPS_OUTPUT("li $v0, 10");		//程序出口调用
		MIPS_OUTPUT("syscall");
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
	MIPS_OUTPUT(funcname << "_E:");
	Reg_recorder::clear_and_init_all();
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
		mips << (is_immed2 ? "addi" : "add");
		is_cal = true;

	}
	else if (op == "SUB") {
		mips << (is_immed2 ? "addi" : "sub");
		if (is_immed2) immed2 = -immed2;   // turn negative
		is_cal = true;

	}
	else if (op == "MUL") {
		if (is_immed1 && is_power(immed1))
		{
			output_shift_mul_div(op, tar_str, cal_str2, immed1);
			return;
		}
		if (is_immed2 && is_power(immed2))
		{
			output_shift_mul_div(op, tar_str, cal_str1, immed2);
			return;
		}
		mips << "mul";
		is_cal = true;

	}
	else if (op == "DIV") {
		if (is_immed2 && is_power(immed2))
		{
			output_shift_mul_div(op, tar_str, cal_str1, immed2);
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
	if (is_immed2 && is_cal)
	{
		stringstream ss;
		ss << immed2;
		name2 = ss.str();
	}
	else
	{
		name2 = get_reg(cal_str2, false);
	}
	if (is_num(tar_str))
	{
		//error_debug("tar is number");
	}
	else
	{
		mips << " " << get_reg(tar_str, true) << ", " << name1 << ", " << name2;
	}
	MIPS_OUTPUT(mips.str());
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
		//MIPS_OUTPUT("li $t0, " << var);
		//if (!is_set) {
			//error_debug("array to a value");
		//}
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
		MIPS_OUTPUT(op << " " << reg << ", -" << offset + ele_offset << "(" << point_reg << ")");

	}
	else {
		MIPS_OUTPUT("sll $v0, "<< get_reg(off_str, false) <<", 2");  // offset *= 4
		MIPS_OUTPUT("sub $v0, " << point_reg << ", $v0");

		MIPS_OUTPUT("addi $v0, $v0, -" << offset);  // add array base
		MIPS_OUTPUT(op << " " << reg << ", 0($v0)");
	}
}

// 非动作指令，根据中间代码类型判断解析方式
void name_handle(vector<string> strs) {
	int len = strs.size();
	if (len <= 1) {
		//error_debug("too few strs");
	}
	else if (strs[1] == ":") {
		MIPS_OUTPUT(strs[0] << ":");    //[MIPS]标签，直接输出
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
			// [MIPS] li加载立即数赋值
			//MIPS_OUTPUT("li $s" << get_reg(strs[0], &conf_names) << ", " << strs[2]);
			MIPS_OUTPUT("li "<< get_reg(strs[0], true) <<", " << strs[2]);
		}
		else {
			// [MIPS] move 加载变量值
			MIPS_OUTPUT("move "<< get_reg(strs[0], true) << ", " << get_reg(strs[2], false));
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


void readline_tar() {
	string line;
	while (getline(midfile, line)) {
		MIPS_OUTPUT("   # " << line);
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
				//save(strs[1], "$v0");
				MIPS_OUTPUT("move " << get_reg(strs[1], true) << ", $v0");
			}

		}
		else if (strs[0] == "@ret") { // v寄存器赋值，跳转至ra OK
			if (strs.size() == 2) {
				if (is_num(strs[1])) {
					MIPS_OUTPUT("li $v0, " << strs[1]);
				}
				else {
					MIPS_OUTPUT("move $v0, " << get_reg(strs[1], false) );
				}
			}
			MIPS_OUTPUT("jr $ra");
			MIPS_OUTPUT("nop");

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
			//Reg_recorder::before_branch_jump();
			MIPS_OUTPUT(br_op << " " << name1 << ", " << name2 << ", " << strs[3]);
		}
		else if (strs[0] == "@bz")
		{
			//Reg_recorder::before_branch_jump();
			MIPS_OUTPUT("beq " << get_reg(strs[1], false) << ", $0, " << strs[2]);
			//MIPS_OUTPUT("nop");
		}
		else if (strs[0] == "@bgtz" || strs[0] == "@bgez" ||
			strs[0] == "@blez" || strs[0] == "@bltz")
		{
			//Reg_recorder::before_branch_jump();
			MIPS_OUTPUT(strs[0].substr(1) << " " << get_reg(strs[1], false) << ", " << strs[2]);
		}
		else if (strs[0] == "@j") {
			MIPS_OUTPUT("j " << strs[1]);
			MIPS_OUTPUT("nop");

		}
		else if (strs[0] == "@jal") {
			MIPS_OUTPUT("jal " << strs[1]);
			MIPS_OUTPUT("nop");

		}
		else if (strs[0] == "@printf") {
			if (strs[1] == "LINE") {
				MIPS_OUTPUT("li $a0, 10");
				MIPS_OUTPUT("li $v0, 11");
				MIPS_OUTPUT("syscall");
			}
			else {
				bool is_immed = is_num(strs[2]);
				if (strs[1] == "STRING") {
					MIPS_OUTPUT("li $v0, 4");
					MIPS_OUTPUT("la $a0, " << strs[2]);
					MIPS_OUTPUT("syscall");
				}
				else if (strs[1] == "INT") {
					MIPS_OUTPUT("li $v0, 1");
					if (is_immed) {
						MIPS_OUTPUT("li $a0, " << strs[2]);
					}
					else {
						MIPS_OUTPUT("move $a0, " << get_reg(strs[2], false));
					}
					MIPS_OUTPUT("syscall");

				}
				else if (strs[1] == "CHAR") {
					MIPS_OUTPUT("li $v0, 11");
					if (is_immed) {
						MIPS_OUTPUT("li $a0, " << strs[2]);
					}
					else {
						MIPS_OUTPUT("move $a0, " << get_reg(strs[2], false));
					}
					MIPS_OUTPUT("syscall");
				}
			}
		}
		else if (strs[0] == "@scanf") {
			if (strs[1] == "INT") {
				MIPS_OUTPUT("li $v0, 5");
			}
			else if (strs[1] == "CHAR") {
				MIPS_OUTPUT("li $v0, 12");
			}
			MIPS_OUTPUT("syscall");
			MIPS_OUTPUT("move " << get_reg(strs[2], true) << ", $v0");
		}
		else if (strs[0] == "@exit") {

		}
		else if (strs[0] == "@free") {
			/*if (!is_temp(strs[1]) || get_temp_no(strs[1]) >= temp_max)
			{
			cout << "FREE " << get_temp_no(strs[0]) << " " << temp_max << endl;
			}*/
			free_temp_set.insert(strs[1]);	// free temp 用来记录，真正的free操作在下一条中间代码解析完成之后
			continue;
		}
		else {
			name_handle(strs);
		}
		set<string>::iterator it = free_temp_set.begin();	// 释放free_temp
		while (it != free_temp_set.end()) {
			name_regmap.erase(*it);
			it++;
		}
		free_temp_set.clear();
	}
}

void set_data_str() {
	MIPS_OUTPUT(".data");
	int len = str_set.size();
	for (int i = 0; i < len; i++) {
		MIPS_OUTPUT("S_" << i << ": .asciiz \"" << str_set[i] << "\"");
	}
	MIPS_OUTPUT(".text");
}

void tar_code() {
	cout << "using new tar code generater" << endl;
	init_reg_map();
	set_data_str();
	readline_tar();
}

#endif