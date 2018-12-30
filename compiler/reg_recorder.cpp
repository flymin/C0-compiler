# include <iostream>
#include <algorithm>
# include "tar.h"
# include "var.h"
# include "words.h"
# include "reg_recorder.h"
# include "table.h"
# include "tar.h"
# define DEBUG 0
# if DEBUG
# define MIPS_LEFT cout
# define MIPS_RIGHT endl
# else
# define MIPS_LEFT tarfile
# define MIPS_RIGHT endl
# endif // DEBUG
# define MIPS_OUTPUT(x) MIPS_LEFT << x << MIPS_RIGHT


using namespace std;

/*extern from main.cpp*/
extern ofstream tarfile;
extern fstream midfile;
int use_counter = 0;

#ifdef NEW_TAR

Reg_recorder::Reg_recorder(string regname)
{
	this->regname = regname;
	this->state = INACTIVE;
}

void Reg_recorder::clear_and_init()
{
	//this->save();
	this->init();
	this->use_count = use_counter++;
	REG_MAP::iterator it = name_regmap.end();
	it = find_if(name_regmap.begin(), name_regmap.end(), map_value_finder(this->regname));
	if (it != name_regmap.end()) {
		name_regmap.erase(it);
	}
}

void Reg_recorder::init()
{
	// erase map
	//if (has_name(this->name))
	//{
		//name_regmap.erase(name_regmap.find(this->name)); // map erase
	//}
	// init
	this->name = "";
	this->state = INACTIVE;
}

void Reg_recorder::save()
{
	int addr;
	Sym* var;
	if (this->state == MODIFIED)
	{
		// cout << "SAVE" << this->name << endl;
		// store old value
		
		if (is_temp(this->name)) {
			addr = 4 * get_temp_no(this->name) + temp_base_addr;
			MIPS_OUTPUT("sw " << this->regname << ", -" << addr << "($fp) # store " << this->name);
		}
		else {
			var = findSym(cur_func, (char*)this->name.data());
			if (var->global) { // is global variable
				MIPS_OUTPUT("sw " << this->regname << ", -" << global_addr_map[var->name] << "($gp) # store " << this->name);
			}
			else {
				MIPS_OUTPUT("sw " << this->regname << ", -" << offset_map[var->name] << "($fp) # store " << this->name);
			}
		}
		/*if (this->global)
		{
			MIPS_OUTPUT( "sw" << " "
				<< this->regname << ", -" << this->offset
				<< "($gp) # store " << this->name);
		}
		else
		{
			MIPS_OUTPUT( "sw" << " "
				<< this->regname << ", -" << this->offset
				<< "($fp) # store " << this->name);
		}
		*/
	}
}

void Reg_recorder::load()
{
	int addr;
	Sym* var;
	if (is_num(this->name)) {
		MIPS_OUTPUT("li " << this->regname << ", " << this->name);
	}
	else {
		if (is_temp(this->name)) {
			addr = 4 * get_temp_no(this->name) + temp_base_addr;
			MIPS_OUTPUT("lw " << this->regname << ", -" << addr << "($fp) # load " << this->name);
		}
		else {
			var = findSym(cur_func, (char*)this->name.data());
			if (var->global) { // is global variable
				MIPS_OUTPUT("lw " << this->regname << ", -" << global_addr_map[var->name] << "($gp) # load " << this->name);
			}
			else {
				MIPS_OUTPUT("lw " << this->regname << ", -" << offset_map[var->name] << "($fp) # load " << this->name);
			}
		}
	}
	/*
	else if (offset != -1 && this->global)   // is global variable
	{
		MIPS_OUTPUT("lw" << " "
			<< this->regname << ", -" << this->offset
			<< "($gp) # load " << this->name);
	}
	else if (offset != -1)
	{
		MIPS_OUTPUT("lw" << " "
			<< this->regname << ", -" << this->offset
			<< "($fp) # load " << this->name);
	}
	*/
}

void Reg_recorder::save_modi_regs(list<string>* save_list)
{
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		rec->save(); // only MODIFIED can save
		if (rec->state == MODIFIED) {
			save_list->push_back(rec->regname);
			rec->state = INACTIVE;
		}
		it++;
	}
}

void Reg_recorder::clear_and_init_all()
{
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		rec->clear_and_init();
		it++;
	}
}

#endif