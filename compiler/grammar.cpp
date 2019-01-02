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
extern int line_index;			//��ҪԤ���ַ��ж�ʱ�����ָ����е���
								//Ԥ��ֻ�ڵ�ǰ�н���
extern char line_buffer[LINE_MAX_LENGTH];
extern fstream midfile;

/*���ļ���ʹ�õ�ȫ�ֱ���*/
#define print_gram	false
bool mark = false;		//����Ϊ��ȡ��һ�����õı��
Type return_type = VOID;
Sym *this_func;
int temp_max;
bool return_last = false;
int error_sym = 0;

/*����ΪԤ�����õ���ʱ�洢*/
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

/*ֱ�ӵ�������λ��token��num*/
void signed_int() {
	switch (symbol) {
	case ADD:
		getsym_check();
		if (symbol != INTCON) {
			gram_error("����ƥ�����");
		}
		break;
	case SUB:
		getsym_check();
		if (symbol != INTCON) {
			gram_error("����ƥ�����");
		}
		num = -num;
		break;
	case INTCON:
		break;
	default:
		gram_error("����ƥ�����");
	}
	print_gra("This is a int");
}

/*ѭ������������const*/
void const_declare() {
	int const_num;
	Sym *temp = NULL;
	while (symbol == CONSTSY) {
		getsym_check();
		switch (symbol) {
		case INTSY:
			getsym_check();
			while (check_legal(1, IDENT)) {
				//insymbol()������ű����ز���λ��
				temp = pushTable(token, CONST, INT);
				getsym_check();
				if (symbol == BECOME) {
					getsym_check();
					if (check_legal(4, ADD, SUB, INTCON)) {
						signed_int();
						const_num = num;
						//editsymbol()��ֵ������ű�
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
							gram_error("ȱʧ�ֺţ�");
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
						gram_error("�Ƿ��ĵ�ʽ��ֵ��");
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
				gram_error("ȱʧ�ֺţ�");
				while (!check_legal(1, SEMI)) {
					getsym_check();
				}
				getsym_check();
				break;
			}
		case CHARSY:
			getsym_check();
			while (check_legal(1, IDENT)) {
				//insymbol()������ű����ز���λ��
				temp = pushTable(token, CONST, CHAR);
				getsym_check();
				if (symbol == BECOME) {
					getsym_check();
					if (symbol == CHARCON) {
						const_num = num;
						//editsymbol()��ֵ������ű�
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
							gram_error("ȱʧ�ֺţ�");
							while (!check_legal(1, SEMI)) {
								getsym_check();
							}
							getsym_check();
							break;
							//error()ȱʧ�ָ���
						}
						else {//this is for semi
							break;
						}
					}
					else if (symbol == IDENT) {
						//error():
						const_num = 0;
						gram_error("ȱʧ�ֺţ�");
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
				gram_error("ȱʧ�ֺţ�");
				while (!check_legal(1, SEMI)) {
					getsym_check();
				}
				getsym_check();
				break;
				//�Ӳ������ĳ����������
			}
		default:
			gram_error("�Ƿ��ĳ���������");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
			getsym_check();
			break;
			//error() ���Ϸ������ͱ�ʶ��
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

State is_var_declare() {	//����֮ǰ��⵽���͹ؼ���
	again:
	record_state();
	bool next_mark = mark;
	if (!next_mark) {
		gram_error("����ı�������������");
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
		gram_error("ȱʧ��ʶ����");
		while (!check_legal(3, VOIDSY, CHARSY, INTSY)) {
			getsym_check();
		}
		goto again;
		//error:���͹ؼ���֮���Ǳ�ʶ��
		//restore_state();
		//return NOSTATE;
	}
	if (!next_mark) {
		gram_error("ȱʧ��ʶ����");
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

/* ����֮ǰ��⵽���͹ؼ��֣���ȷ���Ǳ�������
   �˺���һ��ֻʶ��һ��������䣨�ֺŽ�����*/
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
		gram_error("ȱʧ��ʶ����");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		getsym_check();
		//error:Ӧ���ֱ�ʶ��
		//skip:��������������
		return;
	}
	while (symbol == IDENT) {	
		kind = var_or_array();
		if (kind != ARRAY) {
			//insymbol(kind, type)������ű����ز���λ��
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
			//insymbol(kind, type, array_length)������ű����ز���λ��
			temp = pushTable(token, ARRAY, type);
			getsym_check();		//�������һ����������
			assert(symbol == LBKT);
			getsym_check();
			if (symbol != INTCON) {
				gram_error("Ӧ����������");
				while (!check_legal(1, RBKT)) {
					getsym_check();
				}
				array_length = 1;
				goto add_length;
			}
			else {
				array_length = num;
				if (array_length <= 0) {
					gram_error("���鳤��Ӧ�ô����㣡");
					//error: the leng of the array should greater than 0
					array_length = 1;
				}
				//�޸ķ��ű�
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
					//error:ȱʧ������
					gram_error("ȱʧ�����ţ�");
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
		/*�����м����*/
		if (!temp) {
			gram_error("��������");
		}
		else {
			declare_var_mid(temp);
		}
	}//while (symbol == IDENT);��ǰ����������
	if (symbol != SEMI) {
		gram_error("ȱʧ�ֺţ�");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		getsym_check();
		//error:ȱʧ�ֺ�
	}
	else {
		print_gra("This is a var declare");
		getsym_check();
	}
	
}

/*������������ͷ����Ҫ���ط��ű�ָ�룬ƥ��������б�֮��������*/
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
		gram_error("ȱʧ�������ƣ�");
		while (!check_legal(1, LPAR)) {
			getsym_check();
		}
		string tempstr = "#unnamefunc" + to_string(error_sym++);
		temp = pushTable((char*)tempstr.data(), FUNC, type);
		getsym_check();
		//error:Ӧ�����ʶ��
		return temp;
	}
	//insymbol(FUNC, type)������ű����ز���λ��
	//��ʼ��������ָ��ΪNULL����������Ϊ0
	temp = pushTable(token, FUNC, type);
	getsym_check();
	print_gra("This is a func head");
	return temp;
}


/*���Ҫ�����������ţ�����ǰ�ַ�ͣ�������͹ؼ�����
  �����б�Ϊ����ӦΪ������
  ��������������ű��������������Լ���������ָ�룬done*/
void para_list(Sym *func) {
	Sym *temp = NULL;
	if (symbol == RPAR) {
		getsym_check();
		return;
	}
	//�����������
	func->value = SymTable.index;
	int para_num = 0;
	Type type;
	while (symbol == INTSY || symbol == CHARSY) {
		switch (symbol) {
		case INTSY:
			getsym_check();
			if (symbol != IDENT) {
				gram_error("ȱ�ٲ�������");
				//error:ȱ�ٱ�ʶ��
				while (!check_legal(1, RPAR)) {
					getsym_check();
				}
				goto para_end;
			}
			//����ǰ�����������������int
			type = INT;
			temp = pushTable(token, PARA, type);
			if (!temp) {
				gram_error("fital:���ű����ʧ�ܣ�");
				//error:���ű����ʧ��
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
			//����ǰ�����������������char
			type = CHAR;
			temp = pushTable(token, PARA, type);
			if (!temp) {
				goto no_mid;
				//error:���ű����ʧ��
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
		
	}//while;��������ѭ��
	if (symbol != RPAR) {
		gram_error("ȱ�������ţ�");
		//error:ȱ��������
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
	declare_func_mid(temp);	//���������м����
	if (symbol == LPAR) {
		getsym_check();
		para_list(temp);
	}
	else {
		//error:û�в����б�
		gram_error("ȱ���Ҵ����ţ�");
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
		//error:û���������
		gram_error("ȱ��������ţ�");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		getsym_check();
	}
func_body:
	composed_state();
	if (symbol != RBRC) {
		//error:û���Ҵ�����
		gram_error("ȱ���Ҵ����ţ�");
		while (!check_legal(1, RBRC)) {
			getsym_check();
		}
	}
	if (this_func->type != VOID) {
		if (!this_func->return_check) {
			if (!this_func->return_check_else) {		//�����if���
				sema_error(string("���� ") + this_func->name + string(" ��������·�����з���ֵ��"));
			}
			else{		//���һ�䲻��if
				sema_error(string("���� ") + this_func->name + string(" ȱ�ٷ���ֵ��"));
			}
		}
	}
	else {
		if (!this_func->return_check) {
			return_mid();
		}
	}
	stack_size = ClearSymTable();	//���ڴ�ͳ�ƾֲ�������Ҫ��ջ��С
	temp->vsize = stack_size;
	temp->psize = stack_size + 4 * temp_max;
	print_gra("This is a func declare\n");
	getsym_check();
}

/*��㺯������������ű�ȷ����������ֵ�Ƿ����Ҫ��
  �������Ϊ�������βθ�������ڷ���ͣ����������֮��
  ��������������ݹ��̣���������ȫ���м����*/
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
			if (num > para_num) {//����������������
				exceed_flag = true;
			}
			goto cont;
		}
		//���ʽҪ��������
		if (num <= para_num) {//���������Ϸ�
			para_type = SymTable.element[para_base + num].type;
			if (exp_type != para_type) {
				//error:���Ͳ�ƥ��
				sema_error("����" + string(func->name) + "�ĵ�" + to_string(num+1) + "���������Ͳ�ƥ��!");
			}
			if (para.certain) {//��������
				push_mid(para.value);
			}
			else {//���ݱ���
				push_mid(para.name);
			}
		}
		else {//����������������
			exceed_flag = true;
		}
	cont:
		num++;
		if (symbol != COMMA) {
			break;
		}
		getsym_check();
	}//while;ʵ��ѭ��
	if (num < para_num) {
		//error:������������
		sema_error("����" + string(func->name) + "�����������㣡");
	}
	if (exceed_flag) {
		//error:����������������
		sema_error("����" + string(func->name) + "���������������ƣ�");
	}
	if (symbol != RPAR) {
		//error:С���Ų�ƥ��
		gram_error("ȱ����С���ţ�");
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

/*Ҫ���ϲ㺯��Ԥ����һ���ַ�*/
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
			//error:�����Ų�ƥ��
			gram_error("ȱ����С���ţ�");
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
		//�������ű�����ָ��
		strcpy_s(ident, token);
		getsym_check();
		switch (symbol) {
		case LBKT://����
			temp = findSym(ident);
			if (!temp || temp->kind != ARRAY) {
				//error:�����������
				if (temp->kind != ARRAY) {
					sema_error("�������������");
				}
				else if (!temp) {
					sema_error("��ʶ��δ���壡");
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
				//WARNING:ʹ���ַ��ͱ���ȡ��
				sema_error("���ʹ���");
			}
			//��������ָ���Ϊ�����±����ȡ������
			if (symbol == RBKT) {
				//ȫ��ƥ�䣬����ȡ������
				if (index.certain && index.value >= temp->paranum) {
					//error:����Խ��
					sema_error("��������Խ��");
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
				//error: �������Ų�ƥ�䣬����
				gram_error("�������Ų�ƥ�䣡");
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
		case LPAR://����
			temp = findFunc(ident);
			if (!temp || temp->kind != FUNC) {
				//error:����ĺ�����
				sema_error("����δ���壡");
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
			//�������ű��õ���������������Ƿ��з���ֵ
			if (temp->type == VOID) {
				//error:���ʽ�в��ܳ���void�ͺ�������
				sema_error("���ʹ���");
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
			//call�ڲ������������ö���
			box->name = new_temp();
			return_get_mid(box->name);
			break;
		default:
			temp = findSym(ident);
			if (!temp) {
				//error:����ķ��ŵ���
				sema_error("��ʶ��δ���壡");
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
			case VAR://������Ŀǰ���ڱ�ʶ������һ���ַ�������temp��Ӧ�ķ��ż���
				box->name = temp->name;
				type = temp->type;
				break;
			case CONST://������������������
				// ����temp��Ӧ������������
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
			sema_error("��̽��ĳ������");
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


/*ǰ�󶼽���Ԥ��
  TODO ������ʽ�ķ���ֵ���ͣ�done
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

/*�ϲ㺯������ͣ���ڱ�ʶ������ĵ��ں�
  �ϲ㺯����Ҫ�������ű����룬����һ��ʹ�ñ����ĵ�ַ*/
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
		//error:Ӧ���ָ�ֵ����
		gram_error("Ӧ���ָ�ֵ���ţ�");
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
			//error:������������char��ת��
			sema_error("������������char��ת����");
		}
		else {
			//error:��ֵ���Ͳ�ƥ��
			sema_error("��ֵ���Ͳ�ƥ�䣡");
		}
	}
	//����var�������ʽ�Ľ�����и�ֵ
	if (exp.certain) {
		assign_mid(var->name, exp.value);
	}
	else {
		assign_mid(var->name, exp.name);
	}
	print_gra("This is an assignment");
}

/*�ϲ㺯������ͣ���ڱ�ʶ�������������
  �ϲ㺯����Ҫ�������ű����룬����һ��ʹ������Ļ���ַ*/
void arr_assignment(Sym *array) {
	Type exp_type;
	Ret_item exp;
	Ret_item index;
	assert(symbol == LBKT);
	getsym_check();			//�ܽ���һ���������ţ�ֱ������
	exp_type = expression(&index);
	if (exp_type == VOID) {
		exp_type = INT;
	}
	if (exp_type != INT) {
		//WARNING:ʹ���ַ��ͱ�������
		sema_error("���ʹ���");
	}
	//���ɼ�������������ָ��
	if (symbol != RBKT) {
		//error:�����Ų�ƥ��
		gram_error("�����Ų�ƥ�䣡");
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
		//error:Ӧ���ָ�ֵ����
		gram_error("Ӧ���ָ�ֵ���ţ�");
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
			//error:������������char��ת��
			sema_error("����������charת����");
		}
		else {
			//error:��ֵ���Ͳ�ƥ��
			sema_error("���Ͳ�ƥ�䣡");
		}
	}
	//����var�������ʽ�Ľ�����и�ֵ
	if (index.certain && index.value >= array->paranum) {
		sema_error("��������Խ��");//error: ���鳣��Խ��
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
	//�����ʽ���ִ�и�ֵ
	print_gra("This is an assignment");
}

void scanfstate() {
	Sym *temp;
	if (symbol != LPAR) {
		//scanf����û��С����
		gram_error("ȱ��С���ţ�");
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
			//error:����ı�����
			if (!temp) {
				sema_error("��ʶ��δ���壡");
			}
			else {
				sema_error("����ı�������");
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
		//���ɣ����ݷ��ű��еĵ�ַ����syscall
		scanf_mid(temp->type, temp->name);
		getsym_check();
		if (symbol != COMMA) {
			break;
		}
		getsym_check();
	}
	if (symbol != RPAR) {
		//error:С���Ų�ƥ��
		gram_error("С���Ų�ƥ�䣡");
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
			//��¼�ַ���������������printf�Ķ���ָ�������
			printf_mid(ARRAY, CHAR, token);
			getsym_check();
			if (symbol == COMMA) {
				getsym_check();
				exp_type = expression(&exp);
				//������ʽ�����ע������
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
			//������ʽ�����ע������
			if (exp.certain) {
				printf_mid(CONST, exp_type, exp.value);
			}
			else {
				printf_mid(VAR, exp_type, exp.name);
			}
			break;
		default:
			//error: ����б����
			gram_error("����б����");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
			return;
		}
		if (symbol == RPAR) {
			getsym_check();
		}
		else {
			gram_error("С���Ų�ƥ�䣡");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
			return;
			//error:С���Ų�ƥ��
		}
	}
	else {
		//error:ȱ��С����
		gram_error("ȱ��С���ţ�");
		while (!check_legal(1, SEMI)) {
			getsym_check();
		}
		return;
	}
	printf_mid();
	print_gra("This is an printf statement");
}

/*return���һ���ڹ����ڲ����֣�ͨ��ȫ�ֱ�������ʽָ���ɷ���ַ���ֵ*/
void returnstate() {
	Type exp_type;
	Ret_item exp;
	if (symbol == LPAR) {
		if (return_type == VOID) {
			//error:��void�����з��ز���
			sema_error("��void�����з��ز�����");
			//while (!check_legal(1, SEMI)) {
			//	getsym_check();
			//}
			//return;
		}
		getsym_check();
		exp_type = expression(&exp);
		if (return_type != VOID) {			//�����ظ�����
			if (exp_type == VOID) {
				exp_type = return_type;
			}
			if (return_type != exp_type) {
				//error:����ֵ���Ͳ�ƥ��
				sema_error("����ֵ���Ͳ�ƥ�䣡");
			}
		}
		if (symbol == RPAR) {
			getsym_check();
		}
		else {
			//error:С���Ų�ƥ��
			gram_error("С���Ų�ƥ�䣡");
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
		//return����û��С����
		gram_error("ȱ��С���ţ�");
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
		//error:�������ʽ����ʹ���ַ���
		sema_error("�������ʽ����ʹ���ַ��ͣ�");
	}
	//TODO �������ʽҪ�������� done
	if (check_legal(6, EQ, NEQ, GT, GE, LT, LE)) {
		cmp_op = symbol;
		getsym_check();
		exp_type = expression(&right);
		if (exp_type == VOID) {
			exp_type = INT;
		}
		if (exp_type == CHAR) {
			//error:�������ʽ����ʹ���ַ���
			sema_error("�������ʽ����ʹ���ַ��ͣ�");
		}
		//�������ʽ
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
	else {//һ�����ʽ
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
			//error:ȱ��������
			while (!check_legal(3, RPAR, LBRC, SEMI)) {
				getsym_check();
			}
			if (symbol == SEMI) {
				gram_error("С���Ų�ƥ�䣡");
				getsym_check();
				goto checkelse;
			}
			else if (symbol == LBRC) {
				gram_error("С���Ų�ƥ�䣡");
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
		//error: ifȱ������
		if (check_legal(3, IDENT, INTCON, CHARCON)) {
			gram_error("ȱ�������ţ�");
			goto condition;
		}
		else {
			gram_error("ifȱ��������");
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
		this_func->return_check_else = false;		//ʵ��û��������false
		this_func->return_check &= this_func->return_check_if;
		label_mid(over_label);
	}
	else {
		label_mid(else_label);
	}
	print_gra("This is an if statement");
}

/*forѭ����Ҫ�Ƚ��г�ʼ�����ж���ת�������е�����������ת��������
  ��Ҫ���ɱ�ǩ������������ָ����Ƶ�statement����*/
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
		//error:ȱ��������
		gram_error("ȱ�������ţ�");
	}
	if (symbol != IDENT) {
		//error:ȱ�ٱ�ʶ��
		gram_error("ȱ�ٱ�ʶ����");
		goto jump2;
	}
	temp1 = findSym(token);
	if (!temp1 || (temp1->kind != VAR && temp1->kind != PARA)) {
		//error:��ʶ������
		sema_error("��ʶ������");
		goto jump2;
	}
	getsym_check();
	if (symbol != BECOME) {
		//error:ȱ�ٵȺ�
		gram_error("ȱ�ٵȺţ�");
		goto jump2;
	}
	getsym_check();
	exp_type = expression(&exp);
	if (exp_type == VOID) {
		exp_type = temp1->type;
	}
	if (exp_type != temp1->type) {
		//error:��ֵ���Ͳ�ƥ��
		sema_error("��ֵ���Ͳ�ƥ�䣡");
	}
	if (symbol != SEMI) {
		//error:ȱ�ٷֺ�
		gram_error("ȱ�ٷֺţ�");
	}
	if (exp.certain) {
		assign_mid(temp1->name, exp.value);
	}
	else {
		assign_mid(temp1->name, exp.name);
	}
	//���ݱ��ʽ������ɸ�ֵ���
	condstart = midfile.tellp();
	getsym_check();
stage2:
	condition(&cond);
	if (symbol != SEMI) {
		//error:ȱ�ٷֺ�
		gram_error("ȱ�ٷֺţ�");
	}
	if (cond.certain && cond.value == 0) {
		jump_mid(end_label);
	}
	else if (!cond.certain) {
		branch_zero_mid(cond.name, end_label);
	}
	// �����������ʽ�������
	//streampos condend = midfile.tellp();
	midfile.seekg(condstart);
	code_storage << midfile.rdbuf();

	label_mid(cond_label);
	//�����label֮��
	midfile << code_storage.str();

	jump_mid(state_label);
	label_mid(step_label);
	getsym_check();
	if (symbol != IDENT) {
		//error:ȱ�ٱ�ʶ��
		gram_error("ȱ�ٱ�ʶ����");
		goto jump3;
	}
	temp1 = findSym(token);
	if (!temp1 || (temp1->kind != VAR && temp1->kind != PARA)) {
		//error:��ʶ������
		sema_error("��ʶ������");
		goto jump3;
	}
	if (temp1->type != INT) {
		//error��ֻ�ܳ���int�ͱ���
		sema_error("ֻ�ܳ���int�ͱ�����");
	}
	getsym_check();
	if (symbol != BECOME) {
		//error:ȱ�ٵȺ�
		gram_error("ȱ�ٵȺţ�");
		goto jump3;
	}
	getsym_check();
	if (symbol != IDENT) {
		//error:ȱ�ٱ�ʶ��
		gram_error("ȱ�ٱ�ʶ����");
		goto jump3;
	}
	temp2 = findSym(token);
	if (!temp2 || (temp2->kind != VAR && temp2->kind != PARA)) {
		//error:��ʶ������
		sema_error("��ʶ������");
		goto jump3;
	}
	getsym_check();
	if (!check_legal(2, ADD, SUB)) {
		//error:ȱ�ٵȺ�
		gram_error("ȱ�ٵȺţ�");
		goto jump3;
	}
	op = symbol;
	getsym_check();
	if (symbol != INTCON) {
		//error:ȱ������
		gram_error("ȱ��������");
		goto jump3;
	}
	step = num;
	//����temp1��temp2����ָ��
	getsym_check();
	if (symbol != RPAR) {
		//error:ȱ�ٷֺ�
		gram_error("ȱ�ٷֺţ�");
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
	//������������ָ�������ڱ�ǩ
	print_gra("This is a for statement");
}

void dostate() {
	Ret_item cond;
	string begin_label, end_label;
	begin_label = new_label(this_func, "dobegin", false);
	end_label = new_label(this_func, "doend", true);
	//��ڴ�������ǩ
	label_mid(begin_label);
	statement();
	if (symbol != WHILESY) {
		//ȱ��while
		gram_error("ȱ��while��");
		if (!readline()) {
			//error("unfinished program");
			gram_error("unfinished program��");
			exit(0);
		}
	}
	getsym_check();
	if (symbol != LPAR) {
		//ȱ��������
		gram_error("ȱ�������ţ�");
		if (!readline()) {
			//error("unfinished program");
			gram_error("unfinished program��");
			exit(0);
		}
	}
	getsym_check();
	condition(&cond);
	if (symbol != RPAR) {
		//ȱ��������
		gram_error("ȱ�������ţ�");
		if (!readline()) {
			//error("unfinished program");
			gram_error("unfinished program��");
			exit(0);
		}
	}
	//����condition���������תָ��
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
	this_func->return_check_else = true;		//������־�Ѿ�����ifѭ��
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
			//error:ȱ��������
			gram_error("ȱ�������ţ�");
		}
		else {
			getsym_check();
		}
		break;
	case IDENT:
		//����ű����
		state = assign_or_call();
		switch (state) {
		case FUNC_CALL:
			temp = findFunc(token);
			if (!temp || temp->kind != FUNC) {
				//error:����ĺ�������
				sema_error("����ĺ������ã�");
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
				//error:������������
				sema_error("�����������ã�");
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
				//error:����ı�������
				sema_error("����ı������ã�");
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
			gram_error("�����󣡣�Ǳ�ڵ�����λ�����⣩");
			while (!check_legal(1, SEMI)) {
				getsym_check();
			}
		}
	check_semi:
		if (symbol != SEMI) {
			//error:ȱ�ٷֺ�
			gram_error("ȱ�ٷֺţ�");
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
		//error:���ű�������
		sema_error("���ű�������");
		exit(0);
	}
	declare_func_mid(temp);	//���������м����
	if (symbol != LPAR) {
		//error:Ӧ����������
		gram_error("Ӧ���������ţ�");
	}
	if (symbol != RPAR || symbol != LPAR)
		getsym_check();
	if (symbol != RPAR) {
		//error:Ӧ����������
		gram_error("Ӧ���������ţ�");
	}
	if (symbol != LBRC)
		getsym_check();
	if (symbol != LBRC) {
		//error:Ӧ�����������
		gram_error("Ӧ����������ţ�");
	}
	getsym_check();
	composed_state();
	if (symbol != RBRC) {
		//error:Ӧ�����Ҵ�����
		gram_error("Ӧ�����Ҵ����ţ�");
	}
	return_mid();
	stack_size = ClearSymTable();	//���ڴ�ͳ�ƾֲ�������Ҫ��ջ��С
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
			//error������Ķ���λ�ã�����
			getsym_check();
			if (!temp_flag) {
				gram_error("����Ķ���λ�ã�");
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