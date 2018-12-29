#ifndef VAR_H_INCLUDED
#define VAR_H_INCLUDED

typedef enum {
	VOID, CHAR, INT, NOTYPE
} Type;

typedef enum {
	CONST, VAR, FUNC, ARRAY, PARA, NOKIND
} Kind;

typedef enum {
	NONE,

	IDENT, INTCON, CHARCON, STRCON, 

	SQM, DQM, COMMA, COLON, SEMI,			//±êµã
	LPAR, RPAR, LBKT, RBKT, LBRC, RBRC,				//À¨ºÅ
	ADD, SUB, MUL, DIV, 
	EQ, NEQ, GT, GE, LT, LE,
	BECOME,


	CHARSY, INTSY, VOIDSY, CONSTSY, IFSY, ELSY, 
	WHILESY, DOSY, FORSY, 

	SCANFSY, PRINTFSY, RTNSY, MAINSY
} Symbol;

#endif // VARS_H_INCLUDED
