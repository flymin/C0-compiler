#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <list>
#include "var.h"
#include "tar.h"
#include "grammar.h"
#include "table.h"
#include "words.h"
#include "live_var.h"
#include "main.h"
#include "const.h"
# define DEBUG 0
# if DEBUG
# define OUT_LEFT cout << "<==="
# define OUT_RIGHT "===>" << endl
# else
# define OUT_LEFT fout
# define OUT_RIGHT endl
# endif // DEBUG
# define OUTPUT(x) OUT_LEFT << x << OUT_RIGHT

using namespace std;

static ifstream fin;
static ofstream fout;

ofstream log_file;
typedef map<string, Fund_block*> CBLOCK_MAP;
typedef map<string, CBLOCK_MAP*> FUNC_CBLOCK_MAP;
typedef map<string, Data_node*> VARNODE_MAP;
FUNC_CBLOCK_MAP func_fblock_map;

list<Fund_block*> cblock_list;
CBLOCK_MAP* cblock_map_ptr;
Fund_block* cur_cblock;
string cur_funcname = "";
int temp_label_count = 0;
bool first_term = true;
set<Data_node*> var_graph;
list<Data_node*> var_stack;

/*====================
|        extern      |
====================*/

// @REQUIRES: funcname & blockname must be right
int get_regno(string funcname, string cblockname, string varname) {
	Fund_block* cblk = (*(func_fblock_map[funcname]))[cblockname];
	if (cblk->has_live(varname)) {
		return cblk->actives[varname]->regno;
	}
	else {
		return -1; // not in actives
	}
}

// 变量类型重载，这里结合基本块判断是否是局部变量
bool is_local_var(string funcname, string cblockname, string varname) {
	Fund_block* cblk = (*(func_fblock_map[funcname]))[cblockname];
	//if (varname == "a") cout << "OH!" << cblk->has_live(varname) <<cblk->has_def(varname);
	return (cblk->has_def(varname) && !cblk->has_live(varname));
}


/*********************
|     基本块类	     |
*********************/

void Fund_block::print_info() {
	log_file << "label:" << this->label << endl;

	set<string>::iterator it;

	it = this->uses.begin();
	log_file << "uses:" << endl;
	while (it != this->uses.end()) {
		log_file << *it << " ";
		it++;
	}

	log_file << endl;

	it = this->defs.begin();
	log_file << "defs:" << endl;
	while (it != this->defs.end()) {
		log_file << *it << " ";
		it++;
	}
	log_file << endl;

	VARNODE_MAP::iterator map_it;

	map_it = this->actives.begin();
	log_file << "actives:" << endl;
	while (map_it != this->actives.end()) {
		log_file << map_it->first << " ";
		map_it++;
	}
	log_file << endl;

	map_it = this->ins.begin();
	log_file << "in:" << endl;
	while (map_it != this->ins.end()) {
		log_file << map_it->first << " ";
		map_it++;
	}
	log_file << endl;

	map_it = this->outs.begin();
	log_file << "out:" << endl;
	while (map_it != this->outs.end()) {
		log_file << map_it->first << " ";
		map_it++;
	}
	log_file << endl;

	log_file << endl;
}

Fund_block::Fund_block(string label) {
	this->label = label;
}

bool Fund_block::has_def(string name) {
	return (this->defs.find(name) != this->defs.end());
}

bool Fund_block::has_use(string name) {
	return (this->uses.find(name) != this->uses.end());
}

bool Fund_block::has_in(string name) {
	return (this->ins.find(name) != this->ins.end());
}

bool Fund_block::has_out(string name) {
	return (this->outs.find(name) != this->outs.end());
}

bool Fund_block::has_live(string name) {
	return (this->actives.find(name) != this->actives.end());
}

void Fund_block::try_use(string name) {
	if (is_local_var(cur_funcname, name) && !has_def(name))
	{
		this->uses.insert(name);
	}
}

void Fund_block::try_def(string name) {
	if (is_local_var(cur_funcname, name) && !has_use(name)) {
		this->defs.insert(name);
	}
}

/* It combines all variables in the 'ins' of all cblocks
* in the 'nexts'.
* if changed, return true
*/
bool Fund_block::refresh_out() {
	bool changed = false;
	set<Fund_block*>::iterator cblock_it = this->nexts.begin();
	while (cblock_it != this->nexts.end()) { // nexts
		Fund_block* next_cblock = *cblock_it;
		VARNODE_MAP::iterator var_it = next_cblock->ins.begin();
		while (var_it != next_cblock->ins.end()) {// ins of a next
			string name = var_it->first;
			if (!this->has_out(name)) { // not in outs
				this->outs.insert(VARNODE_MAP::value_type
				(var_it->first, var_it->second));
				changed = true;
			}
			// should not redirect pointer
			// instead, redirect in nodes
			else {
				Data_node* in_node_ptr = var_it->second->get_terminal_ptr();
				Data_node* out_node_ptr = this->outs[name]->get_terminal_ptr();
				if (in_node_ptr != out_node_ptr) { // different node
					in_node_ptr->redirect_ptr = out_node_ptr;
					changed = true; // necessary?
				}
			}
			var_it++;
		}
		cblock_it++;
	}
	return changed;
}

// same value as outs
void Fund_block::refresh_in() {
	// in outs but not in defs
	VARNODE_MAP::iterator out_it = this->outs.begin();
	while (out_it != this->outs.end()) {
		string name = out_it->first;
		if (!has_in(name) && !has_def(name)) {
			this->ins.insert(VARNODE_MAP::value_type
			(out_it->first, out_it->second));
		}
		out_it++;
	}
	if (!first_term) {
		return;
	}
	// in uses
	set<string>::iterator use_it = this->uses.begin();
	while (use_it != this->uses.end()) {
		string name = *use_it;
		if (!has_in(name))
		{
			Data_node* vn = new Data_node(name);
			this->ins.insert(VARNODE_MAP::value_type(name, vn));
		}
		use_it++;
	}
}

/*********************
|     变量节点        |
*********************/

Data_node::Data_node(string name) {
	// cout << "name:" << name << endl;
	this->name = name;
}

Data_node* Data_node::get_terminal_ptr() {
	Data_node* vn = this;
	while (vn->redirect_ptr != NULL) {
		//cout << vn->name << endl;
		vn = vn->redirect_ptr;
	}
	if (vn != this) {
		this->redirect_ptr = vn; // refresh
	}
	return vn;
}

void Data_node::set_regno(int reg_max) {
	// initial
	vector<bool> reg_occupied;
	for (int i = 0; i < reg_max; i++) {
		reg_occupied.push_back(false);
	}
	// record
	set<Data_node*>::iterator it = this->conflicts.begin();
	while (it != this->conflicts.end()) {// conflict vnodes
		Data_node* vnode = *it;
		if (vnode->regno != -1) {
			reg_occupied[vnode->regno] = true;
		}
		it++;
	}
	// select
	for (int i = 0; i < reg_max; i++) {
		if (reg_occupied[i] == false) {
			this->regno = i;
			return;
		}
	}
	//error_debug("cannot distribute!");
}

void Data_node::cut_conflicts() {
	set<Data_node*>::iterator it = this->conflicts.begin();
	while (it != this->conflicts.end()) {// conflicts
		Data_node* conf_vnode = *it;
		conf_vnode->conf_count--; // reduce
		it++;
	}
}

/*********************
*     in / out       *
*********************/

void finish_function_in_out() {
	bool changed = true;
	while (changed) {// term
		changed = false;
		list<Fund_block*>::iterator it = cblock_list.begin();
		while (it != cblock_list.end()) { // code_blocks
			Fund_block* fblock = *it;
			changed |= fblock->refresh_out();
			fblock->refresh_in();
			it++;
		}
	}
}

void complete_set_varnodes(set<Data_node*>* dustbin,
	Fund_block* fblock, VARNODE_MAP* vnmap) {
	VARNODE_MAP::iterator vn_it;
	// harvest ins
	vn_it = vnmap->begin();
	while (vn_it != vnmap->end()) {
		string name = vn_it->first;
		Data_node* tmn = vn_it->second->get_terminal_ptr();
		if (tmn != vn_it->second) {// useless
			dustbin->insert(vn_it->second); // put into dustbin
			vn_it->second = tmn; // change ptr
		}
		if (!fblock->has_live(name)) {
			fblock->actives.insert(VARNODE_MAP::value_type
			(vn_it->first, vn_it->second)); // put into actives
		}
		vn_it++;
	}
}

void refresh_conflict(Fund_block* fblock) {
	VARNODE_MAP::iterator it = fblock->actives.begin();
	while (it != fblock->actives.end()) {// varnodes
		VARNODE_MAP::iterator conf_it = fblock->actives.begin();
		while (conf_it != fblock->actives.end()) {// varnodes
			if (conf_it != it) {// not self
				it->second->conflicts.insert(conf_it->second);
			}
			conf_it++;
		}
		var_graph.insert(it->second);
		// cout << it->second->name << endl;
		it++;
	}
}

// delete useless varnodes
// add
void complete_function_varnodes() {
	//cout << funcname << endl;
	set<Data_node*> useless_vns;
	list<Fund_block*>::iterator cb_it = cblock_list.begin();
	// put into actives | refresh conflict
	while (cb_it != cblock_list.end()) {// code_blocks
		Fund_block* fblock = *cb_it;
		complete_set_varnodes(&useless_vns, fblock, &fblock->ins);
		complete_set_varnodes(&useless_vns, fblock, &fblock->outs);
		refresh_conflict(fblock);
		fblock->print_info();
		cb_it++;
	}
	// delete useless var_nodes
	set<Data_node*>::iterator useless_it = useless_vns.begin();
	while (useless_it != useless_vns.end()) {
		delete(*useless_it);
		useless_it++;
	}
	useless_vns.clear();
}

void init_conflict_count() {
	set<Data_node*>::iterator it = var_graph.begin();
	while (it != var_graph.end()) {// go through var_graph
		Data_node* vnode = *it;
		vnode->conf_count = vnode->conflicts.size(); // init conflict count
		it++;
	}
}


// push vnode to stack, reduce conflict count of vnode in conflicts
void push_vnode_stack(set<Data_node*>::iterator it) {
	Data_node* vnode = *it;
	// push stack
	var_stack.push_front(vnode);
	var_graph.erase(it);
	// reduce conflict count
	vnode->cut_conflicts();
}

// one term of trying push vnodes into stack as many as possible
bool repush_stack(int reg_max) {
	bool put = false;
	set<Data_node*>::iterator it = var_graph.begin();
	while (it != var_graph.end()) {
		Data_node* vnode = *it;
		if (vnode->conf_count < reg_max) {
			push_vnode_stack(it++);
			put = true;
		}
		else {
			it++;
		}
	}
	return put;
}

// @REQUIRES: var_graph not empty
// select one vnode which will not be distributed a reg
// could be optimize
void select_vnode_without_reg() {
	Data_node* vnode_remove = *(var_graph.begin()); // select one
	log_file << "=============\n" << vnode_remove->name
		<< "\t" << vnode_remove->conflicts.size() << "\t" << vnode_remove->regno << "\n=============\n" << endl;
	var_graph.erase(vnode_remove); // remove from graph
	vnode_remove->cut_conflicts();
}

void graph_to_stack(int reg_max) {
	while (true) {
		bool put = true;
		do {
			put = repush_stack(reg_max);
		} while (put); // push as many as possible
		if (var_graph.empty()) {
			break; // all moved to stack
		}
		else {
			select_vnode_without_reg();
		}
	}
}

void stack_reg(int reg_max) {
	list<Data_node*>::iterator it = var_stack.begin();
	while (it != var_stack.end()) {
		Data_node* vnode = *it; // will be distributed with reg
		vnode->set_regno(reg_max);
		log_file << "=============\n" << vnode->name
			<< "\t" << vnode->conflicts.size() << "\t" << vnode->regno << "\n=============\n" << endl;
		it++;
	}
}

void reg_distri(int reg_max) {
	init_conflict_count();
	graph_to_stack(reg_max);
	stack_reg(reg_max);
}

void complete_function() {
	finish_function_in_out();
	complete_function_varnodes();
	reg_distri(REG_MAX);
}


/*********************
 *     functions     *
 *********************/

bool has_code_block(string label)
{
	return (cblock_map_ptr->find(label) != cblock_map_ptr->end());
}

// redirect use/define information to file (debug)
void print_cblock_list_info() {
	if (!DEBUG) return;
	list<Fund_block*>::iterator it = cblock_list.begin();
	while (it != cblock_list.end()) {
		Fund_block* tcb = *it;
		tcb->print_info();
		it++;
	}
}

void clear_graph() {
	//cout << "funcname:" << cur_funcname << endl;
	if (cur_funcname != "") {
		//cout << cblock_list.size() << endl;
		func_fblock_map.insert(FUNC_CBLOCK_MAP::value_type(cur_funcname, cblock_map_ptr));
	}
	complete_function();
	cblock_list.clear();
	cblock_map_ptr = new CBLOCK_MAP;
	var_graph.clear();
	var_stack.clear();
}

void init_graph(string funcname) {
	clear_graph();
	cur_funcname = funcname;
	temp_label_count = 0;
	first_term = true;
	string first_label_name = set_temp_label(); // output @label
	turn_next_block(first_label_name);  // create and set cur_cblock
}


// create & output & return temp label
string set_temp_label() {
	stringstream ss;
	ss << temp_label_count++;
	OUTPUT("@label " << ss.str());
	return ss.str();
}

// get fblock with label, if not exists, create one
Fund_block* get_code_block(string label) {
	if (has_code_block(label)) {
		return (*cblock_map_ptr)[label];
	}
	else {
		Fund_block* cb = new Fund_block(label);
		(*cblock_map_ptr)[label] = cb;
		return cb;
	}
}

// set next and prev
void link_code_blocks(Fund_block* prev, Fund_block* next) {
	prev->nexts.insert(next);
	next->prevs.insert(prev);
}

// get code_block with label & set cur_block & add to list
void turn_next_block(string label) {
	cur_cblock = get_code_block(label);
	cblock_list.push_front(cur_cblock);
}


void live_medis_read() {
	string line;
	while (getline(fin, line)) {
		istringstream is(line);
		string str;
		vector<string> strs;
		while (is >> str) {
			strs.push_back(str);
		}
		OUTPUT(line);

		if (strs[0] == "@var"	||	strs[0] == "@array" || 
			strs[0] == "@para"	||	strs[0] == "@call"	||
			strs[0] == "@jal"	||	strs[0] == "@exit"	||
			strs[0] == "@free") {
			//do nothing
		}
		else if (strs[0] == "@func")
		{
			init_graph(strs[1]);
		}
		else if (strs[0] == "@push")
		{
			cur_cblock->try_use(strs[1]);
		}
		else if (strs[0] == "@get")
		{
			cur_cblock->try_def(strs[1]);
		}
		else if (strs[0] == "@ret")
		{
			if (strs.size() > 1)
			{
				cur_cblock->try_use(strs[1]);
			}
		}
		else if (strs[0] == "@be" || strs[0] == "@bne")
		{
			cur_cblock->try_use(strs[1]);
			cur_cblock->try_use(strs[2]);
			link_code_blocks(cur_cblock, get_code_block(strs[3]));
			string temp_label = set_temp_label();
			link_code_blocks(cur_cblock, get_code_block(temp_label));
			turn_next_block(temp_label);
		}
		else if (strs[0] == "@bz" || strs[0] == "@bgtz" || strs[0] == "@bltz" ||
			strs[0] == "@blez" || strs[0] == "@bgez")
		{
			cur_cblock->try_use(strs[1]);
			link_code_blocks(cur_cblock, get_code_block(strs[2]));
			string temp_label = set_temp_label();
			link_code_blocks(cur_cblock, get_code_block(temp_label));
			turn_next_block(temp_label);
		}
		else if (strs[0] == "@j")
		{
			link_code_blocks(cur_cblock, get_code_block(strs[1]));
		}
		else if (strs[0] == "@printf")
		{
			if (strs[1] != "STRING" && strs[1] != "LINE")
			{
				cur_cblock->try_use(strs[2]);
			}
		}
		else if (strs[0] == "@scanf")
		{
			cur_cblock->try_def(strs[2]);
		}
		else if (strs[1] == ":")
		{
			link_code_blocks(cur_cblock, get_code_block(strs[0]));
			turn_next_block(strs[0]);
		}
		else if (strs.size() == 3)
		{
			cur_cblock->try_use(strs[2]);
			cur_cblock->try_def(strs[0]);
		}
		else if (strs.size() == 4)			//ARRSET
		{
			cur_cblock->try_use(strs[2]);
			cur_cblock->try_use(strs[3]);
		}
		else if (strs.size() == 5)
		{
			if (strs[3] != "ARRGET")
			{
				cur_cblock->try_use(strs[2]);
			}
			cur_cblock->try_use(strs[4]);
			cur_cblock->try_def(strs[0]);
		}
		else
		{
			//error_debug("cal len not 3 or 5 in livevar");
		}
	}
}

string livevar_main(string filename)
{
	fin.open(filename.c_str());
	string lv_filename = generate_filename("LIVEVAR");
	fout.open(lv_filename.c_str());
	log_file.open(generate_filename("LIVE_LOG").c_str());

	live_medis_read();
	clear_graph();

	log_file.close();
	fout.close();
	fin.close();

	return lv_filename;
}