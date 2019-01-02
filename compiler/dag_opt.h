#ifndef DAG_OPT_H_INCLUDED
#define DAG_OPT_H_INCLUDED

#include <iostream>
#include <map>
#include <vector>
#include "var.h"
#include "table.h"

#define IS_VAR(name) (name[0] == '_' || (name[0] >= 'a' && name[0] <= 'z') || \
							(name[0] >= 'A' && name[0] <= 'Z'))
#define IS_NUM(name) ((name[0] >= '0' && name[0] <= '9') || name[0] == '-')

using namespace std;

class Node;
//class FuncItem;

//type for string to node map
typedef map<string, Node*> NODE_MAP;
extern NODE_MAP node_map;

// vector to record all nodes
extern vector<Node*> nodes;
extern int temp_count;
extern Sym *cur_func_DAG;

class Node {
public:
	int no;
	Node* lptr;			// 左节点
	Node* rptr;			// 右节点
	bool is_leaf;
	string content;		// 操作符
	bool is_certain;	// 所有相关子节点计算完成
	string name;

	Node(string value);
	Node(string op, Node* lptr, Node* rptr);
	void make_certain();
};

void read_medis();
NODE_MAP::iterator export_code(NODE_MAP::iterator it);
NODE_MAP::iterator record_code(NODE_MAP::iterator it);
void refresh_global_vars();
void refresh_vars();
void init_DAG();
bool has_node(string name);
void add_to_map(string name, Node* nodeptr);
void set_node(string name, Node* nodeptr, bool);
Node* get_node(string name);
void set_name(Node* node);
string get_name(Node* node);
//string get_name(string old_name);
bool is_global_var(string name);
void build_DAG(vector<string> code);
string dag_main(string);
string use_new_name(string name);

#endif // DAG_OPT_H_INCLUDED