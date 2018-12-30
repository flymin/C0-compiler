#ifndef TAR_H_INCLUDED
#define TAR_H_INCLUDED
#include "mid_code.h"
#include "var.h"
#include "grammar.h"
#include "const.h"
#include "table.h"
#include "words.h"
#include "tar.h"
#include "reg_recorder.h"
#include <vector>
#include <map>
#include <set>

#define NEW_TAR

class map_value_finder
{
public:
	map_value_finder(const std::string &cmp_string) :m_s_cmp_string(cmp_string) {}
	bool operator ()(const std::map<string, Reg_recorder*>::value_type &pair)
	{
		return pair.second->regname == m_s_cmp_string;
	}
private:
	const std::string &m_s_cmp_string;
};

extern int temp_base_addr;
extern Sym *cur_func;

typedef map<string, int> STRINT_MAP;
extern STRINT_MAP offset_map;
extern STRINT_MAP global_addr_map;

typedef map<string, Reg_recorder*> REG_MAP;
extern REG_MAP reg_regmap;
extern REG_MAP name_regmap;

bool is_num(string str);
void tar_code();
bool is_temp(string name);

int get_temp_no(string name);


#endif