#include<iostream>
#include <sstream>
#include<fstream>
#include<string>
#include "var.h"
#include "const.h"
#include "words.h"
#include "grammar.h"
#include "table.h"
#include <stdarg.h>
#include <assert.h>
#include "mid_code.h"
#include <algorithm>

using namespace std;


/*extern from main.cpp*/
extern char token[TOKEN_MAX_LENTH];	// store sign
extern int token_len;
extern int num;								// current integer
extern char cur_c;							// current char to read
extern Symbol symbol;
extern bool success;
extern int line_no;

/*extern from words.cpp*/
extern int line_index;			//需要预读字符判断时，针对指针进行调整
								//预读只在当前行进行
extern char line_buffer[LINE_MAX_LENGTH];
extern fstream midfile;

/*本文件中使用的全局变量*/
#define print_gram	false
bool mark = false;		//这是为获取下一行设置的标记
Type return_type = VOID;
Sym *this_func;
int temp_max;
bool return_last = false;
int error_sym = 0;

/*这是为预读设置的临时存储*/
char token_temp[TOKEN_MAX_LENTH];
int token_len_temp;
int num_temp;
char cur_c_temp;
Symbol symbol_temp;
bool success_temp;
int line_index_temp;
char line_buffer_temp[LINE_MAX_LENGTH];

void print_gra(string msg) {
	if (print_gram) {
		cout << "In line " << line_no << ": ";
		cout << msg << endl;
	}
}

void gram_error(string info){
	success = false;
	cout << line_buffer << endl;
	cout << "[GRAM ERROR] " << info << " in " << line_no << endl;
}

void getsym_check() {
	start:
	if (mark) {
		mark = getsym();
		if (symbol == NONE) {
			goto start;
		}
		return;
	}
	while(!mark || !line_buffer[0]) {
		if (!readline()) {
			gram_error("unfinished program");
			exit(0);
		}
		mark = true;
	}
	mark = getsym();
	if (symbol == NONE) {
		goto start;
	}
	return;
}

bool getsym_next() {
	if (!getsym()) {
		gram_error("unfinished line");
		return false;
	}
	return true;
}

bool check_legal(int num, ...) {
	va_list permit;
	va_start(permit, num);
	Symbol temp;
	int i;
	for (i = 0; i < num; i++) {
		temp = va_arg(permit, Symbol);
		if (temp == symbol)
			return true;
	}
	return false;
}

/*直接调整数字位置token的num*/
void signed_int() {
	switch (symbol) {
	case ADD:
		getsym_check();
		if (symbol != INTCON) {
			gram_error("整数匹配错误！");
		}
		break;
	case SUB:
		getsym_check();
		if (symbol != INTCON) {
			gram_error("整数匹配错误！");
		}
		num = -num;
		break;
	case INTCON:
		break;
	default:
		gram_error("整数匹配错误！");
	}
	print_gra("This is a int");
}

/*循环处理完所有const*/
void const_declare() {
	int const_num;
	Sym *temp = NULL;
	while (symbol == CONSTSY) {
		getsym_check();
		switch (symbol) {
		case INTSY:
			getsym_check();
			while (check_legal(1, IDENT)) {
				//insymbol()插入符号表，返回插入位置
				temp = pushTable(token, CONST, INT);
				getsym_check();
				if (symbol == BECOME) {
					getsym_check();
					if (check_legal(4, ADD, SUB, INTCON)) {
						signed_int();
						const_num = num;
						//editsymbol()将值填入符号表
						temp->value = const_num;
						if (!this_func) {
							temp->global = true;
						}
						else {
							temp->global = false;
						}
						getsym_check();
						if (symbol == COMMA) {
							getsym_check();
							continue;
						}
						else if (symbol != SEMI) {
							gram_error("缺失分号！");
							while (!check_legal(1, SEMI)) {
								getsym_check();
							}
							getsym_check();
							break;
						}
						else {//this is for semi
							break;
						}
					}
					else if (symbol == IDENT) {
						gram_error("非法的等式右值！");
						const_num = 0;
						while (!check_legal(1, SEMI)) {
							getsym_check();
						}
						getsym_check();
						break;
					}
					getsym_check();
				}
			}
			if (symbol == SEMI) {
				getsym_check();
				break;
			}
			else {
				gram_error("缺失分号！");
				while (!check_legal(1, SEMI)) {
					getsym_check();
				}
				getsym_check();
				break;
			}
		case CHARSY:
			getsym_check();
			while (check_legal(1, IDENT)) {
				//insymbol()插入符号表，返回插入位置
				temp = pushTable(token, CONST, CHAR);
				getsym_check();
				if (symbol == BECOME) {
					getsym_check();
					if (symbol == CHARCON) {
						const_num = num;
						//editsymbol()将值填入符号表
						temp->value = const_num;
						if (!this_func) {
							temp->global = true;
						}
						else {
							temp->global = false;
						}
						getsym_check();
						if (symbol == COMMA) {
							getsym_check();
							continue;
						}
						else if (symbol != SEMI) {
							gram_error("缺失分号！");
							while (!check_legal(1, SEMI)) {
								getsym_check();
							}
							getsym_check();
							break;
							//error()缺失分隔符
						}
						else {//this is for semi
							break;
						}
					}
					else if (symbol == IDENT) {
						//error():
						const_num = 0;
						gram_error("缺失分号！");
						while (!check_legal(1, SEMI)) {
							getsym_check();
						}
						getsym_check();
						break;
					}
					getsym_check();
				}
			}
			if (symbol == SEMI) {
				getsym_check();
				break;
			}
			else {
				gram_error("缺失分号！");
				while (!check_legal(1, SEMI)) {
					getsym_check();
				}
				getsym_check();
				break;
				//从不完整的常量定义结束
			}
		default:
			gram_error("非法的常量声明！");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
			getsym_check();
			break;
			//error() 不合法的类型标识符
		}//switch
		print_gra("This is a const declare");
	}//while (symbol == CONSTSY)
}

void record_state() {
	strcpy_s(token_temp, token);
	token_len_temp = token_len;
	num_temp = num;
	cur_c_temp = cur_c;
	symbol_temp = symbol;
	success_temp = success;
	line_index_temp = line_index;
	strcpy_s(line_buffer_temp, line_buffer);
}

void restore_state() {
	strcpy_s(token, token_temp);
	token_len = token_len_temp;
	num = num_temp;
	cur_c = cur_c_temp;
	symbol = symbol_temp;
	success = success_temp;
	line_index = line_index_temp;
	strcpy_s(line_buffer, line_buffer_temp);
}

State is_var_declare() {	//调用之前检测到类型关键字
	again:
	record_state();
	bool next_mark = mark;
	if (!next_mark) {
		gram_error("错误的变量或函数声明！");
		while (!check_legal(3, VOIDSY, CHARSY, INTSY)) {
			getsym_check();
		}
		goto again;
	}
	next_mark = getsym_next();
	if (symbol == MAINSY) {
		restore_state();
		return MAIN_STATE;
	}
	if (symbol != IDENT) {
		gram_error("缺失标识符！");
		while (!check_legal(3, VOIDSY, CHARSY, INTSY)) {
			getsym_check();
		}
		goto again;
		//error:类型关键字之后不是标识符
		//restore_state();
		//return NOSTATE;
	}
	if (!next_mark) {
		gram_error("缺失标识符！");
		while (!check_legal(3, VOIDSY, CHARSY, INTSY)) {
			getsym_check();
		}
		goto again;
		//restore_state();
		//return NOSTATE;
	}
	next_mark = getsym_next();
	switch (symbol) {
	case LBRC:
	case LPAR:
		restore_state();
		return FUNC_STATE;
	case LBKT:
	default:
		restore_state();
		return VAR_STATE;
	}
}

Kind var_or_array() {
	record_state();
	getsym_next();
	if (symbol == LBKT) {
		restore_state();
		return ARRAY;
	}
	else {
		restore_state();
		return VAR;
	}
}

/* 调用之前检测到类型关键字，并确定是变量声明
   此函是一次只识别一条声明语句（分号结束）*/
void var_declare() {
	Kind kind;
	Type type;
	Sym *temp = NULL;
	int array_length = 1;
	if (symbol == CHARSY) {
		type = CHAR;
	}
	else {
		type = INT;
	}
	getsym_check();
	if (symbol != IDENT) {
		gram_error("缺失标识符！");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		getsym_check();
		//error:应出现标识符
		//skip:跳读到结束符号
		return;
	}
	while (symbol == IDENT) {	
		kind = var_or_array();
		if (kind != ARRAY) {
			//insymbol(kind, type)插入符号表，返回插入位置
			temp = pushTable(token, VAR, type);
			if (!this_func) {
				temp->global = true;
			}
			else {
				temp->global = false;
			}
			getsym_check();
			if (symbol == COMMA) {
				getsym_check();
			}
		}
		else {
			//insymbol(kind, type, array_length)插入符号表，返回插入位置
			temp = pushTable(token, ARRAY, type);
			getsym_check();		//这个符号一定是中括号
			assert(symbol == LBKT);
			getsym_check();
			if (symbol != INTCON) {
				gram_error("应输入整数！");
				while (!check_legal(1, RBKT)) {
					getsym_check();
				}
				array_length = 1;
				goto add_length;
			}
			else {
				array_length = num;
				if (array_length <= 0) {
					gram_error("数组长度应该大于零！");
					//error: the leng of the array should greater than 0
					array_length = 1;
				}
				//修改符号表
				getsym_check();
			add_length:
				temp->paranum = array_length;
				if (!this_func) {
					temp->global = true;
				}
				else {
					temp->global = false;
				}
				if (symbol != RBKT) {
					//error:缺失中括号
					gram_error("缺失中括号！");
					while (!check_legal(1, SEMI)) {
						getsym_check();
					}
					getsym_check();
					break;
				}
				else {
					getsym_check();
					if (symbol == COMMA) {
						getsym_check();
					}
				}
			}
		}//if (symbol != INTCON)...else...
		/*产生中间代码*/
		if (!temp) {
			gram_error("声明错误");
		}
		else {
			declare_var_mid(temp);
		}
	}//while (symbol == IDENT);当前类型语句结束
	if (symbol != SEMI) {
		gram_error("缺失分号！");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		getsym_check();
		//error:缺失分号
	}
	else {
		print_gra("This is a var declare");
		getsym_check();
	}
	
}

/*待更正：函数头部需要返回符号表指针，匹配完参数列表之后填充参数*/
Sym* func_head() {
	Type type;
	Sym* temp = NULL;
	switch (symbol) {
	case INTSY:
		type = INT;
		return_type = INT;
		break;
	case CHARSY:
		type = CHAR;
		return_type = CHAR;
		break;
	default:
		type = VOID;
		return_type = VOID;
	}
	getsym_check();
	if (symbol != IDENT && symbol != MAINSY) {
		gram_error("缺失函数名称！");
		while (!check_legal(1, LPAR)) {
			getsym_check();
		}
		string tempstr = "#unnamefunc" + to_string(error_sym++);
		temp = pushTable((char*)tempstr.data(), FUNC, type);
		getsym_check();
		//error:应输入标识符
		return temp;
	}
	//insymbol(FUNC, type)插入符号表，返回插入位置
	//初始化参数表指针为NULL，参数个数为0
	temp = pushTable(token, FUNC, type);
	getsym_check();
	print_gra("This is a func head");
	return temp;
}


/*入口要求跳过左括号，即当前字符停留在类型关键字上
  参数列表为空则应为右括号
  待更正：输入符号表项，补充参数个数以及参数表项指针，done*/
void para_list(Sym *func) {
	Sym *temp = NULL;
	if (symbol == RPAR) {
		getsym_check();
		return;
	}
	//申请参数表项
	func->value = SymTable.index;
	int para_num = 0;
	Type type;
	while (symbol == INTSY || symbol == CHARSY) {
		switch (symbol) {
		case INTSY:
			getsym_check();
			if (symbol != IDENT) {
				gram_error("缺少参数名！");
				//error:缺少标识符
				while (!check_legal(1, RPAR)) {
					getsym_check();
				}
				goto para_end;
			}
			//将当前参数插入参数表，类型int
			type = INT;
			temp = pushTable(token, PARA, type);
			if (!temp) {
				gram_error("fital:符号表插入失败！");
				//error:符号表插入失败
				exit(0);
			}
			break;
		case CHARSY:
			getsym_check();
			if (symbol != IDENT) {
				while (!check_legal(3, RPAR, LBKT, SEMI)) {
					getsym_check();
				}
			}
			//将当前参数插入参数表，类型char
			type = CHAR;
			temp = pushTable(token, PARA, type);
			if (!temp) {
				goto no_mid;
				//error:符号表插入失败
			}
			break;
		}
		declare_para_mid(type, temp);
	no_mid:
		para_num++;
		getsym_check();
		if (symbol == COMMA) {
			getsym_check();
		}
		
	}//while;参数声明循环
	if (symbol != RPAR) {
		gram_error("缺少右括号！");
		//error:缺少右括号
		while (!check_legal(2, RPAR, LBRC)) {
			getsym_check();
		}
		if (symbol == LBRC) {
			func->paranum = para_num;
			return;
		}
	}
para_end:
	func->paranum = para_num;
	print_gra("This is a param list");
	getsym_check();
}

void func_declare() {
	print_gra("\tFunc declare start");
	temp_max = 0;
	Sym *temp = NULL;
	int stack_size;
	temp = func_head();
	if (!temp) {
		vector<char> pair_temp;
		while (!check_legal(1, LBRC)) {
			getsym_check();
		}
		pair_temp.push_back('{');
		getsym_check();
		while (!pair_temp.empty()) {
			while (!check_legal(2, LBRC, RBRC)) {
				getsym_check();
			}
			if (symbol == LBRC) {
				pair_temp.push_back('{');
			}
			else {
				pair_temp.pop_back();
			}
			getsym_check();
		}
		return;
	}
	this_func = temp;
	declare_func_mid(temp);	//函数声明中间代码
	if (symbol == LPAR) {
		getsym_check();
		para_list(temp);
	}
	else {
		//error:没有参数列表
		gram_error("缺少右大括号！");
		while (!check_legal(2, SEMI, LBRC)) {
			getsym_check();
		}
		getsym_check();
		goto func_body;
	}
	if (symbol == LBRC) {
		getsym_check();
	}
	else {
		//error:没有左大括号
		gram_error("缺少左大括号！");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		getsym_check();
	}
func_body:
	composed_state();
	if (symbol != RBRC) {
		//error:没有右大括号
		gram_error("缺少右大括号！");
		while (!check_legal(1, RBRC)) {
			getsym_check();
		}
	}
	if (this_func->type != VOID) {
		if (!this_func->return_check) {
			if (!this_func->return_check_else) {		//最后是if语句
				sema_error(string("函数 ") + this_func->name + string(" 不是所有路径都有返回值！"));
			}
			else{		//最后一句不是if
				sema_error(string("函数 ") + this_func->name + string(" 缺少返回值！"));
			}
		}
	}
	else {
		if (!this_func->return_check) {
			return_mid();
		}
	}
	stack_size = ClearSymTable();	//出口处统计局部变量需要的栈大小
	temp->vsize = stack_size;
	temp->psize = stack_size + 4 * temp_max;
	print_gra("This is a func declare\n");
	getsym_check();
}

/*外层函数负责检索符号表确定函数返回值是否符合要求
  传入参数为函数的形参个数，入口符号停留在左括号之后
  这里产生参数传递过程，负责生成全部中间代码*/
void call(Sym *func) {
	int num = 0;
	int para_num = func->paranum;
	int para_base = func->value;
	bool exceed_flag = false;
	Type para_type, exp_type;
	while (symbol != RPAR && symbol != SEMI
		&& symbol != MUL  && symbol != DIV) {
		Ret_item para;
		exp_type = expression(&para);
		if (exp_type == VOID) {
			while (!check_legal(2,COMMA, RPAR)) {
				getsym_check();
			}
			if (num > para_num) {//参数个数超过限制
				exceed_flag = true;
			}
			goto cont;
		}
		//表达式要返回类型
		if (num <= para_num) {//参数个数合法
			para_type = SymTable.element[para_base + num].type;
			if (exp_type != para_type) {
				//error:类型不匹配
				sema_error("函数" + string(func->name) + "的第" + to_string(num+1) + "个参数类型不匹配!");
			}
			if (para.certain) {//传递数字
				push_mid(para.value);
			}
			else {//传递变量
				push_mid(para.name);
			}
		}
		else {//参数个数超过限制
			exceed_flag = true;
		}
	cont:
		num++;
		if (symbol != COMMA) {
			break;
		}
		getsym_check();
	}//while;实参循环
	if (num < para_num) {
		//error:参数数量不足
		sema_error("函数" + string(func->name) + "参数个数不足！");
	}
	if (exceed_flag) {
		//error:参数数量超过限制
		sema_error("函数" + string(func->name) + "参数个数超过限制！");
	}
	if (symbol != RPAR) {
		//error:小括号不匹配
		gram_error("缺少右小括号！");
		//while (!check_legal(1, RPAR)) {
		//	getsym_check();
		//}
		return;
	}
	else {
		call_func_mid(func);
		print_gra("This is a funciton call");
	}
	getsym_check();
}

/*要求上层函数预读入一个字符*/
Type factor(Ret_item* box) {
	Sym *temp = NULL;
	Ret_item index;
	Type type;
	Type exp_type = NOTYPE;
	box->certain = false;
	char ident[TOKEN_MAX_LENTH] = { 0 };
	switch (symbol) {
	case ADD:
	case SUB:
	case INTCON:
		signed_int();
		box->value = num;
		box->certain = true;
		type = INT;
		getsym_check();
		break;
	case CHARCON:
		type = CHAR;
		box->value = num;
		box->certain = true;
		type = CHAR;
		getsym_check();
		break;
	case LPAR:
		getsym_check();
		expression(box);
		if (symbol != RPAR) {
			//error:右括号不匹配
			gram_error("缺少右小括号！");
			while (!check_legal(14, ADD, SUB, MUL, DIV, SEMI, COMMA,
								RPAR, RBKT,
								EQ, NEQ, GT, GE, LT, LE)) {
				getsym_check();
			}
			if (symbol == SEMI) {
				type = VOID;
				return type;
			}
			else if (symbol == ADD || symbol == SUB ||
				symbol == MUL || symbol == DIV) {
				type = VOID;
				break;
			}
		}
		type = INT;
		getsym_check();
		break;
	case IDENT:
		//检索符号表，保留指针
		strcpy_s(ident, token);
		getsym_check();
		switch (symbol) {
		case LBKT://数组
			temp = findSym(ident);
			if (!temp || temp->kind != ARRAY) {
				//error:错误的数组名
				if (temp->kind != ARRAY) {
					sema_error("错误的数组名！");
				}
				else if (!temp) {
					sema_error("标识符未定义！");
				}
				while (!check_legal(14, ADD, SUB, MUL, DIV, SEMI, COMMA,
									RPAR, RBKT,
									EQ, NEQ, GT, GE, LT, LE)) {
					getsym_check();
				}
				type = VOID;
				box->certain = true;
				box->value = 0;
				break;
			}
			type = temp->type;
			getsym_check();
			box->name = new_temp();
			exp_type = expression(&index);
			if (exp_type == CHAR) {
				//WARNING:使用字符型变量取数
				sema_error("类型错误！");
			}
			//产生计算指令，作为数组下标进行取数操作
			if (symbol == RBKT) {
				//全部匹配，生成取数操作
				if (index.certain && index.value >= temp->paranum) {
					//error:数组越界
					sema_error("常量数组越界");
					getsym_check();
					type = VOID;
					break;
				}
				if (index.certain) {
					array_get_mid(temp->name, index.value, box->name);
				}
				else {
					array_get_mid(temp->name, index.name, box->name);
				}
				getsym_check();
				break;
			}
			else {
				//error: 右中括号不匹配，跳过
				gram_error("右中括号不匹配！");
				while (!check_legal(14, ADD, SUB, MUL, DIV, SEMI, COMMA,
									RPAR, RBKT,
									EQ, NEQ, GT, GE, LT, LE)) {
					getsym_check();
				}
				if (symbol == RBKT) {
					getsym_check();
				}
				type = VOID;
				break;
			}
		case LPAR://函数
			temp = findFunc(ident);
			if (!temp || temp->kind != FUNC) {
				//error:错误的函数名
				sema_error("函数未定义！");
				getsym_check();
				if (symbol == LPAR) {
					while (!check_legal(1, RPAR)) {
						getsym_check();
					}
				}
				while (!check_legal(14, ADD, SUB, MUL, DIV, SEMI, COMMA,
									RPAR, RBKT,
									EQ, NEQ, GT, GE, LT, LE)) {
					getsym_check();
				}
				box->certain = true;
				box->value = 0;
				type = VOID;
				getsym_check();
				break;
			}
			//检索符号表，得到参数个数，检查是否有返回值
			if (temp->type == VOID) {
				//error:表达式中不能出现void型函数调用
				sema_error("类型错误！");
				while (!check_legal(14, ADD, SUB, MUL, DIV, SEMI, COMMA,
									RPAR, RBKT,
									EQ, NEQ, GT, GE, LT, LE)) {
					getsym_check();
				}
				box->certain = true;
				box->value = 0;
				type = VOID;
				getsym_check();
				break;
			}
			type = temp->type;
			getsym_check();
			call(temp);
			//call内部产生函数调用动作
			box->name = new_temp();
			return_get_mid(box->name);
			break;
		default:
			temp = findSym(ident);
			if (!temp) {
				//error:错误的符号调用
				sema_error("标识符未定义！");
				while (!check_legal(14, ADD, SUB, MUL, DIV, SEMI, COMMA,
									RPAR, RBKT,
									EQ, NEQ, GT, GE, LT, LE)) {
					getsym_check();
				}
				box->certain = true;
				box->value = 0;
				type = VOID;
				break;
			}
			switch (temp->kind) {
			case PARA:
			case VAR://变量，目前处于标识符后面一个字符，加载temp对应的符号即可
				box->name = temp->name;
				type = temp->type;
				break;
			case CONST://常量按照立即数处理
				// 加载temp对应的立即数即可
				box->certain = true;
				box->value = temp->value;
				type = temp->type;
				break;
			}
		}
	}
	print_gra("This is a factor");
	return type;
}

Type term(Ret_item* box) {
	Type type = CHAR, ret_type;
	Ret_item fac_box;
	box->value = 1;
	box->certain = true;
	Symbol cur_op = MUL;
	bool need_new_on_cal = false;
	string new_name;
	do {
		ret_type = factor(&fac_box);
		type = ret_type == INT ? INT :
			(ret_type == VOID ? VOID : type);
		if (cur_op == DIV && fac_box.certain && fac_box.value == 0){
			//error("division by zero");
			sema_error("可探测的除零错误！");
			fac_box.value = 1;  // [ERROR HANDLE] set division by 1
		}
		if (box->certain && fac_box.certain) {
			switch (cur_op)
			{
			case MUL:
				box->value *= fac_box.value;
				break;
			case DIV:
				box->value /= fac_box.value;
				break;
			default:
				;//error_debug("item");
			}
		}
		else if (box->certain && !fac_box.certain){
			if (box->value == 1 && cur_op == MUL){
				box->name = fac_box.name;
				need_new_on_cal = true;
			}
			else{
				box->name = new_temp();
				assign_mid(box->name, box->value);
				cal_mid(cur_op, box->name, box->name, fac_box.name);
			}
		}
		else if (!box->certain && fac_box.certain){
			if (need_new_on_cal) {
				new_name = new_temp();
				cal_mid(cur_op, new_name, box->name, fac_box.value);
				box->name = new_name;
				need_new_on_cal = false;
			}
			else {
				cal_mid(cur_op, box->name, box->name, fac_box.value);
			}
		}
		else{
			if (need_new_on_cal) {
				new_name = new_temp();
				cal_mid(cur_op, new_name, box->name, fac_box.name);
				box->name = new_name;
				need_new_on_cal = false;
			}
			else {
				cal_mid(cur_op, box->name, box->name, fac_box.name);
			}
		}
		box->certain &= fac_box.certain;
		if (symbol == MUL || symbol == DIV){
			cur_op = symbol;
			type = type == VOID ? type:INT;
			getsym_check();
		}
		else { break; }
	} while (true);
	print_gra("This is a term");
	return type;
}


/*前后都进行预读
  TODO 处理表达式的返回值类型，done
  */
Type expression(Ret_item* box) {
	Type type = CHAR, ret_type;
	print_gra("expression start");
	Ret_item itm_box;
	Symbol cur_op = ADD;
	box->value = 0;
	box->certain = true;
	bool need_new_on_cal = false;
	string new_name;
	if (check_legal(2, ADD, SUB)) {
		if (symbol == SUB) {
			cur_op = SUB;
		}
		getsym_check();
	}
	do {
		ret_type = term(&itm_box);
		type = ret_type == INT ? INT :
			(ret_type == VOID ? VOID : type);
		if (box->certain && itm_box.certain) {
			switch (cur_op)
			{
			case ADD:
				box->value += itm_box.value;
				break;
			case SUB:
				box->value -= itm_box.value;
				break;
			default:
				;//error_debug("item");
			}
		}
		else if (box->certain && !itm_box.certain) {
			if (box->value == 0){
				switch (cur_op) {
				case ADD:
					box->name = itm_box.name;
					need_new_on_cal = true;
					break;
				case SUB:
					box->name = new_temp();
					cal_mid(cur_op, box->name, 0, itm_box.name);
				}
			}
			else{
				box->name = new_temp();
				assign_mid(box->name, box->value);
				cal_mid(cur_op, box->name, box->name, itm_box.name);
			}
		}
		else if (!box->certain && itm_box.certain) {
			if (need_new_on_cal) {
				new_name = new_temp();
				cal_mid(cur_op, new_name, box->name, itm_box.value);
				box->name = new_name;
				need_new_on_cal = false;
			}
			else {
				cal_mid(cur_op, box->name, box->name, itm_box.value);
			}
		}
		else {
			if (need_new_on_cal) {
				new_name = new_temp();
				cal_mid(cur_op, new_name, box->name, itm_box.name);
				box->name = new_name;
				need_new_on_cal = false;
			}
			else {
				cal_mid(cur_op, box->name, box->name, itm_box.name);
			}
		}
		box->certain &= itm_box.certain;
		if (symbol == ADD || symbol == SUB) {
			cur_op = symbol;
			type = type == VOID ? type : INT;
			getsym_check();
		}
		else { break; }
	} while (true);
	print_gra("This is a expression");
	return type;
}

State assign_or_call() {
	record_state();
	getsym_next();
	if (symbol == LPAR) {
		restore_state();
		return FUNC_CALL;
	}
	else if (symbol == LBKT) {
		restore_state();
		return ARR_ASSIGN;
	}
	else {
		restore_state();
		return VAR_ASSIGN;
	}
}

/*上层函数调用停留在标识符后面的等于号
  上层函数需要检索符号表并传入，在这一层使用变量的地址*/
void var_assignment(Sym *var) {
	Type exp_type;
	Ret_item exp;
	if (symbol == BECOME) {
	assign_cando:
		getsym_check();
	assign_value:
		exp_type = expression(&exp);
	}
	else {
		//error:应出现赋值符号
		gram_error("应出现赋值符号！");
		if (symbol == EQ) {
			goto assign_cando;
		}
		if (symbol == IDENT) {
			goto assign_value;
		}
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		return;
	}
	if (exp_type == VOID) {
		exp_type = var->type;
	}
	if (exp_type != var->type) {
		if (exp_type == INT && var->type == CHAR) {
			//error:不允许整型向char型转换
			sema_error("不允许整型向char型转换！");
		}
		else {
			//error:赋值类型不匹配
			sema_error("赋值类型不匹配！");
		}
	}
	//利用var，将表达式的结果进行赋值
	if (exp.certain) {
		assign_mid(var->name, exp.value);
	}
	else {
		assign_mid(var->name, exp.name);
	}
	print_gra("This is an assignment");
}

/*上层函数调用停留在标识符后面的中括号
  上层函数需要检索符号表并传入，在这一层使用数组的基地址*/
void arr_assignment(Sym *array) {
	Type exp_type;
	Ret_item exp;
	Ret_item index;
	assert(symbol == LBKT);
	getsym_check();			//能进来一定是中括号，直接跳过
	exp_type = expression(&index);
	if (exp_type == VOID) {
		exp_type = INT;
	}
	if (exp_type != INT) {
		//WARNING:使用字符型变量索引
		sema_error("类型错误！");
	}
	//生成加载数组索引的指令
	if (symbol != RBKT) {
		//error:中括号不匹配
		gram_error("中括号不匹配！");
		while (!check_legal(3, BECOME, RBKT, SEMI)) {
			getsym_check();
		}
		switch(symbol) {
		case(BECOME):
			goto become_value;
		case(SEMI):
			return;
		default:
			break;
		};
	}
	getsym_check();
	if (symbol != BECOME) {
		//error:应出现赋值符号
		gram_error("应出现赋值符号！");
		if (symbol == EQ) {
			goto become_value;
		}
		if (symbol == IDENT) {
			goto value;
		}
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		return;
	}
become_value:
	getsym_check();
value:
	exp_type = expression(&exp);
	if (exp_type == VOID) {
		exp_type = array->type;
	}
	if (exp_type != array->type) {
		if (exp_type == INT && array->type == CHAR) {
			//error:不允许整型向char型转换
			sema_error("出现整型向char转换！");
		}
		else {
			//error:赋值类型不匹配
			sema_error("类型不匹配！");
		}
	}
	//利用var，将表达式的结果进行赋值
	if (index.certain && index.value >= array->paranum) {
		sema_error("常量数组越界");//error: 数组常量越界
		return;
	}
	if (exp.certain && index.certain) {
		array_set_mid(array->name, index.value, exp.value);
	}
	else if (!exp.certain && index.certain) {
		array_set_mid(array->name, index.value, exp.name);
	}
	else if (exp.certain && !index.certain) {
		array_set_mid(array->name, index.name, exp.value);
	}
	else {
		array_set_mid(array->name, index.name, exp.name);
	}
	//将表达式结果执行赋值
	print_gra("This is an assignment");
}

void scanfstate() {
	Sym *temp;
	if (symbol != LPAR) {
		//scanf语句后没有小括号
		gram_error("缺少小括号！");
		goto skip;
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		return;
	}
	getsym_check();
	skip:
	while (symbol == IDENT) {
		temp = findSym(token);
		if (!temp || (temp->kind != VAR && temp->kind != PARA)) {
			//error:错误的变量名
			if (!temp) {
				sema_error("标识符未定义！");
			}
			else {
				sema_error("错误的变量名！");
			}
			while (!check_legal(2, SEMI, COMMA)) {
				getsym_check();
			}
			if (symbol == SEMI) {
				getsym_check();
				return;
			}
			else {
				getsym_check();
				continue;
			}
		}
		//生成：根据符号表中的地址生成syscall
		scanf_mid(temp->type, temp->name);
		getsym_check();
		if (symbol != COMMA) {
			break;
		}
		getsym_check();
	}
	if (symbol != RPAR) {
		//error:小括号不匹配
		gram_error("小括号不匹配！");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		return;
	}
	getsym_check();
	print_gra("This is a scanf statement");
}

void printfstate() {
	Type exp_type;
	Ret_item exp;
	if (symbol == LPAR) {
		getsym_check();
		switch (symbol) {
		case STRCON:
			//登录字符串常量表，操作在printf的动作指令中完成
			printf_mid(ARRAY, CHAR, token);
			getsym_check();
			if (symbol == COMMA) {
				getsym_check();
				exp_type = expression(&exp);
				//处理表达式输出，注意类型
				if (exp.certain) {
					printf_mid(CONST, exp_type, exp.value);
				}
				else {
					printf_mid(VAR, exp_type, exp.name);
				}
			}
			break;
		case ADD:
		case SUB:
		case IDENT: 
		case INTCON:
		case CHARCON:
		case LPAR:
			exp_type = expression(&exp);
			//处理表达式输出，注意类型
			if (exp.certain) {
				printf_mid(CONST, exp_type, exp.value);
			}
			else {
				printf_mid(VAR, exp_type, exp.name);
			}
			break;
		default:
			//error: 输出列表错误
			gram_error("输出列表错误！");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
			return;
		}
		if (symbol == RPAR) {
			getsym_check();
		}
		else {
			gram_error("小括号不匹配！");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
			return;
			//error:小括号不匹配
		}
	}
	else {
		//error:缺少小括号
		gram_error("缺少小括号！");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		return;
	}
	printf_mid();
	print_gra("This is an printf statement");
}

/*return语句一定在过程内部出现，通过全局变量的形式指定可否出现返回值*/
void returnstate() {
	Type exp_type;
	Ret_item exp;
	if (symbol == LPAR) {
		if (return_type == VOID) {
			//error:在void类型中返回参数
			sema_error("在void类型中返回参数！");
			//while (!check_legal(1, SEMI)) {
			//	getsym_check();
			//}
			//return;
		}
		getsym_check();
		exp_type = expression(&exp);
		if (return_type != VOID) {			//避免重复报错
			if (exp_type == VOID) {
				exp_type = return_type;
			}
			if (return_type != exp_type) {
				//error:返回值类型不匹配
				sema_error("返回值类型不匹配！");
			}
		}
		if (symbol == RPAR) {
			getsym_check();
		}
		else {
			//error:小括号不匹配
			gram_error("小括号不匹配！");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
			return;
		}
		if (exp.certain) {
			return_mid(exp.value);
		}
		else {
			return_mid(exp.name);
		}
	}
	else if(return_type != VOID){
		//return语句后没有小括号
		gram_error("缺少小括号！");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		return;
	}
	else {
		return_mid();
	}
	print_gra("This is a return statement");
}

void condition(Ret_item* cond) {
	Ret_item left;
	Ret_item right;
	Symbol cmp_op;
	Type exp_type;
	exp_type = expression(&left);
	if (exp_type == VOID) {
		exp_type = INT;
	}
	if (exp_type == CHAR) {
		//error:条件表达式不能使用字符型
		sema_error("条件表达式不能使用字符型！");
	}
	//TODO 条件表达式要检查参数， done
	if (check_legal(6, EQ, NEQ, GT, GE, LT, LE)) {
		cmp_op = symbol;
		getsym_check();
		exp_type = expression(&right);
		if (exp_type == VOID) {
			exp_type = INT;
		}
		if (exp_type == CHAR) {
			//error:条件表达式不能使用字符型
			sema_error("条件表达式不能使用字符型！");
		}
		//两个表达式
		if (left.certain && right.certain){   // left certain
			cond->certain = true;
			switch (cmp_op)
			{
			case GT:
				cond->value = (left.value > right.value);
				break;
			case GE:
				cond->value = (left.value >= right.value);
				break;
			case LT:
				cond->value = (left.value < right.value);
				break;
			case LE:
				cond->value = (left.value <= right.value);
				break;
			case EQ:
				cond->value = (left.value == right.value);
				break;
			case NEQ:
				cond->value = (left.value != right.value);
				break;
			default:
				;//error_debug("cond");
			}
		}
		else {
			if (left.certain && !right.certain){       // left uncertain
				cond->name = new_temp();
				cal_mid(cmp_op, cond->name, left.value, right.name);
			}
			else if (!left.certain && right.certain){
				cond->name = new_temp();
				cal_mid(cmp_op, cond->name, left.name, right.value);
			}
			else {
				cond->name = new_temp();
				cal_mid(cmp_op, cond->name, left.name, right.name);
			}
			cond->certain = false;
		}
	}
	else {//一个表达式
		cond->certain = left.certain;
		cond->name = left.name;
		cond->value = left.value;
	}
	print_gra("This is a condition");
}

void ifstate() {
	Ret_item cond;
	string over_label, else_label;
	this_func->return_check_if = false;
	this_func->return_check_else = false;
	over_label = new_label(this_func, "else_over", false);
	else_label = new_label(this_func, "else_begin", true);
	if (symbol == LPAR) {
		getsym_check();
		condition:
		condition(&cond);
		if (symbol != RPAR) {
			//error:缺少右括号
			while (!check_legal(3, RPAR, LBRC, SEMI)) {
				getsym_check();
			}
			if (symbol == SEMI) {
				gram_error("小括号不匹配！");
				getsym_check();
				goto checkelse;
			}
			else if (symbol == LBRC) {
				gram_error("小括号不匹配！");
				goto statements;
			}
		}
		getsym_check();
		statements:
		if (cond.certain && cond.value == 0){
			jump_mid(else_label);
		}
		else if (!cond.certain){
			branch_zero_mid(cond.name, else_label);
		}
		statement();
	}
	else {
		//error: if缺少条件
		if (check_legal(3, IDENT, INTCON, CHARCON)) {
			gram_error("缺少左括号！");
			goto condition;
		}
		else {
			gram_error("if缺少条件！");
		}
		while (!check_legal(6, SEMI, LBRC, IFSY, ELSY, DOSY, FORSY)) {
			getsym_check();
		}
		switch (symbol) {
		case SEMI:
			getsym_check();
		case ELSY:
			break;
		case IFSY:
			getsym_check();
			ifstate();
			break;
		case DOSY:
			getsym_check();
			dostate();
		case FORSY:
			getsym_check();
			forstate();
		}
	}
	checkelse:
	if (symbol == ELSY) {
		jump_mid(over_label);
		label_mid(else_label);
		getsym_check();
		if (this_func->return_check) {
			this_func->return_check_if = true;
		}
		statement();
		this_func->return_check_else = false;		//实际没有跳出，false
		this_func->return_check &= this_func->return_check_if;
		label_mid(over_label);
	}
	else {
		label_mid(else_label);
	}
	print_gra("This is an if statement");
}

/*for循环需要先进行初始化和判断跳转，最后进行递增、减并跳转到条件处
  需要生成标签，将递增、减指令后移到statement后面*/
void forstate() {
	Type exp_type;
	Sym *temp1, *temp2;
	Ret_item exp, cond;
	Symbol op;
	int step;

	stringstream code_storage;
	streampos condstart;

	string cond_label, end_label, state_label, step_label;
	cond_label = new_label(this_func, "for_cond", false);
	end_label = new_label(this_func, "for_end", false);
	state_label = new_label(this_func, "for_state", false);
	step_label = new_label(this_func, "for_step", true);
	if (symbol == LPAR) {
		getsym_check();
	}
	else {
		//error:缺少左括号
		gram_error("缺少左括号！");
	}
	if (symbol != IDENT) {
		//error:缺少标识符
		gram_error("缺少标识符！");
		goto jump2;
	}
	temp1 = findSym(token);
	if (!temp1 || (temp1->kind != VAR && temp1->kind != PARA)) {
		//error:标识符错误
		sema_error("标识符错误！");
		goto jump2;
	}
	getsym_check();
	if (symbol != BECOME) {
		//error:缺少等号
		gram_error("缺少等号！");
		goto jump2;
	}
	getsym_check();
	exp_type = expression(&exp);
	if (exp_type == VOID) {
		exp_type = temp1->type;
	}
	if (exp_type != temp1->type) {
		//error:赋值类型不匹配
		sema_error("赋值类型不匹配！");
	}
	if (symbol != SEMI) {
		//error:缺少分号
		gram_error("缺少分号！");
	}
	if (exp.certain) {
		assign_mid(temp1->name, exp.value);
	}
	else {
		assign_mid(temp1->name, exp.name);
	}
	//根据表达式结果生成赋值语句
	condstart = midfile.tellp();
	getsym_check();
stage2:
	condition(&cond);
	if (symbol != SEMI) {
		//error:缺少分号
		gram_error("缺少分号！");
	}
	if (cond.certain && cond.value == 0) {
		jump_mid(end_label);
	}
	else if (!cond.certain) {
		branch_zero_mid(cond.name, end_label);
	}
	// 复制条件表达式部分输出
	//streampos condend = midfile.tellp();
	midfile.seekg(condstart);
	code_storage << midfile.rdbuf();

	label_mid(cond_label);
	//输出到label之后
	midfile << code_storage.str();

	jump_mid(state_label);
	label_mid(step_label);
	getsym_check();
	if (symbol != IDENT) {
		//error:缺少标识符
		gram_error("缺少标识符！");
		goto jump3;
	}
	temp1 = findSym(token);
	if (!temp1 || (temp1->kind != VAR && temp1->kind != PARA)) {
		//error:标识符错误
		sema_error("标识符错误！");
		goto jump3;
	}
	if (temp1->type != INT) {
		//error：只能出现int型变量
		sema_error("只能出现int型变量！");
	}
	getsym_check();
	if (symbol != BECOME) {
		//error:缺少等号
		gram_error("缺少等号！");
		goto jump3;
	}
	getsym_check();
	if (symbol != IDENT) {
		//error:缺少标识符
		gram_error("缺少标识符！");
		goto jump3;
	}
	temp2 = findSym(token);
	if (!temp2 || (temp2->kind != VAR && temp2->kind != PARA)) {
		//error:标识符错误
		sema_error("标识符错误！");
		goto jump3;
	}
	getsym_check();
	if (!check_legal(2, ADD, SUB)) {
		//error:缺少等号
		gram_error("缺少等号！");
		goto jump3;
	}
	op = symbol;
	getsym_check();
	if (symbol != INTCON) {
		//error:缺少整数
		gram_error("缺少整数！");
		goto jump3;
	}
	step = num;
	//根据temp1和temp2生成指令
	getsym_check();
	if (symbol != RPAR) {
		//error:缺少分号
		gram_error("缺少分号！");
		goto jump3;
	}
	cal_mid(op, temp1->name, temp2->name, step);
	jump_mid(cond_label);
	getsym_check();
statm:
	label_mid(state_label);
	statement();
	jump_mid(step_label);
	goto end;
jump2:
	while (!check_legal(2, SEMI, RPAR)) {
		getsym_check();
	}
	switch (symbol) {
	case SEMI:
		getsym_check();
		goto stage2;
	case RPAR:
		getsym_check();
		goto statm;
	}
jump3:
	while (!check_legal(1, RPAR)) {
		getsym_check();
	}
	getsym_check();
	goto statm;
end:
	label_mid(end_label);
	//产生递增、减指令并处理出口标签
	print_gra("This is a for statement");
}

void dostate() {
	Ret_item cond;
	string begin_label, end_label;
	begin_label = new_label(this_func, "dobegin", false);
	end_label = new_label(this_func, "doend", true);
	//入口处产生标签
	label_mid(begin_label);
	statement();
	if (symbol != WHILESY) {
		//缺少while
		gram_error("缺少while！");
		if (!readline()) {
			//error("unfinished program");
			gram_error("unfinished program！");
			exit(0);
		}
	}
	getsym_check();
	if (symbol != LPAR) {
		//缺少左括号
		gram_error("缺少左括号！");
		if (!readline()) {
			//error("unfinished program");
			gram_error("unfinished program！");
			exit(0);
		}
	}
	getsym_check();
	condition(&cond);
	if (symbol != RPAR) {
		//缺少右括号
		gram_error("缺少右括号！");
		if (!readline()) {
			//error("unfinished program");
			gram_error("unfinished program！");
			exit(0);
		}
	}
	//根据condition结果产生跳转指令
	if (!cond.certain) {
		branch_zero_mid(cond.name, end_label);
		jump_mid(begin_label);
	}
	else {
		if (cond.value) {
			jump_mid(begin_label);
		}
	}
	label_mid(end_label);
	print_gra("This is a do while statement");
	getsym_check();
}

void statementlist() {
	while (symbol != RBRC) {
		statement();
	}
	print_gra("This is a statement list");
}

void statement() {
	temp_counts.push_back(temp_counts.back());
	State state;
	Sym *temp;
	int para_num = 0;
	this_func->return_check = false;
	this_func->return_check_else = true;		//用来标志已经跳出if循环
	switch (symbol) {
	case IFSY:
		getsym_check();
		ifstate();
		break;
	case DOSY:
		getsym_check();
		dostate();
		break;
	case FORSY:
		getsym_check();
		forstate();
		break;
	case LBRC:
		getsym_check();
		statementlist();
		if (symbol != RBRC) {
			//error:缺少右括号
			gram_error("缺少右括号！");
		}
		else {
			getsym_check();
		}
		break;
	case IDENT:
		//查符号表检索
		state = assign_or_call();
		switch (state) {
		case FUNC_CALL:
			temp = findFunc(token);
			if (!temp || temp->kind != FUNC) {
				//error:错误的函数调用
				sema_error("错误的函数调用！");
				while (!check_legal(1, SEMI)) {
					getsym_check();
				}
				break;
			}
			getsym_check();
			assert(symbol == LPAR);
			getsym_check();
			call(temp);
			break;
		case ARR_ASSIGN:
			temp = findSym(token);
			if (!temp || temp->kind != ARRAY) {
				//error:错误的数组调用
				sema_error("错误的数组调用！");
				while (!check_legal(1, SEMI)) {
					getsym_check();
				}
				break;
			}
			getsym_check();
			arr_assignment(temp);
			break;
		case VAR_ASSIGN:
			temp = findSym(token);
			if (!temp || (temp->kind != VAR && temp->kind != PARA)) {
				//error:错误的变量调用
				sema_error("错误的变量调用！");
				while (!check_legal(1, SEMI)) {
					getsym_check();
				}
				break;
			}
			getsym_check();
			var_assignment(temp);
			break;
		}
		goto check_semi;
	case SCANFSY:
		getsym_check();
		scanfstate();
		goto check_semi;
	case PRINTFSY:
		getsym_check();
		printfstate();
		goto check_semi;
	case RTNSY:
		getsym_check();
		returnstate();
		this_func->return_check = true;
		goto check_semi;
	default:
		if (symbol != SEMI) {
			gram_error("语句错误！（潜在的声明位置问题）");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
		}
	check_semi:
		if (symbol != SEMI) {
			//error:缺少分号
			gram_error("缺少分号！");
			//while (!check_legal(1, SEMI)) {
			//	getsym_check();
			//}
		}
		else {
			getsym_check();
		}
	}
	//print_gra("This is a statement");
	temp_max = max(*max_element(temp_counts.begin(), temp_counts.end()), temp_max);
	temp_counts.pop_back();
}

void composed_state() {
	init_temp();
	if (symbol == CONSTSY) {
		const_declare();
	}
	while (check_legal(2, CHARSY, INTSY)) {
		var_declare();
	}
	branch = 0;
	statementlist();
	print_gra("This is a composed statement");
}

void main_state() {
	Sym *temp;
	int stack_size;
	temp_max = 0;
	temp = func_head();
	this_func = temp;
	if (!temp) {
		//error:符号表插入错误
		sema_error("符号表插入错误！");
		exit(0);
	}
	declare_func_mid(temp);	//函数声明中间代码
	if (symbol != LPAR) {
		//error:应出现左括号
		gram_error("应出现左括号！");
	}
	if (symbol != RPAR || symbol != LPAR)
		getsym_check();
	if (symbol != RPAR) {
		//error:应出现右括号
		gram_error("应出现右括号！");
	}
	if (symbol != LBRC)
		getsym_check();
	if (symbol != LBRC) {
		//error:应出现左大括号
		gram_error("应出现左大括号！");
	}
	getsym_check();
	composed_state();
	if (symbol != RBRC) {
		//error:应出现右大括号
		gram_error("应出现右大括号！");
	}
	return_mid();
	stack_size = ClearSymTable();	//出口处统计局部变量需要的栈大小
	temp->vsize = stack_size;
	temp->psize = stack_size + 4 * temp_max;
	print_gra("This is a main function");
}

void program() {
	bool temp_flag = false;
	getsym_check();
	if (symbol == CONSTSY) {
		const_declare();
	}
	if (check_legal(3, CHARSY, INTSY, VOIDSY)) {
		while (VAR_STATE == is_var_declare()) {
			var_declare();
		}
		call_func_mid("main");
		exit_mid();
		temp_counts.push_back(0);
		check_again:
		while (FUNC_STATE == is_var_declare()) {
			func_declare();
		}
		//printf("\nentering main\n");
		while (MAIN_STATE != is_var_declare()) {
			//error：错误的定义位置，跳过
			getsym_check();
			if (!temp_flag) {
				gram_error("错误的定义位置！");
				temp_flag = true;
			}
			while (!check_legal(3, VOIDSY,INTSY, CHARSY)) {
				getsym_check();
			}
			goto check_again;
		}
		main_state();
	}
	print_gra("This is a program");
}