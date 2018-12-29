# include <iostream>
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
	//this->use_count = use_counter++;
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
	if (this->state == MODIFIED)
	{
		// cout << "SAVE" << this->name << endl;
		// store old value
		if (this->global)
		{
			MIPS_OUTPUT( "sw" << " "
				<< this->regname << ", " << this->offset
				<< "($gp) # store " << this->name);
		}
		else
		{
			MIPS_OUTPUT( "sw" << " "
				<< this->regname << ", " << this->offset
				<< "($fp) # store " << this->name);
		}
	}
}

void Reg_recorder::load()
{
	if (is_num(this->name))
	{
		MIPS_OUTPUT("li " << this->regname << ", " << this->name);
	}
	else if (offset != -1 && this->global)   // is global variable
	{
		MIPS_OUTPUT(((this->type == CHAR) ? "lb" : "lw") << " "
			<< this->regname << ", " << this->offset
			<< "($gp) # load " << this->name);
	}
	else if (offset != -1)
	{
		MIPS_OUTPUT(((this->type == CHAR) ? "lb" : "lw") << " "
			<< this->regname << ", " << this->offset
			<< "($fp) # load " << this->name);
	}
}

#endif