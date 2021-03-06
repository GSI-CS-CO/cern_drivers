%{      /******************** C declaration section */
#include "convP.h"
%}

/******************************* yacc/bison declaration section */

%union { /*define the possible token types */
 int     Num;    /*number EG constant expression */
 symbol *Sym;    /*symbol table reference EG identifier */
}

/* Keywords */

%token <Sym>  INCLUDE DEFINE TYPEDEF STRUCT UNION ENUM UNSIGNED SIZEOF
%token <Sym>  IF IFDEF IFNDEF ELSE ENDIF
%token <Sym>  CHAR SHORT INT LONG FLOAT DOUBLE STRING
%token <Sym>  SHL SHR EQ

/* Auxiliary tokens */

%token        E_O_F
%token <Num>  NUM       	  /*numeric value */
%token <Sym>  TYPE STRU UNIO ENU  /*name of type (EG typedef) */
%token <Sym>  SYM       	  /*undefined OR defined name */

/* Types of nonterminal symbols */

%type <Num> exp aexp mexp term prime expr
%type <Sym> name scalar var compound enum struct union type aname typed

%%      /********  ************* Grammar rules */

prog: line
     |prog line
 ;
 line: '\n'
     | E_O_F        {if (Eof()) return; }
     | '#' preproc
     | cmdlist
     | error   '\n' {yyerrok; }
 ;
 preproc: DEFINE name expr   {Define($2,$3); }
   |      INCLUDE filename   {Include(); }
   |      IFNDEF name        {IfDef($2,IFNDEF);}
   |      IFDEF  name        {IfDef($2,IFDEF); }
   |      IF exp             {IfDef($2,IF);    }
   |      ELSE               {IfDef(0L,ELSE);  }
   |      ENDIF              {IfDef(0L,ENDIF); }
 ;
 cmdlist: cmd  ';'
   |      cmdlist cmd ';'
 ;
 cmd: /*empty*/
   |  vardef
   |  typedef
 ;

 filename: '\"' {SetFileName('\"'); }
   |       '<'  {SetFileName('>' ); }
 ;
 name:  SYM                     {nameptr=$1; }
 ;
 expr: /*empty*/                {$$=0; }
    |   exp
 ;
 exp:   aexp
    |   exp '|' aexp            {$$= $1 | $3;}
    |   exp '&' aexp            {$$= $1 + $3;}
    |   exp '^' aexp            {$$= $1 ^ $3;}
    |   '(' type ')' exp        {$$= $4; }
 ;
 aexp:  mexp
    |   aexp SHL mexp  {$$= $1 << $3; }
    |   aexp SHR mexp  {$$= $1 >> $3; }
 ;
 mexp:  term
    |   mexp '+' term  {$$= $1 + $3;}
    |   mexp '-' term  {$$= $1 - $3;}
 ;
 term:  prime
    |   term '*' prime  {$$= $1 * $3;}
    |   term '/' prime  {if ($3) $$= $1 / $3; else printf("Divide by 0 ");}
    |   term '%' prime  {if ($3) $$= $1 % $3; else printf("Divide by 0 ");}
 ;
 prime: NUM
    |   SYM                  {$$= $1->Val; if ($1->Type!=NUM) Error("Constant expected:%s",$1->Name); }
    |   SIZEOF '(' typed ')' {$$= SizeOf($3);}
    |   '(' exp ')'          {$$= $2;}
    |   '-' prime            {$$= - $2;}
    |   '~' prime            {$$= ~ $2;}
 ;
 typed: type
    |   name
 ;

vardef:  type varlist	 {SetType($1); }
 ;
 type:   scalar          {SetType($1); }
    |    compound        {SetType($1); }
 ;
 scalar: CHAR
    |    SHORT
    |    INT
    |    LONG
    |    FLOAT
    |    DOUBLE
    |    STRING
    |    UNSIGNED scalar {$$= $2; }
    |    UNSIGNED '\n' scalar {$$= $3; }
 ;
 compound: enum
    |      union
    |      struct
    |      TYPE
 ;
 varlist: /*empty*/     {NoVarList(); }
    |     varl
    |     varlist comma varl
 ;
 varl: var              {VarInList($1); }
 ;
 var: name              {SetVar($1); }
    | '*' var           {SetPtrLevel($$= $2); }
    | var '[' exp ']'   {SetDimension($1,$3); }
 ;

typedef: TYPEDEF {Typedef=1;} type var  {TypeDef($4); }
 ;

struct:    STRUCT aname structb {$$= typtr; }
    |      STRUCT STRU          {$$= $2; }
 ;
 structb: /*empty*/ {MarkBlock(StructType);}
    |      bopen {OpenBlock(StructType);} vardeflist bclose
 ;
 vardeflist: /*empty*/
    |      vardef scolon vardeflist
 ;

union:     UNION aname unionb  {$$= typtr; };
    |      UNION  UNIO         {$$= $2; }
 ;
 unionb: /*empty*/ {MarkBlock(UnionType);}
    |      bopen {OpenBlock(UnionType);} vardeflist bclose
 ;

enum:     ENUM aname enumb     {$$= typtr; }
    |     ENUM ENU             {$$= $2; }
 ;
 aname: /*empty*/              {nameptr= NULL;}
    |   SYM                    {nameptr= $1;  }
 ;
 enumb: bopen {OpenBlock(EnumType);} enumlist bclose
 ;
 bopen:  nextline '{' nextline
 ;
 bclose: nextline '}' nextline    {CloseBlock();}
 ;
 nextline: /*empty*/
    |      '\n'
    |      nextline '\n'
 ;
 enumlist: enumer
    |      enumlist comma enumer
 ;
 enumer:   name '=' exp  {SetEnum($1,0,$3); }
    |      name          {SetEnum($1,1, 0); }
 ;
 comma:  ',' nextline
 ;
 scolon: ';' nextline
 ;

%%

#include "convP.c"

/* --conv.y-- */
