# include <iostream>
#include <algorithm>
# include "tar.h"
# include "var.h"
# include "words.h"
# include "reg_recorder.h"
# include "table.h"
# include "tar.h"
# include "live_var.h"
# define DEBUG 0
# if DEBUG
# define OUT_LEFT cout
# define OUT_RIGHT endl
# else
# define OUT_LEFT tarfile
# define OUT_RIGHT endl
# endif // DEBUG
# define OUTPUT(x) OUT_LEFT << x << OUT_RIGHT


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
	this->save();
	this->init();
	this->use_count = use_counter++;
}

void Reg_recorder::init()
{
	// erase map
	if (has_name(this->name))
	{
		var_regmap.erase(var_regmap.find(this->name)); // map erase
	}
	// init
	this->name = "";
	this->state = INACTIVE;
}


void Reg_recorder::save()
{
	if (this->state == MODIFIED)
	{
		// cout << "SAVE" << this->name << endl;
		// store old value
		if (this->global)
		{
			OUTPUT("sw " << this->regname << ", " << this->offset
				<< "($gp) # store " << this->name);
		}
		else
		{
			OUTPUT("sw " << this->regname << ", -" << this->offset
				<< "($fp) # store " << this->name);
		}
	}
}

void Reg_recorder::load()
{
	if (is_num(this->name))
	{
		OUTPUT("li " << this->regname << ", " << this->name);
	}
	else if (offset != -1 && this->global)   // is global variable
	{
		OUTPUT("lw " << this->regname << ", " << this->offset
			<< "($gp) # load " << this->name);
	}
	else if (offset != -1)
	{
		OUTPUT("lw " << this->regname << ", -" << this->offset
			<< "($fp) # load " << this->name);
	}
}

// before call
void Reg_recorder::record_occu_regs(list<string>* save_list)
{
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		if (rec->state == OCCUPIED)
		{
			save_list->push_back(rec->regname);
		}
		it++;
	}
}

// before call
void Reg_recorder::save_occu_regs(list<string>* save_list, int offset)
{
	list<string>::iterator it = save_list->begin();
	while (it != save_list->end())
	{
		OUTPUT("sw " << *it << ", " << offset << "($sp)");
		offset += 4;
		it++;
	}
}

// after call
void Reg_recorder::load_occu_regs(list<string>* save_list, int offset)
{
	list<string>::iterator it = save_list->begin();
	while (it != save_list->end())
	{
		OUTPUT("lw " << *it << ", " << offset << "($sp)");
		offset += 4;
		it++;
	}
}

// label
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

// before branch
void Reg_recorder::init_var_occu_regs()
{
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		if (rec->state == OCCUPIED && rec->name[1] == 's')
		{
			rec->init();
		}
		it++;
	}
}

// before return
void Reg_recorder::init_all()
{
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		rec->init();
		it++;
	}
}

// before return
void Reg_recorder::save_global_modi_regs()
{
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		if (rec->global)
		{
			rec->save();
		}
		it++;
	}
}

void Reg_recorder::local_modi_regs(void(Reg_recorder::*func)(), bool not_reverse = true)
{
	REG_MAP::iterator it = reg_regmap.begin();
	while (it != reg_regmap.end())
	{
		Reg_recorder* rec = it->second;
		if (is_local_var(cur_func->name, cur_label, rec->name) ^ (!not_reverse))
		{
			// if (!not_reverse) cout << "LOCAL" << rec->name << endl;
			(rec->*func)();
		}
		it++;
	}
}

// a, #0, #8, global_a, local_a, not_occup_a
void Reg_recorder::before_branch_jump()
{
	Reg_recorder::local_modi_regs(&Reg_recorder::save, false);
	Reg_recorder::init_var_occu_regs();
	Reg_recorder::local_modi_regs(&Reg_recorder::init, true);
}

void Reg_recorder::before_label()
{
	Reg_recorder::local_modi_regs(&Reg_recorder::save, false);
	Reg_recorder::init_all();
}

void Reg_recorder::before_return()
{
	Reg_recorder::save_global_modi_regs();
	Reg_recorder::init_all();
}

#endif