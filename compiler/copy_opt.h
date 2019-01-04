#ifndef ASS_OPT_H_INCLUDED
#define ASS_OPT_H_INCLUDED

# define IS_TEMP(name) (name[0] == '#')

class Line {
public:
	bool active = false;
	vector<string> last_use_names;  // must be temp

	Line(bool active)
	{
		this->active = active;
	}
};

class Block_copy {
public:
	int def_line_no;
	string name;
	Block_copy* source = NULL;	// ָ��ǰBlock_copy��������Դ
	int last_used_line = -1;

	Block_copy(int line, string name) {
		this->def_line_no = line;
		this->name = name;
	}

	Block_copy* get_source() {	// def block��sourceΪ�Լ���useblock��sourceΪ�����
		Block_copy* block = this;
		while (block->source != NULL) {
			block = block->source;
		}
		return block;
	}
};

string use(string usename);
string copy_main(string, int*);

#endif // ASS_OPT_H_INCLUDED
