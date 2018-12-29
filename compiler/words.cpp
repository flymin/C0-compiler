#include<iostream>
#include<fstream>
#include<string>
#include "var.h"
#include "const.h"

using namespace std;

extern char token[TOKEN_MAX_LENTH];	// store sign
extern int token_len;
extern int num;								// current integer
extern char cur_c;								// current char to read
extern ifstream progfile;
extern ofstream resfile;
extern fstream midfile;
extern Symbol symbol;
extern bool success;

int line_index;						//line_buffer位置指针
int sym_num = 0;					//词法分析输出用，当前输出计数
char line_buffer[LINE_MAX_LENGTH] = { 0 };		//作为当前行的缓存buffer
int line_no = 0;					//用来统计当前读取的程序行数


void error(string info)
{
	success = false;
	cout << line_buffer << endl;
	cout << "[WORD ERROR] " << info << " in " << line_no << endl;
}

void token_init() {
	token_len = 0;
	token[token_len] = 0;
}

void cat_token() {
	token[token_len++] = cur_c;
}

bool readline() {
	if (progfile.eof())
		return false;
	else {
		progfile.getline(line_buffer, 256, '\n');
		line_no++;
		line_index = 0;
		cur_c = '\0';
		//cout << line_no << ":" << line_buffer << endl;
		//midfile << line_buffer << endl;
		return true;
	}
}

bool nextchar(){		//从当前行中读取下一个字符
	if (line_index > strlen(line_buffer)) {
		return false;
	}
	cur_c = line_buffer[line_index++];
	return true;
}

bool lastchar() {		//从当前行中读取下一个字符
	if (line_index <= 0) {
		return false;
	}
	cur_c = line_buffer[--line_index];
	return true;
}

bool need_skip() {
	if (cur_c == ' ' || cur_c == '\t' || cur_c == '\0')
		return true;
	else
		return false;
}

bool letter_in_ident() {
	if ((cur_c >= 'A' && cur_c <= 'Z') ||
		(cur_c >= 'a' && cur_c <= 'z') ||
		(cur_c == '_'))
		return true;
	else
		return false;
}

bool is_string_char() {
	//32, 33, 35 - 126
	if (
		cur_c == 32 || cur_c == 33 ||
		(cur_c >= 35 && cur_c <= 126)
		)
		return true;
	else
		return false;
}

bool is_digit() {
	return (cur_c >= '0' && cur_c <= '9');
}

bool is_char() {
	if (
		cur_c == '+' || cur_c == '-' ||
		cur_c == '*' || cur_c == '/' || cur_c == '_' ||
		(cur_c >= 'A' && cur_c <= 'Z') ||
		(cur_c >= 'a' && cur_c <= 'z') ||
		is_digit()
		)
		return true;
	else
		return false;
}


int string2int(){
	int num = 0;	// the unsigned integer
	for (int i = 0; i < token_len; i++)
	{
		num *= 10;
		num += token[i] - '0';
	}
	if (DEBUG)
	{
		cout << "read num: " << num << endl;
	}
	return num;
}

Symbol judge_word() {
	token[token_len] = 0; // set tailed
	if (strcmp(token, "char") == 0)
	{
		return CHARSY;
	}
	else if (strcmp(token, "int") == 0)
	{
		return INTSY;
	}
	else if (strcmp(token, "void") == 0)
	{
		return VOIDSY;
	}
	else if (strcmp(token, "const") == 0)
	{
		return CONSTSY;
	}
	else if (strcmp(token, "if") == 0)
	{
		return IFSY;
	}
	else if (strcmp(token, "else") == 0)
	{
		return ELSY;
	}
	else if (strcmp(token, "while") == 0)
	{
		return WHILESY;
	}
	else if (strcmp(token, "do") == 0)
	{
		return DOSY;
	}
	else if (strcmp(token, "for") == 0)
	{
		return FORSY;
	}
	else if (strcmp(token, "scanf") == 0)
	{
		return SCANFSY;
	}
	else if (strcmp(token, "printf") == 0)
	{
		return PRINTFSY;
	}
	else if (strcmp(token, "return") == 0)
	{
		return RTNSY;
	}
	else if (strcmp(token, "main") == 0)
	{
		return MAINSY;
	}
	else  	// return ident if cannot mate any reserved words
	{
		return IDENT;
	}
}

string symbol2string(Symbol sym) {
	switch (sym)
	{
	case IDENT:
		return "IDENT";
	case INTCON:
		return "INTCON";
	case CHARCON:
		return "CHARCON";
	case STRCON:
		return "STRCON";
	case SQM:
		return "SQM";
	case DQM:
		return "DQM";
	case COMMA:
		return "COMMA";
	case COLON:
		return "COLON";
	case SEMI:
		return "SEMI";
	case LPAR:
		return "LPAR";
	case RPAR:
		return "RPAR";
	case LBKT:
		return "LBKT";
	case RBKT:
		return "RBKT";
	case LBRC:
		return "LBRC";
	case RBRC:
		return "RBRC";
	case BECOME:
		return "BECOME";
	case ADD:
		return "ADD";
	case SUB:
		return "SUB";
	case MUL:
		return "MUL";
	case DIV:
		return "DIV";
	case EQ:
		return "EQ";
	case NEQ:
		return "NEQ";
	case GT:
		return "GT";
	case GE:
		return "GE";
	case LT:
		return "LT";
	case LE:
		return "LE";
	case INTSY:
		return "INTSY";
	case CHARSY:
		return "CHARSY";
	case VOIDSY:
		return "VOIDSY";
	case CONSTSY:
		return "CONSTSY";
	case IFSY:
		return "IFSY";
	case ELSY:
		return "ELSY";
	case WHILESY:
		return "WHILESY";
	case DOSY:
		return "DOSY";
	case FORSY:
		return "FORSY";
	case SCANFSY:
		return "SCANFSY";
	case PRINTFSY:
		return "PRINTFSY";
	case RTNSY:
		return "RTNSY";
	case MAINSY:
		return "MAINSY";
	default:
		//error_debug("unexpected word");
		return "";
	}
}

void sym_print() {
	if (symbol == NONE) return;
	string sym_str = symbol2string(symbol);
	sym_num++;
	switch (symbol) {
	case INTCON:
		resfile << sym_num << ' ' << sym_str << ' ' << num << endl;
		break;
	case CHARCON:
		resfile << sym_num << ' ' << sym_str << ' ' << (char)num << endl;
		break;
	default:
		resfile << sym_num << ' ' << sym_str << ' ' << token << endl;
	}
}

bool getsym() {		//只负责处理一行中的一个单词，外部函数要处理好buffer
	token_init();
	symbol = NONE;
	while (need_skip()) {
		if (!nextchar()) return false;
	}

	if (letter_in_ident()){
		while (letter_in_ident() || is_digit()){
			cat_token();
			if (!nextchar()) return false;
		}
		token[token_len] = 0; // set tailed
		symbol = judge_word();
	}
	else if (is_digit()){
		bool leading_zero = false;
		if (cur_c == '0'){
			leading_zero = true;			//先导0
		}
		while (is_digit()){					//读取所有数字
			cat_token();
			if (!nextchar()) return false;
		}
		num = string2int();
		if (leading_zero && token_len != 1){
			symbol = INTCON;
			error("unexpected leading zero");
		}
		symbol = INTCON;
	}
	else{  	// record char
		switch (cur_c) {
		case '\'':
			// mate char
			symbol = CHARCON;
			num = -1;
			do {
				if (!nextchar()) return false;
			} while (need_skip() && cur_c != '\'');
			if (is_char()) {
				num = (int)cur_c;
			}
			else if (cur_c == '\'') {
				error("empty char");
				return true;	// is ''
			}
			else {
				error("invalid char");
			}

			// 匹配单引号
			do {
				if (!nextchar()) return false;
			} while (need_skip());
			if (cur_c != '\'') {
				num = -1;
				error("multiple chars");
				do {
					if (!nextchar()) return false;
				} while (cur_c == '\'');
			}
			break;
		case '\"':
			symbol = STRCON;
			// mate string
			while (true)
			{
				if (!nextchar()) {
					symbol = NONE;
					error("invalid string");
					return false;
				}
				if (cur_c == '\"') {  // "
					token[token_len] = 0; // set tailed
					break;
				}
				else if (!is_string_char()) {
					error("invalid char in string");
					symbol = NONE;
					continue;
				}
				else {
					if (cur_c == '\\') {
						token[token_len++] = cur_c;
					}
					cat_token();
				}
			}
			break;
		case '!':
			cat_token();
			if (!nextchar()) return false;
			if (cur_c == '=') {   // =
				cat_token();
				symbol = NEQ;
				token[token_len] = 0;
			}
			else {
				if (!lastchar()) return false;
				error("unexpected token \'!\'");
			}
			break;
		case '=':
			cat_token();
			if (!nextchar()) return false;
			if (cur_c == '=') {
				symbol = EQ;
				cat_token();
				token[token_len] = 0;
			}
			else
			{
				if (!lastchar()) return false;
				symbol = BECOME;
				token[token_len] = 0;
			}
			break;
		case '>':
			cat_token();
			if (!nextchar()) return false;
			if (cur_c == '=') {
				cat_token();
				symbol = GE;
			}
			else {
				if (!lastchar()) return false;
				symbol = GT;
			}
			token[token_len] = 0;
			break;
		case '<':
			cat_token();
			if (!nextchar()) return false;
			if (cur_c == '=') {
				cat_token();
				symbol = LE;
			}
			else {
				if (!lastchar()) return false;
				symbol = LT;
			}
			token[token_len] = 0;
			break;
		case ',':
			symbol = COMMA;
			cat_token();
			token[token_len] = 0;
			break;
		case ':':
			symbol = COLON;
			cat_token();
			token[token_len] = 0;
			break;
		case ';':
			symbol = SEMI;
			cat_token();
			token[token_len] = 0;
			break;
		case '(':
			symbol = LPAR;
			cat_token();
			token[token_len] = 0;
			break;
		case ')':
			symbol = RPAR;
			cat_token();
			token[token_len] = 0;
			break;
		case '[':
			symbol = LBKT;
			cat_token();
			token[token_len] = 0;
			break;
		case ']':
			symbol = RBKT;
			cat_token();
			token[token_len] = 0;
			break;
		case '{':
			symbol = LBRC;
			cat_token();
			token[token_len] = 0;
			break;
		case '}':
			symbol = RBRC;
			cat_token();
			token[token_len] = 0;
			break;
		case '+':
			symbol = ADD;
			cat_token();
			token[token_len] = 0;
			break;
		case '-':
			symbol = SUB;
			cat_token();
			token[token_len] = 0;
			break;
		case '*':
			symbol = MUL;
			cat_token();
			token[token_len] = 0;
			break;
		case '/':
			symbol = DIV;
			cat_token();
			token[token_len] = 0;
			break;
		default:
			error((string)"unexpected token \'" + cur_c + "\'");
			if (!nextchar()) return false;
			return getsym();    // skip
		}
		if (!nextchar()) return false;
	}
	return true;
}

void words_main() {
	bool mark = true;
	while (readline()) {
		mark = true;
		while(mark){
			mark = getsym();
			sym_print();
		}
	}
}