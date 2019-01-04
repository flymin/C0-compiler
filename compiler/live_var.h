#ifndef LIVEVAR_ANA_H_INCLUDED
#define LIVEVAR_ANA_H_INCLUDED

#include <iostream>
#include <set>
#include <map>
using namespace std;

class Fund_block;
extern map<string, map<string, Fund_block*>*> func_fblock_map;

class Data_node
{
public:
	string name;
	Data_node* redirect_ptr = NULL;
	set<Data_node*> conflicts;
	int conf_count = 0;
	int regno = -1;
	Data_node(string name);
	Data_node* get_terminal_ptr();
	void set_regno(int reg_max);
	void cut_conflicts();
};

class Fund_block
{
public:
	string label;
	set<Fund_block*> prevs;
	set<Fund_block*> nexts;
	set<string> uses;
	set<string> defs;
	map<string, Data_node*> ins; // same values as outs
	map<string, Data_node*> outs; // can be enlarge, but not modified
	map<string, Data_node*> actives; // ins | outs

	Fund_block(string label);
	void print_info();
	bool has_def(string name);
	bool has_use(string name);
	bool has_in(string name);
	bool has_out(string name);
	bool has_live(string name);
	void try_use(string name);
	void try_def(string name);
	bool refresh_out();
	void refresh_in();
};

int get_regno(string funcname, string cblockname, string varname);
bool is_local_var(string funcname, string cblockname, string varname);
string livevar_main(string filename);
string set_temp_label();
void turn_next_block(string label);

#endif // LIVEVAR_ANA_H_INCLUDED
