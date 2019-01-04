#ifndef MAIN_H
#define MAIN_H
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

//#define NEW_TAR
#define OPT

string generate_filename(string info);

#endif