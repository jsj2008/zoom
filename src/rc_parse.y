%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#include "zmachine.h"
#include "rc.h"
#include "rcp.h"
#include "hash.h"

#define YYERROR_VERBOSE

extern int _rc_line;
extern hash rc_hash;

extern void rc_error(char*);
extern int  rc_lex(void);

#define EMPTY_GAME(x) x.interpreter = -1; x.revision = -1; x.name = NULL; x.fonts = NULL; x.n_fonts = -1; x.colours = NULL; x.n_colours = -1; x.gamedir = NULL;

static inline rc_game merge_games(const rc_game* a, const rc_game* b)
{
  rc_game r;

  if (a->interpreter == -1)
    r.interpreter = b->interpreter;
  else
    r.interpreter = a->interpreter;

  if (a->revision == -1)
    r.revision = b->revision;
  else
    r.revision = a->revision;

  if (a->name == NULL)
    r.name = b->name;
  else
    r.name = a->name;

  if (a->fonts == NULL)
    {
      r.fonts = b->fonts;
      r.n_fonts = b->n_fonts;
    }
  else if (b->fonts == NULL)
    {
      r.fonts = a->fonts;
      r.n_fonts = a->n_fonts;
    }
  else
    {
      int x;

      r.n_fonts = a->n_fonts + b->n_fonts;
      r.fonts = malloc(r.n_fonts*sizeof(rc_font));
      
      for (x=0; x<a->n_fonts; x++)
      	r.fonts[x] = a->fonts[x];
      for (x=0; x<b->n_fonts; x++)
        r.fonts[x+a->n_fonts] = b->fonts[x];

      free(a->fonts);
      free(b->fonts);
    }

  if (a->colours == NULL)
    {
      r.colours   = b->colours;
      r.n_colours = b->n_colours;
    }
  else if (b->colours == NULL)
    {
      r.colours   = a->colours;
      r.n_colours = a->n_colours;      
    }
  else
    rc_error("Can only have one set of colours per block");

  if (a->gamedir == NULL)
    r.gamedir = b->gamedir;
  else
    r.gamedir = a->gamedir;

  return r;
}
%}

%union {
  char*       str;
  int         num;
  char        chr;

  rc_font     font;
  rc_game     game;
  rc_colour   col;
  stringlist* slist;
}

%token DEFAULT
%token INTERPRETER
%token REVISION
%token FONT
%token COLOURS
%token GAME
%token ROMAN
%token BOLD
%token ITALIC
%token FIXED
%token GAMEDIR
%token SAVEDIR
%token SOUNDZIP

%token <str> GAMEID
%token <num> NUMBER
%token <str> STRING
%token <chr> CHARACTER

%type <col>   ColourDefn
%type <game>  ColourList
%type <slist> RevisionList
%type <num>   FontQual
%type <num>   FontDefn
%type <font>  FontType
%type <game>  RCOption
%type <game>  RCOptionList
%type <game>  RCBlock

%%

RCFile:		  RCDefn
		| RCFile RCDefn
		;

RCDefn:		  DEFAULT STRING RCBlock
		    {
		      rc_game* game;

		      game = malloc(sizeof(rc_game));
		      *game = $3;
		      game->name = $2;
		      hash_store(rc_hash,
		                 "default",
				 7,
				 game);

		    }
		| DEFAULT RCBlock
		    {
		      rc_game* game;

		      game = malloc(sizeof(rc_game));
		      *game = $2;
		      game->name = "%s";
		      hash_store(rc_hash,
		                 "default",
				 7,
				 game);

		    }
		| GAME STRING RevisionList
		    {
		      rc_game* game;
		      stringlist* next;

		      game = malloc(sizeof(rc_game));
		      EMPTY_GAME((*game));
		      game->name = $2;

		      next = $3;
		      while (next != NULL)
		        {
		          hash_store(rc_hash,
		                     next->string,
				     strlen(next->string),
				     game);
			  next = next->next;
	                }
		    }
		| GAME STRING RevisionList RCBlock
		    {
		      rc_game* game;
		      stringlist* next;

		      game = malloc(sizeof(rc_game));
		      *game = $4;
		      game->name = $2;

		      next = $3;
		      while (next != NULL)
		        {
		          hash_store(rc_hash,
		                     next->string,
				     strlen(next->string),
				     game);
			  next = next->next;
	                }
		    }
		;

RCBlock:	  '{' RCOptionList '}'
		    {
		      $$ = $2;
		    }
		;

RCOptionList:	  RCOption
		| RCOptionList RCOption
		    {
		      $$ = merge_games(&$1, &$2);
		    }
		;

RCOption:	  INTERPRETER NUMBER
		    {
		      EMPTY_GAME($$);
		      $$.interpreter = $2;
		    }
		| REVISION CHARACTER
		    {
		      EMPTY_GAME($$);
		      $$.revision = $2;
		    }
		| FONT NUMBER STRING FontType
		    {
		      EMPTY_GAME($$);
		      $$.fonts = malloc(sizeof(rc_font));
		      $$.n_fonts = 1;
		      $$.fonts[0] = $4;

		      $$.fonts[0].name = $3;
		      $$.fonts[0].num  = $2;
		    }
		| COLOURS ColourList
		    {
		      $$ = $2;
		    }
                | GAMEDIR STRING
		    {
		      EMPTY_GAME($$);
		      $$.gamedir = $2;
		    }
		;

FontType:	  FontDefn
		    {
		      $$.name = NULL;
		      $$.attributes[0] = $1;
		      $$.n_attr = 1;
		      $$.num = 0;
		    }
		| FontType ',' FontDefn
		    {
		      $$ = $1;
		      $$.n_attr++;
		      $$.attributes[$$.n_attr-1] = $3;
		    }
		;

FontDefn:	  FontQual
		    {
		      $$ = $1;
		    }
		| FontDefn '-' FontQual
		    {
		      $$ = $1|$3;
		    }
		;

FontQual:	  ROMAN   { $$ = 0; }
		| BOLD    { $$ = 1; }
		| ITALIC  { $$ = 2; }
		| FIXED   { $$ = 4; }
		;

ColourList:	  ColourDefn
		    {
		      EMPTY_GAME($$);
		      $$.colours = malloc(sizeof(rc_colour));
		      $$.colours[0] = $1;
		      $$.n_colours = 1;
		    }
		| ColourList ',' ColourDefn
		    {
		      $$ = $1;
		      $$.n_colours++;
		      $$.colours = realloc($$.colours, 
				           sizeof(rc_colour)*$$.n_colours);
		      $$.colours[$$.n_colours-1]=$3;
		    }
		;

ColourDefn:	  '(' NUMBER ',' NUMBER ',' NUMBER ')'
		    {
		      $$.r = $2&0xff;
		      $$.g = $4&0xff;
		      $$.b = $6&0xff;
		    }
		;

RevisionList:	  GAMEID 
		    {
		      $$ = malloc(sizeof(stringlist));
		      $$->next = NULL;
		      $$->string = $1;
		    }
		| RevisionList ',' GAMEID
		    {
		      $$ = malloc(sizeof(stringlist));
		      $$->next = $1;
		      $$->string = $3;
		    }
		;

%%
