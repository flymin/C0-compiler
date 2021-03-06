// c0.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
#include <sstream>
#include "const.h"
#include "var.h"
#include "words.h"
#include "grammar.h"
#include "tar.h"
#include "dag_opt.h"
#include "copy_opt.h"
#include "live_var.h"
#include "main.h"

using namespace std;

char token[TOKEN_MAX_LENTH] = { 0 };	// store sign
int token_len = 0;
int num;								// current integer
char cur_c;								// current char to read
ifstream progfile;
ofstream resfile;
fstream midfile;
ofstream tarfile;
Symbol symbol;
bool success = true;

string filename;
string mid_result;
string tar_result;
int file_count = 0;

#define COUNTER_SIZE 3
int line_counter[COUNTER_SIZE] = { 0 };
bool change_stop() {
	for (int i = 0; i < COUNTER_SIZE - 1; i++) {
		if (line_counter[i] != line_counter[i + 1]) {
			line_counter[i + 1] = line_counter[i];
			return false;
		}
	}
	return true;
}

string generate_filename(string info){
	stringstream ss;
	ss << mid_result << "_" << file_count++ << "_" << info << ".txt";
	return ss.str();
}

int main() {
	int _exit;

	//filename = "16071064_test.txt";
	mid_result = "16071064_mid";
	tar_result = "16071064_tar_my.txt";
	
	cout << "please input the name of program file:";
	cin >> filename;
	//cout << "please input the name of mid code file:";
	//cin >> mid_result;
	//cout << "please input the name of tar code file:";
	//cin >> tar_result;
	
	progfile.open((filename).c_str());
	if (!progfile) {
		cout << "open file fail\n";
		cout << "enter a number to exit\n";
		cin >> _exit;
		return 0;
	}
	midfile.open((mid_result + ".txt").c_str(), ios::trunc | ios::out | ios::in);
	if (!midfile) {
		cout << "open file fail\n";
		cout << "enter a number to exit\n";
		cin >> _exit;
		return 0;
	}
	tarfile.open((tar_result).c_str());
	if (!tarfile) {
		cout << "open file fail\n";
		cout << "enter a number to exit\n";
		cin >> _exit;
		return 0;
	}

	if (progfile.eof()) {
		cout << "file is empty!\n";
		cout << "enter a number to exit\n";
		cin >> _exit;
		return 0;
	}

	program();
	
	if (!success) {
		cout << "compile unsucceed" << endl;
		progfile.close();
		midfile.close();
		tarfile.close();
		cout << "enter a number to exit\n";
		cin >> _exit;
		return 0;
	}

	midfile.clear();
	midfile.seekg(0, std::ios::beg);
	midfile.close();//生成中间代码之后直接保存
	string opt_filename = (mid_result + ".txt");
	//这里完成优化

#ifdef OPT
	do {
		opt_filename = dag_main(opt_filename);
		opt_filename = copy_main(opt_filename, &line_counter[0]);
	} while (!change_stop());
	//} while (0);
	opt_filename = livevar_main(opt_filename);
#endif

	midfile.open(opt_filename.c_str(), ios::in);
	tar_code();
	
	if (success) {
		cout << "completed successfully" << endl;
	}
	progfile.close();
	midfile.close();
	tarfile.close();
	cout << "enter a number to exit\n";
	cin >> _exit;
	return 0;
}

