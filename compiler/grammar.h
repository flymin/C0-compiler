#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED

#include "mid_code.h"

typedef enum
{
	CONST_STATE, VAR_STATE, FUNC_STATE, MAIN_STATE,
	FUNC_CALL, VAR_ASSIGN, ARR_ASSIGN,
	FUNC_CONST_STATE, FUNC_VAR_STATE,
	NOSTATE
} State;

Type expression(Ret_item* box);
void statement();
void forstate();
void dostate();
void composed_state();
void program();

#endif
