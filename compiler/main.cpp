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


int main()
{
	int _exit;

	char filename[32] = "16071064_test.txt";
	char mid_result[32] = "16071064_mid.txt";
	char tar_result[32] = "16071064_tar.txt";
	
	//char filename[128];
	//char mid_result[128];
	//char tar_result[128];
	//cout << "please input the name of program file:";
	//cin >> filename;
	//cout << "please input the name of mid code file:";
	//cin >> mid_result;
	//cout << "please input the name of tar code file:";
	//cin >> tar_result;
	
	progfile.open(filename);
	if (!progfile) {
		cout << "open file fail\n";
		cout << "enter a number to exit\n";
		cin >> _exit;
		return 0;
	}
	midfile.open(mid_result, ios::trunc | ios::out | ios::in);
	if (!midfile) {
		cout << "open file fail\n";
		cout << "enter a number to exit\n";
		cin >> _exit;
		return 0;
	}
	tarfile.open(tar_result);
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

