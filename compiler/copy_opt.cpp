#include <iostream>
#include <map>
#include <list>
#include <regex>
#include "var.h"
#include "tar.h"
#include "grammar.h"
#include "dag_opt.h"
#include "copy_opt.h"
#include "table.h"
#include "words.h"
#include "main.h"
#include <vector>
#include <sstream>

# define DEBUG 0
# if DEBUG
# define OUT_LEFT cout << "<==="
# define OUT_RIGHT "===>" << endl
# else
# define OUT_LEFT fout
# define OUT_RIGHT endl
# endif // DEBUG
# define OUTPUT(x) OUT_LEFT << x << OUT_RIGHT; line_count++
# define HAS_TEMP(tempname) temp_map.find(tempname) != temp_map.end()

using namespace std;

#ifdef OPT

int line_count = 0;			//统计中间代码行数，两次相同认为中间代码优化可终止
static ifstream fin;
static ofstream fout;

// temp <- temp
// var <- temp
// temp <- var
// var <- var

Sym* cur_func_ASS = NULL;
typedef map<int, Line*> LINE_MAP;
typedef map<string, Block_copy*> BLOCK_MAP;
typedef map<string, string> TEMP_MAP;

LINE_MAP line_map;		// int-Line map
BLOCK_MAP block_map;	// string-Block_copy map
vector<string> temp_storage;	// 临时变量的存储区
int new_temp_count = 0;			// 历史变量计数
TEMP_MAP temp_map;	// string-string map
int lineno = 0;		// 中间代码行号
bool skip = false;	//表示当前基本快是否到达，删除代码
stringstream code_storage;

// 判断line map中是否包含line
bool has_line(int line) {
	return (line_map.end() != line_map.find(line));
}

void save_used_to_line(Block_copy* useblock) {
	if (has_line(useblock->def_line_no)) {  // defined by "="
		useblock->last_used_line = lineno;  // seems not necessary
		Line* line = line_map[useblock->def_line_no];
		line->active = true;    // set line active
	}
}

// 判断block map中包含这个Block_copy
bool has_block(string name) {
	return (block_map.find(name) != block_map.end());
}

/* @REQUIRES:
*  @MODIFIES: block_map
*  @EFFECTS:
*  has_block(defname) => remove old block
*                     => remove all blocks whose source is defname
*  usename == "" => source = NULL
*  usename != "" => source = get_block(usename)
*/
string def(int line, string defname, string usename = "")
{
	Block_copy* defblock = NULL;
	if (has_block(defname)) {
		// disable sources pointing to defblock
		defblock = block_map[defname];
		BLOCK_MAP::iterator it = block_map.begin();
		while (it != block_map.end()) {
			if (it->second->source == defblock) {
				it->second->source = NULL;
			}
			it++;
		}
		defblock->def_line_no = line;  // refresh def_line_no
		if (has_line(defblock->last_used_line) && defblock->name[0] == '#') {
			line_map[defblock->last_used_line]->last_use_names
				.push_back(defblock->name);
			defblock->last_used_line = -1;
		}
	}
	else {
		// create defblock
		defblock = new Block_copy(line, defname);
		block_map[defname] = defblock;
	}
	if (usename == "") {// cal
		defblock->source = NULL;
	}
	else if (has_block(usename)) {   // assign
		//cout << usename << " ";
		defblock->source = block_map[usename];
		use(usename);
		return defblock->get_source()->name;
	}
	else {   // assign
		//cout << usename << " ";
		defblock->source = new Block_copy(-1, usename);
		block_map[usename] = defblock->source;
		return defblock->get_source()->name;
	}
	return "";
}

/* @REQUIRES:
* @MODIFIES: block_map
* @EFFECTS:
*  has_block(usename) => set source used
*                  => return source's name
*  !has_block(usename) => just return usename
*/
string use(string usename) {
	if (has_block(usename)) {
		Block_copy* useblock_source = block_map[usename]->get_source();
		// cout << "use:" << usename << endl;
		save_used_to_line(useblock_source);
		return useblock_source->name;	// 复制传播！将source的name作为这个use的name返回
	}
	else {
		return usename;
	}
}

// 将生成的中间代码暂存在code_storage buffer中
// code_storage buffer 是一个字符流，中间包括\n换行符
void store_medi(vector<string> code) {
	int len = code.size();
	// cout << code_storage.str() << endl;
	code_storage << code[0];
	// cout << code_storage.str() << endl;
	for (int i = 1; i < len; i++) {
		code_storage << " " << code[i];
	}
	code_storage << "\n";
	// cout << code_storage.str() << endl;
	lineno++;
}

// @exit/label/@func 划分blocks
void init_blocks() {
	new_temp_count = 0;			//根据每个基本块分配临时变量
	temp_storage.clear();
	LINE_MAP::iterator line_it = line_map.begin();
	while (line_it != line_map.end()) {
		line_it->second->last_use_names.clear();
		delete(line_it->second);
		line_it++;
	}
	line_map.clear();
	BLOCK_MAP::iterator block_it = block_map.begin();
	while (block_it != block_map.end()) {
		delete(block_it->second);
		block_it++;
	}
	block_map.clear();
	temp_map.clear();
	lineno = 0;
	skip = false;
}

// output medi中用来生成新的临时变量
string get_new_temp(string tempname) {
	if (temp_map.find(tempname) != temp_map.end()) {
		// has temp
		return temp_map[tempname];
	}
	else if (!temp_storage.empty()) {
		temp_map[tempname] = temp_storage.front();
		temp_storage.erase(temp_storage.begin());
		return temp_map[tempname];
	}
	else {
		int tempno = new_temp_count++;
		stringstream ss;
		ss << "#" << tempno;
		temp_map[tempname] = ss.str();
		return temp_map[tempname];
	}
}

// 通过产生@free工作来释放中间变量
void remove_temp(string tempname) {
	TEMP_MAP::iterator it = temp_map.find(tempname);
	if (it != temp_map.end()) {
		OUTPUT("@free " << it->second);
		temp_storage.push_back(it->second);
		temp_map.erase(it);
	}
}

void save_vars() {
	BLOCK_MAP::iterator it = block_map.begin();
	while (it != block_map.end()) {
		if (IS_VAR(it->first) && has_line(it->second->def_line_no)) {
			line_map[it->second->def_line_no]->active = true;
		}
		it++;
	}
}

void snatch_new_temp(string oldtemp, string newtemp) {
	TEMP_MAP::iterator it = temp_map.begin();
	while (it != temp_map.end()) {
		string cur_newtemp = it->second;
		if (cur_newtemp == newtemp) {
			temp_map.erase(it);
			break;
		}
		it++;
	}
	//cout << oldtemp << "\t" << newtemp << endl;
	temp_map.insert(TEMP_MAP::value_type(oldtemp, newtemp));
}

bool is_last_use(string oldname, int lineno) {
	vector<string>::iterator it = line_map[lineno]->last_use_names.begin();
	while (it != line_map[lineno]->last_use_names.end()) {
		if (*it == oldname) {// found
			return true;
		}
		it++;
	}
	return false;
}

template <class A, class B>
bool has_key(const map<A, B> &m, const A key) {
	return (m.find(key) != m.end());
}

void set_last_use_before_output(bool is_return) {
	BLOCK_MAP::iterator it = block_map.begin();
	while (it != block_map.end()) {
		if (((!is_return && IS_VAR(it->first)) || is_global_var(it->first))
			&& has_line(it->second->def_line_no))
		{
			line_map[it->second->def_line_no]->active = true;
		}
		else if (IS_TEMP(it->first) && has_line(it->second->last_used_line)) {
			// cout << it->second->last_used_line << endl;
			line_map[it->second->last_used_line]->last_use_names
				.push_back(it->first);
		}
		it++;
	}
}

// 输出一个基本块的中间代码
// 其中插入了last use temp的free动作
void output_medis(bool is_return = false)
{
	set_last_use_before_output(is_return);

	string line;
	int l = 0;
	// cout << code_storage.str() << endl;
	while (getline(code_storage, line)) {
		if (!has_line(l)) {
			OUTPUT(line);
		}
		else if (line_map[l]->active) {
			istringstream is(line);
			stringstream ss;
			string str;
			is >> str;
			TEMP_MAP cur_temp_map;
			string def_oldtemp = "";
			bool def_got_temp = true;
			if (IS_TEMP(str)) {
				def_oldtemp = str;
				if (has_key(temp_map, def_oldtemp)) {
					cur_temp_map.insert(TEMP_MAP::value_type(def_oldtemp, temp_map[def_oldtemp]));
					def_got_temp = true;
				}
				else {
					def_got_temp = false;
				}
			}
			// read line
			while (is >> str) {
				// got temp
				if (IS_TEMP(str) && !has_key(cur_temp_map, str)) {
					// insert into map
					cur_temp_map.insert(TEMP_MAP::value_type
					(str, get_new_temp(str)));
					// if 'def' = 'use'
					if (def_oldtemp == str) {
						def_got_temp = true;
					}
				}
			}
			// clear list use temp
			TEMP_MAP::iterator it = cur_temp_map.begin();
			while (it != cur_temp_map.end()) {
				string old_temp = it->first;
				string new_temp = it->second;
				// 'def' have not got temp and can got now
				if (!def_got_temp && is_last_use(old_temp, l)) {
					// change temp map
					snatch_new_temp(def_oldtemp, new_temp);
					// insert into cur temp map
					cur_temp_map.insert(TEMP_MAP::value_type(def_oldtemp, new_temp));
					def_got_temp = true;
				}
				else if (is_last_use(old_temp, l)) {
					// remove key 'old temp' from temp map
					remove_temp(old_temp);
				}
				it++;
			}
			if (!def_got_temp) {
				cur_temp_map.insert(TEMP_MAP::value_type(def_oldtemp, get_new_temp(def_oldtemp)));
			}
			// reset istringstream
			is.clear();
			is.str(line);
			is >> str;
			if (IS_TEMP(str)) {
				str = cur_temp_map[str];
			}
			ss << str;
			// read and output
			while (is >> str) {
				ss << " ";
				if (IS_TEMP(str)) {
					str = cur_temp_map[str];
				}
				ss << str;
			}
			OUTPUT(ss.str());
		}
		l++;
	}
	code_storage.clear();
}

bool outed = false;

// @REQUIRES: len(strs) == 5
// 针对运算指令进行优化，将可以计算的常数算出
void expre_opt(vector<string>* strs) {
	if (strs->size() != 5 || (*strs)[0][0] == '@') {
		return;
	}
	string tar = (*strs)[0];
	string op = (*strs)[3];
	string cal1 = (*strs)[2];
	string cal2 = (*strs)[4];
	string result = "";
	if (is_num(cal1) && is_num(cal2) && op != "ARRSET") {
		int num1, num2, result_value;
		//sscanf(cal1.c_str(), "%d", &num1);
		//sscanf(cal2.c_str(), "%d", &num2);
		num1 = atoi(cal1.c_str());
		num2 = atoi(cal2.c_str());
		if (op == "SUB") result_value = num1 - num2;
		else if (op == "ADD") result_value = num1 + num2;
		else if (op == "DIV") {
			if (num2 == 0) {
				//warning("may division by zero"); TODO
				result_value = 0;
			}
			else {
				result_value = num1 / num2;
			}
		}
		else if (op == "MUL") result_value = num1 * num2;
		else if (op == "GT") result_value = (num1 > num2);
		else if (op == "GE") result_value = (num1 >= num2);
		else if (op == "LT") result_value = (num1 < num2);
		else if (op == "LE") result_value = (num1 <= num2);
		else if (op == "EQ") result_value = (num1 == num2);
		else if (op == "NE") result_value = (num1 != num2);
		else cout << (string)("unknown op \'" + op + "\' in expression opt") << endl;
		stringstream ss;
		ss << result_value;
		result = ss.str();
	}
	else if (op == "ADD") {
		if (cal1 == "0") result = cal2;
		else if (cal2 == "0") result = cal1;
	}
	else if (op == "SUB") {// a = b - b
		if (cal1 == cal2) result = "0";
		else if (cal2 == "0") result = cal1;
	}
	else if (op == "MUL") {
		if (cal1 == "0" || cal2 == "0") result = "0";
		else if (cal1 == "1") result = cal2;
		else if (cal2 == "1") result = cal1;
	}
	else if (op == "DIV") {
		if (cal1 == "0") result = "0";
		else if (cal2 == "1") result = cal1;
		else if (cal1 == cal2) result = "1";
	}
	else if (op == "NE" && cal1 == cal2) {
		result = "0";
	}
	else if (op == "EQ" && cal1 == cal2) {
		result = "1";
	}
	if (result != "") {
		strs->clear();
		strs->push_back(tar);
		strs->push_back("=");
		strs->push_back(result);
		// if (!outed) cout << tar <<" = " << result << endl;
	}
}


list<string> call_stack;


void copy_read_medis() {
	string line;
	while (getline(fin, line)) {
		istringstream is(line);
		string str;
		vector<string> strs;
		while (is >> str) {
			strs.push_back(str);
		}

		expre_opt(&strs);
		if (strs[0] == "@var" || strs[0] == "@array" || strs[0] == "@para") {
			OUTPUT(line);
		}
		else if (strs[0] == "@func") {
			output_medis();
			init_blocks();			// 函数入口作为基本快开始
			cur_func_ASS = findFunc((char*)strs[1].data());
			OUTPUT(line);
		}
		else if (strs[0] == "@push" && !skip) {
			// use
			line_map[lineno] = new Line(true);
			strs[1] = use(strs[1]);
			call_stack.push_back(strs[1]);
			store_medi(strs);
		}
		else if (strs[0] == "@call" && !skip) {
			// use paras
			int len = findFunc((char*)strs[1].data())->paranum;
			for (int i = 0; i < len; i++) {
				use(call_stack.front());
				call_stack.pop_front();
			}
			// store line
			line_map[lineno] = new Line(true);
			BLOCK_MAP::iterator it = block_map.begin();
			while (it != block_map.end())
			{
				if (is_global_var(it->first))
				{
					save_used_to_line(it->second);
					def(lineno, it->first);    // may be modified in function called
				}
				it++;
			}
			store_medi(strs);
		}
		else if (strs[0] == "@get" && !skip) {
			// def
			line_map[lineno] = new Line(true);
			def(lineno, strs[1]);
			store_medi(strs);
		}
		else if (strs[0] == "@ret" && !skip) {
			// use | output | skip
			line_map[lineno] = new Line(true);
			if (strs.size() > 1)
			{
				strs[1] = use(strs[1]);
			}
			store_medi(strs);
			output_medis(true);
			skip = true;
		}
		else if (strs[0] == "@be" && !skip) {
			// use
			line_map[lineno] = new Line(true);
			strs[1] = use(strs[1]);
			strs[2] = use(strs[2]);
			store_medi(strs);
			save_vars();
		}
		else if (strs[0] == "@bz" && !skip) {
			if (is_num(strs[1]) && atoi(strs[1].c_str()))
			{
				continue;
			}
			// use
			line_map[lineno] = new Line(true);
			strs[1] = use(strs[1]);
			store_medi(strs);
			save_vars();
		}
		else if (strs[0] == "@j" && !skip) {
			// 调整了语法分析阶段for循环的语句顺序，这里删除死代码
			//regex e("(for_end)$");
			string temp_line;
			if (strs[1].find("for_end") != string::npos) {
				while (getline(fin, temp_line)) {
					istringstream temp_is(temp_line);
					string temp_str;
					vector<string> temp_strs;
					while (temp_is >> temp_str) {
						temp_strs.push_back(temp_str);
					}
					if (temp_strs.size() == 2 && temp_strs[1] == ":") {
						if (temp_strs[0] == strs[1]) {	//跳过
														//str = temp_str;
							strs.assign(temp_strs.begin(), temp_strs.end());
							line = temp_line;
							break;
						}
					}
				}
			}
			else {
				//@j与目标标签之间符合skip要求直接连带@j skip掉，这种情况也不用划分基本块
				//这个处理配合DAG的b-j转换可以达到删除死代码的效果
				streampos pos = fin.tellg();	//保存当前流指针位置
				while (getline(fin, temp_line)) {
					istringstream temp_is(temp_line);
					string temp_str;
					vector<string> temp_strs;
					while (temp_is >> temp_str) {
						temp_strs.push_back(temp_str);
					}
					if (temp_strs[0] == "@ret" || temp_strs[0] == "@exit" || temp_strs[0] == "@func") {
						// 恢复，继续
						fin.seekg(pos);
						break;
					}
					else if (temp_strs[0] == "@free") {}
					else if (temp_strs[1] == ":") {
						if (temp_strs[0] == strs[1]) {	//跳过
							//str = temp_str;
							strs.assign(temp_strs.begin(), temp_strs.end());
							line = temp_line;
							goto label;
						}
						else {
							//恢复，继续
							fin.seekg(pos);
							break;
						}
					}
				}
				// output | skip
				line_map[lineno] = new Line(true);
				output_medis();
				OUTPUT(line);
				skip = true;
			}
		}
		else if (strs[0] == "@jal" && !skip) {
			OUTPUT(line);
		}
		else if (strs[0] == "@printf" && !skip) {
			// use
			line_map[lineno] = new Line(true);
			if (strs[1] != "STRING" && strs[1] != "LINE") {
				strs[2] = use(strs[2]);
			}
			store_medi(strs);
		}
		else if (strs[0] == "@scanf" && !skip) {
			// def
			line_map[lineno] = new Line(true);
			def(lineno, strs[2]);
			store_medi(strs);
		}
		else if (strs[0] == "@exit") {
			output_medis(true);
			init_blocks();			// exit作为整个程序的开始
			OUTPUT(line);
		}
		else if (strs[0] == "@free") {}
		else if (strs.size() > 1 && strs[1] == ":") {
		label:
			// output | stop
			output_medis();
			init_blocks();			//标签作为基本快开始标志
			OUTPUT(line);
		}
		else if (strs.size() == 3 && !skip) {	// X = X代码
			// def | use
			line_map[lineno] = new Line(false);
			strs[2] = def(lineno, strs[0], strs[2]);
			store_medi(strs);
		}
		else if (strs.size() == 4 && !skip) {	// ARRSET
			strs[0] = use(strs[0]);
			strs[2] = use(strs[2]);
			strs[3] = use(strs[3]);
			line_map[lineno] = new Line(true);
			store_medi(strs);
		}
		else if (strs.size() == 5 && !skip) {	// 所有带有赋值的代码
			// def | use
			strs[2] = use(strs[2]);
			strs[4] = use(strs[4]);
			line_map[lineno] = new Line(false);
			def(lineno, strs[0]);
			store_medi(strs);
		}
		else if (!skip) {
			cout << line + " cal len not 3 or 5 in ass" << endl;
		}
	}
	outed = true;
}

string copy_main(string filename, int *lc)
{
	init_blocks();
	line_count = 0;
	fin.open(filename.c_str());
	string copy_filename = generate_filename("ASS");
	fout.open(copy_filename.c_str());
	copy_read_medis();
	fout.close();
	fin.close();
	*lc = line_count;
	return copy_filename;
}

#endif