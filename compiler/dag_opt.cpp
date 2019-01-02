#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <map>
#include "var.h"
#include "dag_opt.h"
#include "words.h"
#include "grammar.h"
#include "const.h"
#include "table.h"
#define IS_VAR(name) (name[0] == '_' || (name[0] >= 'a' && name[0] <= 'z') || \
							(name[0] >= 'A' && name[0] <= 'Z'))
#define IS_NUM(name) ((name[0] >= '0' && name[0] <= '9') || name[0] == '-')
// for now we suppose all var in symtable
#define GET_NEW_NAME(name) (get_name(node_map[name]))
# define DEBUG 0
# if DEBUG
# define MIPS_LEFT cout << "<==="
# define MIPS_RIGHT "===>" << endl
# else
# define MIPS_LEFT fout
# define MIPS_RIGHT endl
# endif // DEBUG
# define MIPS_OUTPUT(x) MIPS_LEFT << x << MIPS_RIGHT

using namespace std;

NODE_MAP node_map;			//����-�ڵ��
vector<Node*> nodes;		//���нڵ��б�
int temp_count = 0;
Sym* cur_func_DAG = NULL;
ifstream fin;
ofstream fout;

Node::Node(string value)
{
	this->no = -1;
	this->content = value;
	this->is_leaf = true;
	this->lptr = NULL;
	this->rptr = NULL;
	this->is_certain = true;//Ҷ�ڵ�Ϊcertain���ݹ���ֹ
	this->name = "#-1";
}

Node::Node(string op, Node* lptr, Node* rptr)
{
	this->no = -1;
	this->content = op;
	this->is_leaf = false;
	this->lptr = lptr;
	this->rptr = rptr;
	this->is_certain = false;
	this->name = "#-1";
}

void Node::make_certain()
{
	if (this->is_certain)
	{
		return;
	}
	this->lptr->make_certain();
	this->rptr->make_certain();
	string var1 = get_name(this->lptr);
	string var2 = get_name(this->rptr);
	MIPS_OUTPUT(get_name(this) << " = " << var1 <<
		" " << this->content << " " << var2);
	this->is_certain = true;
}

// scanf(continue) | get(continue)
void remove_var(string name)
{
	if (has_node(name))
	{
		NODE_MAP::iterator it = node_map.find(name);    // get iterator
		record_code(it);    // temp <- var, erase it
	}
}

// assign: to record vars' value, for other blocks' using
// remove: values of vars may be modified
void init_DAG()
{
	refresh_vars();
	node_map.clear();
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		delete(nodes[i]);
	}
	nodes.clear();
	temp_count = 0;
}

// finish block
NODE_MAP::iterator export_code(NODE_MAP::iterator it) {
	set_name(it->second);
	it->second->make_certain();
	if (it->first != get_name(it->second)) {  // var = temp����var���и�ֵ
		MIPS_OUTPUT(it->first << " = " << get_name(it->second));
		node_map.erase(it++); // why erase?
							  // need not to set name cause it's the end of the block
	}
	else {
		it++;
	}
	return it;
}

// continue block
NODE_MAP::iterator record_code(NODE_MAP::iterator it) {
	Node* node = it->second;
	string name = it->first;
	node_map.erase(it++);
	if (name == get_name(node))      // must be certain
	{

		set_name(node); // refresh node's name
	}
	return it;
}

bool is_global_var(string name) {
	Sym *var = findSym((char *)name.data());
	if (!var) {
		return false;
	}
	else {
		return var->global;
	}
}

// call(continue) | return
void record_global_vars(){
	NODE_MAP::iterator it = node_map.begin();
	while (it != node_map.end()){
		if (is_global_var(it->first)){
			it = export_code(it);
		}
		else{
			it++;
		}
	}
}

void refresh_global_vars(){
	NODE_MAP::iterator it = node_map.begin();
	while (it != node_map.end()){
		if (is_global_var(it->first)){
			it = export_code(it);
		}
		else{
			it++;
		}
	}
}

// branch | jump | label
// var <- temp
void refresh_vars(){
	NODE_MAP::iterator it = node_map.begin();
	while (it != node_map.end()){
		if (IS_VAR(it->first)){
			it = export_code(it);
		}
		else{
			it++;
		}
	}
}

// �жϱ������Ƿ��ж�Ӧ�Ľڵ�
bool has_node(string name)
{
	NODE_MAP::iterator it = node_map.find(name);
	return (it != node_map.end());
}

void add_to_map(string name, Node* nodeptr)
{
	node_map[name] = nodeptr;
}

/* @REQUIRES: len(name) > 0 && nodeptr != NULL
 * @MODIFIES: node_map
 * @EFFECTS:
 *  has_node => record_code
 *  node_map[name] = nodeptr
 */
void set_node(string name, Node* nodeptr, bool change_name = true) {
	if (has_node(name)) {
		record_code(node_map.find(name));
	}
	node_map[name] = nodeptr;   // change value in map
	if (true) {
		set_name(nodeptr);  // set name
	}
}

/* @REQUIRES: len(name) > 0
 * @MODIFIES: node_map, nodes
 * @EFFECTS:
 *  has_node => return node_map[name]
 *  !has_node => return new node of which the content is name
 */
Node* get_node(string name)
{
	if (!has_node(name))
	{
		Node* node = new Node(name);    // create node
		nodes.push_back(node);  // add to node list
		set_node(name, node);
	}
	return node_map[name];
}

/* @REQUIRES: node != NULL
 * @MODIFIES: node
 * @EFFECTS:
 *  is_num => name = content
 *  \exists key in node_map : is_var(key) && value is node => name = key
 *  \except => name = "#" + id (could be #-1)
 */
void set_name(Node* node)
{
	if (IS_NUM(node->content))
	{
		node->name = node->content;
		return;
	}
	string content = node->content;
	// points to it self
	if (IS_VAR(content) && has_node(content) && node_map[content] == node)
	{
		node->name = content;
		return;
	}
	bool has_name = (node->name[0] != '#');
	if (has_name)   // already has name
	{
		NODE_MAP::iterator it = node_map.begin();
		while (it != node_map.end()) {
			if (IS_VAR(it->first) && it->second == node)
			{
				MIPS_OUTPUT(it->first << " = " << node->name);
				node->content = it->first;
				node->name = it->first;
				return;
			}
			it++;
		}
	}
	else if (!node->is_certain)
	{
		NODE_MAP::iterator it = node_map.begin();
		while (it != node_map.end()) {
			if (IS_VAR(it->first) && it->second == node)
			{
				node->name = it->first;
				node->make_certain();
				node->content = it->first;
				return;
			}
			it++;
		}
	}

	stringstream ss;
	if (has_name)
	{
		node->no = temp_count++;
		ss << "#" << node->no;
		MIPS_OUTPUT(ss.str() << " = " << node->name);
	}
	else
	{
		ss << "#" << node->no;
	}
	node->name = ss.str();
}

/* @REQUIRES: node != NULL
 * @MODIFIES: node
 * @EFFECTS:
 *  name == "#-1" => no = temp_count++
 *                => name = "#" + no
 *  return name
 */
string get_name(Node* node)
{
	if (node->name == "#-1")
	{
		node->no = temp_count++;
		stringstream ss;
		ss << "#" << node->no;
		node->name = ss.str();
	}
	return node->name;
}


// XX = XX op XX
// XX = XX
void build_DAG(vector<string> code)
{
	if (code.size() == 3)   // assign
	{
		if (IS_VAR(code[0])) refresh_vars();
		set_node(code[0], get_node(code[2]), false);
	}
	else if (code.size() == 4 && code[1] == "ARRSET")
	{
		if (has_node(code[0]))
		{
			Node* node = get_node(code[0]);
			node_map.erase(node_map.find(code[0]));
			for (unsigned int i = 0; i < nodes.size(); i++)
			{
				if (!nodes[i]->is_leaf && nodes[i]->lptr == node)
				{
					set_name(nodes[i]);
					nodes[i]->make_certain();
				}
			}
		}

		MIPS_OUTPUT(code[0] << " ARRSET " << use_new_name(code[2]) <<
			" " << use_new_name(code[3]));
	}
	else if (code.size() == 5)
	{
		if (IS_VAR(code[0])) refresh_vars();
		string op = code[3];
		Node* node1 = get_node(code[2]);
		Node* node2 = get_node(code[4]);
		bool could_reverse = (op == "ADD" || op == "MUL" || op == "BE" || op == "EQ");
		int len = nodes.size();
		for (int i = 0; i < len; i++)   // all nodes
		{
			Node* node = nodes[i];
			if (node->content != op)    // not the same op
			{
				continue;
			}
			if ((node->lptr == node1 && node->rptr == node2) ||
				(could_reverse && node->lptr == node2 && node->rptr == node1))  // the same vars
			{
				set_node(code[0], node);    // found
				return;
			}
		}
		// not found, create one
		Node* node = new Node(op, node1, node2);
		nodes.push_back(node);
		set_node(code[0], node);
	}
	else
	{
		for (unsigned int i = 0; i<code.size(); i++) cout << code[i] << " ";
		//error_debug("cal len not 3 or 5 in dag.add_and_output");
	}
}

string use_new_name(string name) {
	if (!has_node(name)) {
		if (IS_VAR(name) || IS_NUM(name)) {
			return name;
		}
		else {
			return get_name(get_node(name));
		}
	}
	node_map[name]->make_certain();
	return GET_NEW_NAME(name);
}

void read_medis()
{
	string raw_line;
	while (getline(fin, raw_line)) {
		string line = "";
		int len = raw_line.length();
		//for (int i = 0; i < len; i++){
			//if (raw_line[i] == '#'){
			//	line += '$';
			//}
			//else{
			//	line += raw_line[i];
			//}
		//}
		line = raw_line;
		istringstream is(line);
		string str;
		vector<string> strs;
		while (is >> str){
			strs.push_back(str);
		}
		if (strs[0] == "@var" || strs[0] == "@array")
		{
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@para")
		{
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@func")
		{
			cur_func_DAG = findFunc((char *)strs[1].data());
			assert(cur_func_DAG != NULL);
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@push")
		{
			MIPS_OUTPUT("@push " << use_new_name(strs[1]));
		}
		else if (strs[0] == "@call")
		{
			record_global_vars();
			refresh_global_vars();
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@get")
		{
			if (IS_VAR(strs[1])) refresh_vars();
			remove_var(strs[1]);    // var <- function return value
			string name = use_new_name(strs[1]);
			MIPS_OUTPUT("@get " << name);
		}
		else if (strs[0] == "@ret"){	// return �����Ϊ������ĳ��ڣ�ˢ�»�����
			if (strs.size() == 1){
				refresh_global_vars();
				init_DAG();
				MIPS_OUTPUT(line);
			}
			else
			{
				string var1 = use_new_name(strs[1]);
				refresh_global_vars();
				init_DAG();
				MIPS_OUTPUT("@ret " << var1);
			}
		}
		else if (strs[0] == "@be")
		{
			if (has_node(strs[1]) && has_node(strs[2]) &&
				GET_NEW_NAME(strs[1]) == GET_NEW_NAME(strs[2])) { // have same node
				// ��ͬ�ڵ��ǰ����ʹ����������תָ��
				refresh_vars();
				MIPS_OUTPUT("@j " << strs[3]);
			}
			else {
				string var1 = use_new_name(strs[1]);
				string var2 = use_new_name(strs[2]);
				refresh_vars();
				MIPS_OUTPUT("@be " << var1 << " " << var2 << " " << strs[3]);
			}
		}
		else if (strs[0] == "@bz")
		{
			if (has_node(strs[1]) && GET_NEW_NAME(strs[1]) == "0")
			{
				refresh_vars();
				MIPS_OUTPUT("@j " << strs[2]);
			}
			else
			{
				string var1 = use_new_name(strs[1]);
				refresh_vars();
				MIPS_OUTPUT("@bz " << var1 << " " << strs[2]);
			}

		}
		else if (strs[0] == "@j")
		{
			refresh_vars();
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@jal")
		{
			refresh_vars();
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@printf")
		{
			if (strs[1] == "STRING" || strs[1] == "LINE")
			{
				MIPS_OUTPUT(line);
			}
			else
			{
				MIPS_OUTPUT("@printf " << strs[1] << " " << use_new_name(strs[2]));
			}
		}
		else if (strs[0] == "@scanf")
		{
			refresh_vars();
			remove_var(strs[2]);
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@exit")
		{
			MIPS_OUTPUT(line);
		}
		else if (strs[0] == "@free") {}
		else {
			// �ڱ�ǩ�����ֻ�����
			if (strs[1] == ":")
			{
				init_DAG();
				MIPS_OUTPUT(line);
			}
			else {	//�����ĸ�ֵ���
				build_DAG(strs);
			}
		}
	}
}

string dag_main(string filename){
	fin.open(filename.c_str());
	string dag_filename = "DAG" + filename;
	if (!DEBUG)fout.open(dag_filename.c_str());
	read_medis();
	if (!DEBUG)fout.close();
	if (!DEBUG)fout.close();
	fin.close();
	return dag_filename;
}
