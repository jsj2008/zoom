/*
 *  Builder - builds a ZCode interpreter
 *  Copyright (C) 2000 Andrew Hunter
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Yea, it's ugly. Hopefully not quite as ugly as doing the whole
 * thing by hand, though...
 *
 * For those of you who can't be bothered reading the theory, it's
 * basically that all the effort used here to produce the specialised
 * interpreter is not required at runtime. The length of this file
 * should indicate that that turns out to be quite a lot.
 */

#include <stdio.h>
#include <stdlib.h>

#include "operation.h"
#include "gram.h"

extern int    yyline;
extern oplist zmachine;
extern int    yyparse();

static char*  filename;

static int sortops(const void* un, const void* deux)
{
  operation* one;
  operation* two;

  one = (operation*) un;
  two = (operation*) deux;

  if (one->type > two->type)
    return 1;
  else if (one->type < two->type)
    return -1;
  else if (one->value > two->value)
    return 1;
  else if (one->value < two->value)
    return -1;
  else
    return 0;
}

/* The various sections of code that make up an interpreter */
static char* header =
"/*\n * Interpreter automatically generated for version %i\n * Do not alter this file\n */\n\n  switch (instr)\n    {\n";

static char* notimpl = "    /* %s not implemented */\n";

#define VERSIONS \
		  if (versions != -1) \
		    { \
		      fprintf(dest, "_"); \
		      for (z = 1; z<32; z++) \
			{ \
			  if (versions&(1<<z)) \
			    fprintf(dest, "%i", z); \
			} \
		    }

static void output_opname(FILE* dest,
			  char* opname,
			  int   versions)
{
  int x,z;

  fprintf(dest, "goto op_%s", opname);
  VERSIONS;
  fprintf(dest, ";\n");
}

void output_interpreter(FILE* dest,
			int version)
{
  int vmask;
  int x,y;
  int pcadd;

  vmask = 1<<version;
  fprintf(dest, header, version);

  for (x=0; x<zmachine.numops; x++)
    {
      operation* op;

      op = zmachine.op[x];
      if ((op->versions&vmask && op->versions != -1) || (version==-1
							 && op->versions==-1))
	{
	  switch (op->type)
	    {
	      /* All possible 0OP bytes*/
	    case zop:
	      pcadd = 1;
	      fprintf(dest, "    case 0x%x: /* %s */\n",
		      op->value|0xb0, op->name);

	      fprintf(dest, "#ifdef DEBUG\nprintf(\"%s\\n\");\n#endif\n",
		      op->name);

	      if (op->flags.isstore)
		{
		  fprintf(dest, "      st = GetCode(pc+%i);\n", pcadd);
		  pcadd++;
		}
	      if (op->flags.isbranch)
		{
		  fprintf(dest, "      tmp = GetCode(pc+%i);\n", pcadd);
		  fprintf(dest, "      branch = tmp&0x3f;\n");
		  fprintf(dest, "      padding=0;\n");
		  fprintf(dest, "      if (!(tmp&0x40))\n");
		  fprintf(dest, "        {\n");
		  fprintf(dest, "          padding = 1;\n");
		  fprintf(dest, "          if (branch&0x20)\n");
		  fprintf(dest, "            branch -= 64;\n");
		  fprintf(dest, "          branch <<= 8;\n");
		  fprintf(dest, "          branch |= GetCode(pc+%i);\n",
			  pcadd+1);
		  fprintf(dest, "        }\n");
		  fprintf(dest, "      negate = tmp&0x80;\n");
 		  pcadd++;
		}
	      if (op->flags.isstring)
		{
		  fprintf(dest, "      string = zscii_to_ascii(&GetCode(pc+%i), &padding);\n", pcadd);
		}

	      if (op->flags.isbranch || op->flags.isstring)
		fprintf(dest, "      pc += %i+padding;\n", pcadd);
	      else
		fprintf(dest, "      pc += %i;\n", pcadd);
	      
	      fprintf(dest, "      ");
	      output_opname(dest, op->name, op->versions);
	      fprintf(dest, "\n");
	      break;

	      /* All possible 1OP bytes */
	    case unop:
	      for (y=0; y<3; y++)
		{
		  pcadd = 1;
		  fprintf(dest, "    case 0x%x: /* %s */\n",
			  op->value|0x80|(y<<4), op->name);

		  fprintf(dest, "#ifdef DEBUG\nprintf(\"%s\\n\");\n#endif\n",
			  op->name);

		  switch (y)
		    {
		    case 0:
		      fprintf(dest, "      arg1 = (GetCode(pc+%i)<<8)|GetCode(pc+%i);\n", pcadd, pcadd+1);
		      pcadd+=2;
		      break;

		    case 1:
		      fprintf(dest, "      arg1 = GetCode(pc+%i);\n",
			      pcadd);
		      pcadd++;
		      break;

		    case 2:
		      fprintf(dest, "      arg1 = GetVar(GetCode(pc+%i));\n",
			      pcadd);
		      pcadd++;
		      break;
		    }
		  
		  if (op->flags.isstore)
		    {
		      fprintf(dest, "      st = GetCode(pc+%i);\n", pcadd);
		      pcadd++;
		    }
		  if (op->flags.isbranch)
		    {
		      fprintf(dest, "      tmp = GetCode(pc+%i);\n", pcadd);
		      fprintf(dest, "      branch = tmp&0x3f;\n");
		      fprintf(dest, "      padding=0;\n");
		      fprintf(dest, "      if (!(tmp&0x40))\n");
		      fprintf(dest, "        {\n");
		      fprintf(dest, "          padding = 1;\n");
		      fprintf(dest, "          if (branch&0x20)\n");
		      fprintf(dest, "            branch -= 64;\n");
		      fprintf(dest, "          branch <<= 8;\n");
		      fprintf(dest, "          branch |= GetCode(pc+%i);\n",
			      pcadd+1);
		      fprintf(dest, "        }\n");
		      fprintf(dest, "      negate = tmp&0x80;\n");
		      pcadd++;
		    }
		  if (op->flags.isstring)
		    {
		      fprintf(dest, "      /* Implement me */\n");
		    }
		  
		  if (op->flags.isbranch || op->flags.isstring)
		    fprintf(dest, "      pc += %i+padding;\n", pcadd);
		  else
		    fprintf(dest, "      pc += %i;\n", pcadd);
		  
		  fprintf(dest, "      ");
		  output_opname(dest, op->name, op->versions);
		  
		  fprintf(dest, "\n");
		}
	      break;

	      /* All possible 2OP bytes */
	    case binop:
	      for (y=0; y<4; y++)
		{
		  pcadd = 1;
		  fprintf(dest, "    case 0x%x: /* %s */\n",
			  op->value|(y<<5), op->name);

		  fprintf(dest, "#ifdef DEBUG\nprintf(\"%s\\n\");\n#endif\n",
			  op->name);

		  if (op->flags.reallyvar)
		    fprintf(dest, "      argblock.n_args = 2;\n");
		  if (y&2)
		    fprintf(dest, "      arg1 = GetVar(GetCode(pc+1));\n");
		  else
		    fprintf(dest, "      arg1 = GetCode(pc+1);\n");
		  if (y&1)
		    fprintf(dest, "      arg2 = GetVar(GetCode(pc+2));\n");
		  else
		    fprintf(dest, "      arg2 = GetCode(pc+2);\n");
		  pcadd+=2;
		  
		  if (op->flags.isstore)
		    {
		      fprintf(dest, "      st = GetCode(pc+%i);\n", pcadd);
		      pcadd++;
		    }
		  if (op->flags.isbranch)
		    {
		      fprintf(dest, "      tmp = GetCode(pc+%i);\n", pcadd);
		      fprintf(dest, "      branch = tmp&0x3f;\n");
		      fprintf(dest, "      padding=0;\n");
		      fprintf(dest, "      if (!(tmp&0x40))\n");
		      fprintf(dest, "        {\n");
		      fprintf(dest, "          padding = 1;\n");
		      fprintf(dest, "          if (branch&0x20)\n");
		      fprintf(dest, "            branch -= 64;\n");
		      fprintf(dest, "          branch <<= 8;\n");
		      fprintf(dest, "          branch |= GetCode(pc+%i);\n",
			      pcadd+1);
		      fprintf(dest, "        }\n");
		      fprintf(dest, "      negate = tmp&0x80;\n");
		      pcadd++;
		    }
		  
		  if (op->flags.isbranch || op->flags.isstring)
		    fprintf(dest, "      pc += %i+padding;\n", pcadd);
		  else
		    fprintf(dest, "      pc += %i;\n", pcadd);
		  
		  fprintf(dest, "      ");
		  output_opname(dest, op->name, op->versions);
		  
		  fprintf(dest, "\n");
		}

	      /*
	       * But that's not all, folks (yes, those zany guys at
	       * infocom had a *fifth* way to encode 2OPs. Which
	       * itself has 13 ways of representing its operands.
	       * Lucky me). This is all an evil plan to give me
	       * something better to do.
	       */
	      fprintf(dest, "    case 0x%x: /* %s - variable form */\n",
		      op->value|0xc0, op->name);
	      
	      fprintf(dest, "#ifdef DEBUG\nprintf(\"%s (var form)\\n\");\n#endif\n",
		      op->name);
	      
	      pcadd = 4;
#if 0
	      fprintf(dest, "      padding = 0;\n");
	      fprintf(dest, "      switch (GetCode(pc+1))\n");
	      fprintf(dest, "        {\n");
#define ARGTYPE(i) (i==0?"Large constant":(i==1?"Small constant":(i==2?"Variable":(i==3?"Omitted":"All messed up"))))
	      for (y=0; y<4; y++)
 		{
		  for (z=3; ((z>=0&&y!=3)|(z==3&&y==3)); z--)
		    {
		      int padding;

		      padding = 0;
		      fprintf(dest, "        case %i: /* arg1 - %s, arg2 - %s */\n",
			      (y<<6)|(z<<4)|0xf, ARGTYPE(y), ARGTYPE(z));
		      switch (y)
			{
			case 0:
			  fprintf(dest, "          arg1 = (GetCode(pc+2)<<8)|GetCode(pc+3);\n");
			  padding++;
			  break;
			  
			case 1:
			  fprintf(dest, "          arg1 = GetCode(pc+2);\n");
			  break;
			  
			case 2:
			  fprintf(dest, "          arg1 = GetVar(GetCode(pc+2));\n");
			  break;

			case 3:
			  fprintf(dest, "          arg1 = 0;\n");
			  padding--;
			  break;
			}
		      switch (z)
			{
			case 0:
			  fprintf(dest, "          arg2 = (GetCode(pc+%i)<<8)|GetCode(pc+%i);\n",
				  padding+3, padding+4);
			  padding++;
			  break;
			  
			case 1:
			  fprintf(dest, "          arg2 = GetCode(pc+%i);\n",
				  padding+3);
			  break;
			  
			case 2:
			  fprintf(dest, "          arg2 = GetVar(GetCode(pc+%i));\n",
				  padding+3);
			  break;

			case 3:
			  fprintf(dest, "          arg2 = 0;\n");
			  padding--;
			  break;
			}
		      if (y==3)
			fprintf(dest, "          omit = 0;\n");
		      else if (z==3)
			fprintf(dest, "          omit = 1;\n");
		      else
			fprintf(dest, "          omit = 2;\n");
			
		      fprintf(dest, "          padding = %i;\n", padding);
		      fprintf(dest, "          goto loop;\n");
		    }
		}
	      fprintf(dest, "        default:\n");
	      fprintf(dest, "           zmachine_fatal(\"Invalid argument types for 2OP %s\");\n", op->name);
	      fprintf(dest, "        }\n");
#else

	      fprintf(dest, "        padding = zmachine_decode_varop(stack, &GetCode(pc+1), &argblock)-2;\n");
#endif
	      
	      if (op->flags.isstore)
		{
		  fprintf(dest, "      st = GetCode(pc+%i+padding);\n", pcadd);
		  pcadd++;
		}
	      if (op->flags.isbranch)
		{
		  fprintf(dest, "      tmp = GetCode(pc+%i+padding);\n", pcadd);
		  fprintf(dest, "      branch = tmp&0x3f;\n");
		  fprintf(dest, "      if (!(tmp&0x40))\n");
		  fprintf(dest, "        {\n");
		  fprintf(dest, "          padding++;\n");
		  fprintf(dest, "          if (branch&0x20)\n");
		  fprintf(dest, "            branch -= 64;\n");
		  fprintf(dest, "          branch <<= 8;\n");
		  fprintf(dest, "          branch |= GetCode(pc+%i+padding);\n",
			  pcadd);
		  fprintf(dest, "        }\n");
		  fprintf(dest, "      negate = tmp&0x80;\n");
		  pcadd++;
		}

	      fprintf(dest, "      pc += %i+padding;\n", pcadd);
	      fprintf(dest, "      ");
	      output_opname(dest, op->name, op->versions);
	      fprintf(dest, "\n");
	      break;

	    case varop:
	      fprintf(dest, "    case 0x%x: /* %s */\n",
		      op->value|0xe0, op->name);
	      fprintf(dest, "#ifdef DEBUG\nprintf(\"%s\\n\");\n#endif\n",
		      op->name);
	      pcadd = 2;
	      if (op->flags.islong)
		{
		  fprintf(dest, "      padding = zmachine_decode_doubleop(stack, &GetCode(pc+1), &argblock);\n");
		  pcadd++;
		}
	      else
		fprintf(dest, "      padding = zmachine_decode_varop(stack, &GetCode(pc+1), &argblock);\n");
		  
	      if (op->flags.isstore)
		{
		  fprintf(dest, "      st = GetCode(pc+%i+padding);\n", pcadd);
		  pcadd++;
		}
	      if (op->flags.isbranch)
		{
		  fprintf(dest, "      tmp = GetCode(pc+%i+padding);\n", pcadd);
		  fprintf(dest, "      branch = tmp&0x3f;\n");
		  fprintf(dest, "      if (!(tmp&0x40))\n");
		  fprintf(dest, "        {\n");
		  fprintf(dest, "          padding++;\n");
		  fprintf(dest, "          if (branch&0x20)\n");
		  fprintf(dest, "            branch -= 64;\n");
		  fprintf(dest, "          branch <<= 8;\n");
		  fprintf(dest, "          branch |= GetCode(pc+%i+padding);\n",
			  pcadd);
		  fprintf(dest, "        }\n");
		  fprintf(dest, "      negate = tmp&0x80;\n");
		  pcadd++;
		}

	      fprintf(dest, "      pc += %i+padding;\n", pcadd);
	      fprintf(dest, "      ");
	      output_opname(dest, op->name, op->versions);	      
	      fprintf(dest, "\n");
	      break;

	    case extop:
	      /* Dealt with later */
	      break;
	    }
	}
      else
	{
	  fprintf(dest, notimpl, op->name);
	}
    }

  if (version != -1) /* Extops do not exist generally */
    {
      fprintf(dest, "    case 0x%x: /* Extended ops */\n", 190);
      fprintf(dest, "      switch (GetCode(pc+1))\n");
      fprintf(dest, "        {\n");
      
      for (x=0; x<zmachine.numops; x++)
	{
	  operation* op;
	  
	  op = zmachine.op[x];
	  
	  if (op->type == extop && op->versions&vmask)
	    {
	      fprintf(dest, "        case 0x%x: /* %s */\n", op->value, op->name);
	      fprintf(dest, "#ifdef DEBUG\nprintf(\"ExtOp: %s\n\");\n#endif\n", op->name);
	      pcadd = 3;
	      if (op->flags.islong)
		{
		  fprintf(dest, "          padding = zmachine_decode_doubleop(stack, &GetCode(pc+2), &argblock);\n");
		  pcadd++;
		}
	      else
		fprintf(dest, "          padding = zmachine_decode_varop(stack, &GetCode(pc+2), &argblock);\n");
	      
	      if (op->flags.isstore)
		{
		  fprintf(dest, "          st = GetCode(pc+%i+padding);\n", pcadd);
		  pcadd++;
		}
	      if (op->flags.isbranch)
		{
		  fprintf(dest, "          tmp = GetCode(pc+%i+padding);\n", pcadd);
		  fprintf(dest, "          branch = tmp&0x3f;\n");
		  fprintf(dest, "          if (!(tmp&0x40))\n");
		  fprintf(dest, "            {\n");
		  fprintf(dest, "              padding++;\n");
		  fprintf(dest, "              if (branch&0x20)\n");
		  fprintf(dest, "                branch -= 64;\n");
		  fprintf(dest, "              branch <<= 8;\n");
		  fprintf(dest, "              branch |= GetCode(pc+%i+padding);\n",
			  pcadd);
		  fprintf(dest, "            }\n");
		  fprintf(dest, "          negate = tmp&0x80;\n");
		  pcadd++;
		}

	      fprintf(dest, "          pc+=%i+padding;\n", pcadd);
	      fprintf(dest, "          ");
	      output_opname(dest, op->name, op->versions);	      
	      fprintf(dest, "\n");
	    }
	}
      fprintf(dest, "        default:\n");
      if (version != -1)
	{
	  fprintf(dest, "          zmachine_fatal(\"Unknown extended opcode: %%x\", GetCode(pc+1));\n");
	}
      else
	{
	  fprintf(dest, "          break;\n");
	}
      fprintf(dest, "        }\n");
    }

  fprintf(dest, "    default:\n");
  if (version != -1)
    {
      fprintf(dest, "      zmachine_fatal(\"Unknown opcode: %%x\", GetCode(pc));\n");
    }
  else
    {
      fprintf(dest, "      goto version;\n");
    }
  fprintf(dest, "    }\n");
}

void output_operations(FILE* dest,
		       int   ver)
{
  int x, z;
  int versions, vmask;

  vmask = 1<<ver;

  for (x=0; x<zmachine.numops; x++)
    {
      operation* op;

      op = zmachine.op[x];

      versions = op->versions;
      if ((op->versions&vmask && op->versions != -1) ||
	  (ver==-1 && op->versions == -1))
      {
	fprintf(dest, "#ifndef ZCODE_OP_%s", op->name);
	VERSIONS;
	fprintf(dest, "\n");
	fprintf(dest, "# define ZCODE_OP_%s", op->name);
	VERSIONS;
	fprintf(dest, "\n");

	fprintf(dest, "  op_%s", op->name);
	VERSIONS;
	fprintf(dest, ":\n");

	if (op->code == NULL)
	  fprintf(dest, "    zmachine_warning(\"%s not implemented\");\n", op->name);
	else
	  {
	    fprintf(dest, "    {\n");
	    fprintf(dest, "#line %i \"%s\"\n", op->codeline, filename);
	    fprintf(dest, "%s\n", op->code);
	    fprintf(dest, "    }\n");
	  }
	
	fprintf(dest, "    goto loop;\n\n");
	fprintf(dest, "#endif\n");
      }
    }
}

extern FILE* yyin;

int main(int argc, char** argv)
{
  if (argc==4)
    {
      FILE* output;

      if (!(yyin = fopen(filename=argv[3], "r")))
	{
	  fprintf(stderr, "Couldn't open input file\n");
	  return 1;
	}

      yyline          = 1;
      zmachine.numops = 0;
      zmachine.op     = NULL;
      yyparse();

      qsort(zmachine.op, zmachine.numops-1, sizeof(operation*), sortops);
      
      if ((output = fopen(argv[1], "w")))
	{
	  output_interpreter(output, atoi(argv[2]));
	  output_operations(output, atoi(argv[2]));
	  fclose(output);
	}
      else
	{
	  fprintf(stderr, "Couldn't output open file\n");
	  return 1;
	}
    }
  else
    {
      fprintf(stderr, "Incorrect arguments\n");
      return 1;
    }

  return 0;
}
