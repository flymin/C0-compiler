#ifndef ASS_OPT_H_INCLUDED
#define ASS_OPT_H_INCLUDED

class Line
{
public:
	bool active = false;
	vector<string> last_use_names;  // must be temp

	Line(bool active)
	{
		this->active = active;
	}
};

class Block
{
public:
	int def_line;
	string name;
	Block* nature = NULL;	// 指向当前Block的数据来源
	int last_used_line = -1;

	Block(int line, string name)
	{
		this->def_line = line;
		this->name = name;
	}

	Block* get_nature() {	// def block的nature为自己，useblock的nature为定义点
		Block* block = this;
		while (block->nature != NULL)
		{
			block = block->nature;
		}
		return block;
	}
};

string use(string usename);
string ass_main(string, int*);

#endif // ASS_OPT_H_INCLUDED
