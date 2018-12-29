#ifndef WORD_H_INCLUDED
#define WORD_H_INCLUDED
#include<iostream>
#include<fstream>
#include<string>
#include "var.h"

using namespace std;

void token_init();
void cat_token();

bool readline();
bool nextchar();
bool need_skip();
bool letter_in_ident();
bool is_char();
bool is_string_char();
bool is_digit();

int string2int();

Symbol judge_word();

string symbol2string(Symbol sym);

void sym_print();

bool getsym();

void words_main();


#endif // WORD_H_INCLUDED