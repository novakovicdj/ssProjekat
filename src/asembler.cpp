#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "../inc/asembler.hpp"
#include "FlexLexer.h"
#include <sstream>
#include <iomanip>
using namespace std;

enum tok
{
  DIR = 1,
  HEX,
  LAB,
  NUM,
  REG,
  WRD,
  SPC,
  SGN,
  NWL,
  STR
};

struct inf
{
  char *val;
  char *typ;
};

struct inf i1;
bool newLine = true; // da proveravam da li je nova instrukcija ili direktiva u novom redu
bool endDir = false;
short id = 1;
short currSectionOffset = 0;
SymTable *symTable = new SymTable();
int linebr = 1;

extern string currSection;
map<string, list<char> *> *sectionContent = new map<string, list<char> *>();
map<string, RelaTable *> *relaTables = new map<string, RelaTable *>();

map<string, int> allDirectives = {
    {".global", 1},
    {".extern", 2},
    {".section", 3},
    {".word", 4},
    {".skip", 5},
    {".end", 6},
    {".ascii", 7},
    {".equ", 8}};

map<string, int> allInstructions = {
    {"halt", 1},
    {"int", 2},
    {"iret", 3},
    {"call", 4},
    {"ret", 5},
    {"jmp", 6},
    {"jeq", 7},
    {"jne", 8},
    {"jgt", 9},
    {"push", 10},
    {"pop", 11},
    {"xchg", 12},
    {"add", 13},
    {"sub", 14},
    {"mul", 15},
    {"div", 16},
    {"cmp", 17},
    {"not", 18},
    {"and", 19},
    {"or", 20},
    {"xor", 21},
    {"test", 22},
    {"shl", 23},
    {"shr", 24},
    {"ldr", 25},
    {"str", 26}};

void checkRelaTables(string n)
{
  for (auto it = relaTables->begin(); it != relaTables->end(); it++)
  {
    list<relEntry *> *lst = it->second->getEntries();
    for (auto it2 = lst->begin(); it2 != lst->end(); it2++)
    {
      if ((*it2)->symbol->name == n)
      {
        (*it2)->addend -= symTable->getSymbol(n)->value;
        (*it2)->symbol = symTable->getSymbol(n);
      }
    }
  }
}

void processOperandJump(FlexLexer *lex, string op)
{
  int tok, number;
  string str;
  symb *ent;
  tok = lex->yylex();
  // neg broj?
  if (tok == NUM || tok == HEX)
  {
    str = i1.val;
    number = tok == NUM ? atoi(i1.val) : stoi(str.substr(2, str.length()), nullptr, 16);
    sectionContent->at(currSection)->push_back((char)240);
    sectionContent->at(currSection)->push_back((char)0);
    sectionContent->at(currSection)->push_back((char)(number & 0xFF));
    sectionContent->at(currSection)->push_back((char)((number >> 8) & 0xFF));
    currSectionOffset += 5;
  }
  else if (tok == WRD)
  {
    symTable->addSymbol(i1.val);
    ent = symTable->getSymbol(i1.val);
    if (!symTable->isDefined(i1.val))
    {
      symTable->addToFlink(i1.val, currSectionOffset + 3);
    }
    relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
    sectionContent->at(currSection)->push_back((char)240);
    sectionContent->at(currSection)->push_back((char)0);
    sectionContent->at(currSection)->push_back((char)0);
    sectionContent->at(currSection)->push_back((char)0);
    currSectionOffset += 5;
  }
  else if (tok == SPC)
  {
    if (i1.val[0] == '%')
    {
      tok = lex->yylex();
      if (tok != WRD)
        throw BadInstructionException(op);
      symTable->addSymbol(i1.val);
      ent = symTable->getSymbol(i1.val);
      if (!symTable->isDefined(i1.val))
      {
        symTable->addToFlink(i1.val, currSectionOffset + 3);
      }
      relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_PC16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
      sectionContent->at(currSection)->push_back((char)247);
      sectionContent->at(currSection)->push_back((char)21); // umanjujem za 2 pre formiranaj adrese
      sectionContent->at(currSection)->push_back((char)0);
      sectionContent->at(currSection)->push_back((char)0);
      currSectionOffset += 5;
      // da li u addend stavljam -2 jos ili pomocu onih bita U4U3U2U1 namestim da se registar umanji za dva pre formiranja adrese
    }
    else if (i1.val[0] == '*')
    {
      tok = lex->yylex();
      if (tok == NUM)
      {
        number = atoi(i1.val);
        sectionContent->at(currSection)->push_back((char)240);
        sectionContent->at(currSection)->push_back((char)4);
        sectionContent->at(currSection)->push_back((char)(number & 0xFF));
        sectionContent->at(currSection)->push_back((char)((number >> 8) & 0xFF));
        currSectionOffset += 5;
      }
      else if (tok == HEX)
      {
        str = i1.val;
        number = stoi(str.substr(2, str.length()), nullptr, 16);
        sectionContent->at(currSection)->push_back((char)240);
        sectionContent->at(currSection)->push_back((char)4);
        sectionContent->at(currSection)->push_back((char)number & 0xFF);
        sectionContent->at(currSection)->push_back((char)((number >> 8) & 0xFF));
        currSectionOffset += 5;
      }
      else if (tok == WRD)
      {
        symTable->addSymbol(i1.val);
        ent = symTable->getSymbol(i1.val);
        if (!symTable->isDefined(i1.val))
        {
          symTable->addToFlink(i1.val, currSectionOffset + 3);
        }
        relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
        sectionContent->at(currSection)->push_back((char)240);
        sectionContent->at(currSection)->push_back((char)4);
        sectionContent->at(currSection)->push_back((char)0);
        sectionContent->at(currSection)->push_back((char)0);
        currSectionOffset += 5;
      }
      else if (tok == REG)
      {
        number = atoi(&i1.val[1]);
        number = 0xF0 | (number & 0xF);
        sectionContent->at(currSection)->push_back((char)number);
        sectionContent->at(currSection)->push_back(1);
        currSectionOffset += 3;
      }
      else if (tok == SPC)
      {
        tok = lex->yylex();
        if (tok != REG)
          throw BadInstructionException(op);
        number = atoi(&i1.val[1]);
        number = 0xF0 | (number & 0xF);
        sectionContent->at(currSection)->push_back((char)number);
        tok = lex->yylex();
        if (tok == SGN)
        {
          if (i1.val[0] != '+')
          {
            throw BadInstructionException(op);
          }
          tok = lex->yylex();
          if (tok == NUM || tok == HEX)
          {
            str = i1.val;
            number = tok == NUM ? atoi(i1.val) : stoi(str.substr(2, str.length()), nullptr, 16);
            sectionContent->at(currSection)->push_back((char)3);
            sectionContent->at(currSection)->push_back((char)(number & 0xFF));
            sectionContent->at(currSection)->push_back((char)((number >> 8) & 0xFF));
            currSectionOffset += 5;
          }
          else if (tok == WRD)
          {
            symTable->addSymbol(i1.val);
            ent = symTable->getSymbol(i1.val);
            if (!symTable->isDefined(i1.val))
            {
              symTable->addToFlink(i1.val, currSectionOffset + 3);
            }
            relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
            sectionContent->at(currSection)->push_back((char)3);
            sectionContent->at(currSection)->push_back((char)0);
            sectionContent->at(currSection)->push_back((char)0);
            currSectionOffset += 5;
          }
          else
          {
            throw BadInstructionException(op);
          }
          tok = lex->yylex();
          if (tok != SPC)
          {
            throw BadInstructionException(op);
          }
        }
        else if (tok == SPC)
        {
          sectionContent->at(currSection)->push_back(2);
          currSectionOffset += 3;
        }
        else
        {
          throw BadInstructionException(op);
        }
      }
      else
      {
        throw BadInstructionException(op);
      }
    }
    else
    {
      throw BadInstructionException(op);
    }
  }
  else
  {
    throw BadInstructionException(op);
  }
}

void processArithmeticAndLogicOp(FlexLexer *lex, string op)
{
  int tok;
  int number;
  tok = lex->yylex();
  if (tok != REG)
  {
    // cout << "processArithm 1";
    throw BadInstructionException(op);
  }
  number = atoi(&i1.val[1]);
  tok = lex->yylex();
  if (tok != REG)
  {
    // cout << "processArithm 2";
    throw BadInstructionException(op);
  }
  number = ((number << 4) & 0xF0) | (atoi(&i1.val[1]) & 0xF);
  sectionContent->at(currSection)->push_back((char)number);
  currSectionOffset += 2;
}

void processLdrAndStr(FlexLexer *lex, string op)
{
  int tok;
  int num;
  string str;
  symb *ent;
  tok = lex->yylex();
  // cout << "usao";
  if (tok != REG)
  {
    // cout << "---BACA KOD REG---";
    throw BadInstructionException(op);
  }
  num = atoi(&i1.val[1]);
  // cout << num;
  tok = lex->yylex();
  if (tok == SPC)
  {
    if (i1.val[0] == '$')
    {
      // cout << "$ procitaao";
      tok = lex->yylex();
      // cout << i1.val;
      if (tok == NUM || tok == HEX)
      {
        num = (num << 4) & 0xF0;
        str = i1.val;
        sectionContent->at(currSection)->push_back((char)num);
        sectionContent->at(currSection)->push_back((char)0);
        num = tok == NUM ? atoi(i1.val) : stoi(str.substr(2, str.length()), nullptr, 16);
        sectionContent->at(currSection)->push_back((char)(num & 0xFF));
        sectionContent->at(currSection)->push_back((char)((num >> 8) & 0xFF));
        currSectionOffset += 5;
      }
      else if (tok == WRD)
      {
        // cout << "dobar";
        // if(symTable->getSymbol(i1.val) == 0)
        num = (num << 4) & 0xF0;
        // printf("%02hhx <--", (char)num);
        symTable->addSymbol(i1.val);
        ent = symTable->getSymbol(i1.val);
        if (!symTable->isDefined(i1.val))
        {
          symTable->addToFlink(i1.val, currSectionOffset + 3);
        }
        relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
        sectionContent->at(currSection)->push_back((char)num);
        sectionContent->at(currSection)->push_back((char)0);
        sectionContent->at(currSection)->push_back((char)0);
        sectionContent->at(currSection)->push_back((char)0);
        currSectionOffset += 5;
      }
      else
      {
        // cout << "ovde $";
        throw BadInstructionException(op);
      }
    }
    else if (i1.val[0] == '%')
    {
      // cout << "%%%%";
      tok = lex->yylex();
      if (tok != WRD)
      {
        // cout << "ovde %";
        throw BadInstructionException(op);
      }
      symTable->addSymbol(i1.val);
      ent = symTable->getSymbol(i1.val);
      if (!symTable->isDefined(i1.val))
      {
        symTable->addToFlink(i1.val, currSectionOffset + 3);
      }
      relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_PC16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
      sectionContent->at(currSection)->push_back((char)(num << 4 | (7 & 0xFF)));
      sectionContent->at(currSection)->push_back((char)19);
      sectionContent->at(currSection)->push_back((char)0);
      sectionContent->at(currSection)->push_back((char)0);
      currSectionOffset += 5;
    }
    else if (i1.val[0] == '[')
    {
      tok = lex->yylex();
      if (tok == REG)
      {
        num = (num << 4) | (atoi(&i1.val[1]) & 0xF);
        sectionContent->at(currSection)->push_back((char)num);
        tok = lex->yylex();
        if (tok == SGN)
        {
          if (i1.val[0] != '+')
          {
            // cout << "ovde +";
            throw BadInstructionException(op);
          }
          tok = lex->yylex();
          if (tok == NUM || tok == HEX)
          {
            sectionContent->at(currSection)->push_back((char)3);
            str = i1.val;
            num = tok == NUM ? atoi(i1.val) : stoi(str.substr(2, str.length()), nullptr, 16);
            sectionContent->at(currSection)->push_back((char)(num & 0xFF));
            sectionContent->at(currSection)->push_back((char)((num >> 8) & 0xFF));
            currSectionOffset += 5;
          }
          else if (tok == WRD)
          {
            symTable->addSymbol(i1.val);
            ent = symTable->getSymbol(i1.val);
            if (!symTable->isDefined(i1.val))
            {
              symTable->addToFlink(i1.val, currSectionOffset + 3);
            }
            relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
            sectionContent->at(currSection)->push_back((char)3);
            sectionContent->at(currSection)->push_back((char)0);
            sectionContent->at(currSection)->push_back((char)0);
            currSectionOffset += 5;
          }
          else
          {
            // cout << "ovde 3";
            throw BadInstructionException(op);
          }
          tok = lex->yylex();
        }
        else if (tok == SPC)
        {
          sectionContent->at(currSection)->push_back((char)2);
          currSectionOffset += 3;
        }
        else
        {
          // cout << "ovde 2";
          throw BadInstructionException(op);
        }
      }
      else
      {
        // cout << "ovde 1";
        throw BadInstructionException(op);
      }
    }
    else
    {
      // cout << "Lose poredjenje";
      throw BadInstructionException(op);
    }
  }
  else if (tok == NUM || tok == HEX)
  {
    num = (num << 4) & 0xF0;
    sectionContent->at(currSection)->push_back((char)num);
    sectionContent->at(currSection)->push_back((char)4);
    str = i1.val;
    num = tok == NUM ? atoi(i1.val) : stoi(str.substr(2, str.length()), nullptr, 16);
    sectionContent->at(currSection)->push_back((char)(num & 0xFF));
    sectionContent->at(currSection)->push_back((char)((num >> 8) & 0xFF));
    currSectionOffset += 5;
  }
  else if (tok == WRD)
  {
    // if(symTable->getSymbol(i1.val) == 0)
    symTable->addSymbol(i1.val);
    ent = symTable->getSymbol(i1.val);
    if (!symTable->isDefined(i1.val))
    {
      symTable->addToFlink(i1.val, currSectionOffset + 3);
    }
    relaTables->at(currSection)->addEntry(currSectionOffset + 3, "R_16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);
    num = (num << 4) & 0xF0;
    sectionContent->at(currSection)->push_back((char)num);
    sectionContent->at(currSection)->push_back((char)4);
    sectionContent->at(currSection)->push_back((char)0);
    sectionContent->at(currSection)->push_back((char)0);
    currSectionOffset += 5;
  }
  else if (tok == REG)
  {
    num = (num << 4 & 0xF0) | (atoi(&i1.val[1]) & 0xF);
    sectionContent->at(currSection)->push_back((char)num);
    sectionContent->at(currSection)->push_back((char)1);
    currSectionOffset += 3;
  }
  else
  {
    // cout << "---NE CITA LEPO POSLE REGA---";
    throw BadInstructionException(op);
  }
}

void processDirective(string dir, FlexLexer *lex)
{
  int tok;
  int len;
  symb *ent;
  string txt;
  try
  {
    switch (allDirectives[dir])
    {
    case 1: // .global
      while ((tok = lex->yylex()) != NWL)
      {
        if (tok != WRD)
        {
          throw BadDirectiveException(".global");
        }
        symTable->addSymbol(i1.val, 0, false, true);
        symTable->setGlobalType(i1.val);
        checkRelaTables(i1.val);
      }

      break;
    case 2: // .extern
      while ((tok = lex->yylex()) != NWL)
      {
        if (tok != WRD)
        {
          throw BadDirectiveException(".extern");
        }
        symTable->addSymbol(i1.val, 0, false, true);
        ent = symTable->getSymbol(i1.val);
        ent->sect = "UND";
        ent->bind = 'g';
      }

      break;
    case 3: // .section
      tok = lex->yylex();
      if (tok != WRD)
      {
        throw BadDirectiveException(".section");
      }
      symTable->addSymbol(i1.val, 0, true);
      ent = symTable->getSymbol(i1.val);
      ent->isDef = true;
      // symTable->printSymTable();
      break;
    case 4: // .word
      while ((tok = lex->yylex()) != NWL)
      {
        list<char> *lst = sectionContent->at(currSection);
        currSectionOffset += 2;
        if (tok == WRD)
        {
          symTable->addSymbol(i1.val);
          ent = symTable->getSymbol(i1.val);
          relaTables->at(currSection)->addEntry(currSectionOffset - 2, "R_16", ent->bind == 'l' && ent->isDef ? symTable->getSymbol(ent->sect) : ent, ent->bind == 'l' ? ent->value : 0);

          if (!symTable->isDefined(i1.val))
          {
            symTable->addToFlink(i1.val, currSectionOffset - 2);
          }
          lst->push_back(0);
          lst->push_back(0);
        }
        else if (tok == NUM)
        {
          len = atoi(i1.val);
          sectionContent->at(currSection)->push_back((char)(len & 0xFF));
          sectionContent->at(currSection)->push_back((char)((len >> 8) & 0xFF));
        }
        else if (tok == HEX)
        {
          txt = i1.val;
          len = stoi(txt.substr(2, txt.length()), nullptr, 16);
          sectionContent->at(currSection)->push_back((char)(len & 0xFF));
          sectionContent->at(currSection)->push_back((char)((len >> 8) & 0xFF));
        }
        else
        {
          throw BadDirectiveException(".word");
        }
      }
      break;
    case 5: // .skip
      tok = lex->yylex();
      if (tok != NUM && tok != HEX)
      {
        throw BadDirectiveException(".skip");
      }
      txt = i1.val;
      len = tok == NUM ? atoi(i1.val) : stoi(txt.substr(2, txt.length()), nullptr, 16);
      currSectionOffset += len;
      for (int i = 0; i < len; i++)
      {
        sectionContent->at(currSection)->push_back(0);
      }
      break;
    case 6: // .end
      endDir = true;
      break;
    case 7: // .ascii
      tok = lex->yylex();
      if (tok != STR)
      {
        throw BadDirectiveException(".ascii");
      }
      txt = (string)i1.val;
      currSectionOffset += txt.length();
      // cout << txt.length();
      for (int i = 0; i < txt.length(); i++)
      {
        // cout << txt[i];
        if (txt[i] != '\0')
        {
          if (txt[i] == '\"' || txt[i] == '\'')
          {
            currSectionOffset--;
            continue;
          }
          if (txt[i] == '\\')
          {
            currSectionOffset--;
            i++;
            if (txt[i] == 'n')
            {
              sectionContent->at(currSection)->push_back('\n');
              // sectionContent->at(currSection)->push_back((char)0);
            }
            else if (txt[i] == 't')
            {
              sectionContent->at(currSection)->push_back('\t');
              // sectionContent->at(currSection)->push_back((char)0);
            }
            else if (txt[i] == 'b')
            {
              sectionContent->at(currSection)->push_back('\b');
              // sectionContent->at(currSection)->push_back((char)0);
            }
            else
            {
            }
          }
          else
          {
            sectionContent->at(currSection)->push_back(txt[i]);
            // sectionContent->at(currSection)->push_back((char)0);
          }
        }
        else
        {
          break;
        }
      }
      break;
    case 8: // .equ
      tok = lex->yylex();
      break;
    default:
      cout << "Greska: Nepoznata direktiva!" << endl;
      exit(-4);
    }
  }
  catch (BadDirectiveException &e)
  {
    e.what();
    exit(-4);
  }
  // cout << dir;
}

void processLabel(string lab)
{

  symTable->addSymbol(lab.substr(0, lab.length() - 1), currSectionOffset, false, false, true);
  for (auto it = relaTables->begin(); it != relaTables->end(); it++)
  {
    for (auto it2 = it->second->getEntries()->begin(); it2 != it->second->getEntries()->end(); ++it2)
    {
      if (lab == (*it2)->symbol->name)
      {
        if ((*it2)->symbol->bind == 'l')
        {
          (*it2)->symbol = symTable->getSymbol(symTable->getSymbol(lab)->sect);
        }
      }
    }
  }
}

void processInstruction(string wrd, FlexLexer *lex)
{
  int tok;
  char num;
  unsigned int number;
  string str;
  symb *ent;
  try
  {
    switch (allInstructions[wrd])
    {
    case 1: // halt
      sectionContent->at(currSection)->push_back(0);
      currSectionOffset++;
      break;
    case 2: // int
      tok = lex->yylex();
      if (tok != REG)
      {
        throw BadInstructionException("int");
      }
      sectionContent->at(currSection)->push_back(16);
      num = i1.val[1];
      number = atoi(&num);
      if (number < 0 || number > 7)
      {
        throw BadInstructionException("int");
      }
      number = (number << 4) | 0xF;
      sectionContent->at(currSection)->push_back((char)number);
      currSectionOffset += 2;
      break;
    case 3: // iret
      // psw?
      sectionContent->at(currSection)->push_back((char)32);
      currSectionOffset++;
      break;
    case 4: // call
      sectionContent->at(currSection)->push_back((char)48);
      processOperandJump(lex, "call");
      break;
    case 5: // ret
      sectionContent->at(currSection)->push_back((char)64);
      currSectionOffset++;
      break;
    case 6: // jmp
      sectionContent->at(currSection)->push_back((char)80);
      processOperandJump(lex, "jmp");
      break;
    case 7: // jeq
      sectionContent->at(currSection)->push_back((char)81);
      processOperandJump(lex, "jeq");
      break;
    case 8: // jne
      sectionContent->at(currSection)->push_back((char)82);
      processOperandJump(lex, "jne");
      break;
    case 9: // jgt
      sectionContent->at(currSection)->push_back((char)83);
      processOperandJump(lex, "jgt");
      break;
    case 10: // push
      sectionContent->at(currSection)->push_back((char)176);
      tok = lex->yylex();
      if (tok != REG)
      {
        throw BadInstructionException("push");
      }
      number = (atoi(&i1.val[1]) << 4) | (6 & 0xF);
      sectionContent->at(currSection)->push_back((char)number);
      sectionContent->at(currSection)->push_back((char)18);
      currSectionOffset += 3;
      break;
    case 11: // pop
      sectionContent->at(currSection)->push_back((char)160);
      tok = lex->yylex();
      if (tok != REG)
      {
        throw BadInstructionException("pop");
      }
      number = (atoi(&i1.val[1]) << 4) | (6 & 0xF);
      sectionContent->at(currSection)->push_back((char)number);
      sectionContent->at(currSection)->push_back((char)66);
      currSectionOffset += 3;
      break;
    case 12: // xchg
      sectionContent->at(currSection)->push_back((char)96);
      processArithmeticAndLogicOp(lex, "xchg");
      break;
    case 13: // add
      sectionContent->at(currSection)->push_back((char)112);
      processArithmeticAndLogicOp(lex, "add");
      break;
    case 14: // sub
      sectionContent->at(currSection)->push_back((char)113);
      processArithmeticAndLogicOp(lex, "sub");
      break;
    case 15: // mul
      sectionContent->at(currSection)->push_back((char)114);
      processArithmeticAndLogicOp(lex, "mul");
      break;
    case 16: // div
      sectionContent->at(currSection)->push_back((char)115);
      processArithmeticAndLogicOp(lex, "div");
      break;
    case 17: // cmp
      sectionContent->at(currSection)->push_back((char)116);
      processArithmeticAndLogicOp(lex, "cmp");
      break;
    case 18: // not
      sectionContent->at(currSection)->push_back((char)128);
      tok = lex->yylex();
      if (tok != REG)
      {
        throw BadInstructionException("not");
      }
      number = atoi(&i1.val[1]);
      number = (number << 4) & 0xF0;
      sectionContent->at(currSection)->push_back((char)number);
      currSectionOffset += 2;
      break;
    case 19: // and
      sectionContent->at(currSection)->push_back((char)129);
      processArithmeticAndLogicOp(lex, "and");
      break;
    case 20: // or
      sectionContent->at(currSection)->push_back((char)130);
      processArithmeticAndLogicOp(lex, "or");
      break;
    case 21: // xor
      sectionContent->at(currSection)->push_back((char)131);
      processArithmeticAndLogicOp(lex, "xor");
      break;
    case 22: // test
      sectionContent->at(currSection)->push_back((char)132);
      processArithmeticAndLogicOp(lex, "test");
      break;
    case 23: // shl
      sectionContent->at(currSection)->push_back((char)144);
      processArithmeticAndLogicOp(lex, "shl");
      break;
    case 24: // shr
      sectionContent->at(currSection)->push_back((char)145);
      processArithmeticAndLogicOp(lex, "shr");
      break;
    case 25: // ldr
      sectionContent->at(currSection)->push_back((char)160);
      processLdrAndStr(lex, "ldr");
      break;
    case 26: // str
      sectionContent->at(currSection)->push_back((char)176);
      processLdrAndStr(lex, "str");
      break;
    default:
      cout << "Greska: Nepravilan ulazni fajl!" << endl;
      exit(-4);
    }
  }
  catch (exception &e)
  {
    e.what();
    exit(-5);
  }
  // cout << wrd;
}

int main(int argc, char const *argv[])
{
  string outputFile;
  string inputFile;
  ifstream file;
  ofstream outfile;
  ofstream outfile_b;
  int token;
  bool lab = false;
  try
  {
    if (argc < 2 || argc == 3 || argc > 4)
    {
      throw BadUsageException();
    }
    if (argc == 4)
    {
      if ((string)argv[1] != "-o")
      {
        throw BadUsageException();
      }
      inputFile = argv[3];
      outputFile = argv[2];
    }
    else
    {
      inputFile = argv[1];
      outputFile = "asemblirano.o"; // podrazumevano ime;
    }
  }
  catch (BadUsageException &e)
  {
    e.what();
    exit(-1);
  }

  file.open(inputFile, ios::in);
  try
  {
    if (!file.is_open())
    {
      throw UnknownFileException();
    }
  }
  catch (UnknownFileException &e)
  {
    e.what();
    exit(-2);
  }

  FlexLexer *lex = new yyFlexLexer((istream *)&file);
  try
  {
    while ((token = lex->yylex()) != 0 && !endDir)
    {

      if (token == NWL)
        continue;
      if (!newLine && !lab)
      {
        // cout << "ovde " << linebr;
        throw BadFileException();
      }
      else
      {
        newLine = false;
        switch (token)
        {
        case 1: // DIRECTIVE
          lab = false;
          processDirective(string(i1.val), lex);
          break;
        case 3: // LABEL
          if (currSection == "")
          {
            throw NoSectionError();
          }
          lab = true;
          processLabel(string(i1.val));
          break;
        case 6: // WORD
          if (currSection == "")
          {
            throw NoSectionError();
          }
          lab = false;
          processInstruction(string(i1.val), lex);
          break;
        default:
          throw BadFileException();
          cout << "";
        }
      }
    }
  }
  catch (exception &e)
  {
    e.what();
    exit(-3);
  }
  file.close();
  // backpatching
  list<symb *> *lst = symTable->getEntries();

  for (auto it = lst->begin(); it != lst->end(); it++)
  {

    if ((*it)->name == (*it)->sect)
    {
      (*it)->size = sectionContent->at((*it)->name)->size();
    }

    frt *flst = (*it)->flink;
    // cout << (flst != 0)  << " " << (*it)->name<< endl;
    while (flst != 0)
    {
      for (auto it2 = relaTables->begin(); it2 != relaTables->end(); it2++)
      {
        for (auto it3 = (*it2).second->getEntries()->begin(); it3 != it2->second->getEntries()->end(); it3++)
        {
          if (flst->addr == (*it3)->offset && (*it)->name == (*it3)->symbol->name)
          {
            if ((*it)->bind == 'g')
            {
              //(*it3)->addend -= (*it)->value;
            }
            else
            {
              (*it3)->addend += (*it)->value;
              (*it3)->symbol = symTable->getSymbol((*it)->sect);
            }
            if ((*it)->sect == (*it2).first && (*it3)->typ == (string) "R_PC16")
            {
              auto iter = sectionContent->at((*it2).first)->begin();
              for (int i = 0; i < flst->addr; i++)
              {         // 1 2 3 a b 5
                iter++; //       ^
              }
              int dist = (*it)->value - flst->addr;
              sectionContent->at((*it2).first)->insert(iter, dist & 0xFF);
              // iter++;
              sectionContent->at((*it2).first)->insert(iter, ((dist >> 8) & 0xFF));
              // iter++;
              // cout << "U sekciji: " << (*it2).first << endl;
              iter = sectionContent->at((*it2).first)->erase(iter);
              iter = sectionContent->at((*it2).first)->erase(iter);
              it3 = it2->second->getEntries()->erase(it3);
              it3--;
            }
          }
        }
      }
      flst = flst->next;
    }
    for (auto it2 = relaTables->begin(); it2 != relaTables->end(); it2++)
    {
      for (auto it3 = (*it2).second->getEntries()->begin(); it3 != it2->second->getEntries()->end(); it3++)
      {
        if ((*it)->name == (*it3)->symbol->name)
        {
          if ((*it)->sect == (*it2).first && (*it3)->typ == (string) "R_PC16")
          {
            auto iter = sectionContent->at((*it2).first)->begin();
            for (int i = 0; i < (*it3)->offset; i++)
            {         // 1 2 3 a b 5
              iter++; //       ^
            }
            int dist = (*it)->value - (*it3)->offset;
            sectionContent->at((*it2).first)->insert(iter, dist & 0xFF);
            // iter++;
            sectionContent->at((*it2).first)->insert(iter, ((dist >> 8) & 0xFF));
            // iter++;
            // cout << "U sekciji: " << (*it2).first << endl;
            iter = sectionContent->at((*it2).first)->erase(iter);
            iter = sectionContent->at((*it2).first)->erase(iter);
            it3 = it2->second->getEntries()->erase(it3);
            it3--;
          }
        }
      }
    }
  }

  try
  {
    for (auto it = symTable->getEntries()->begin(); it != symTable->getEntries()->end(); it++)
    {
      if (!(*it)->isDef && (*it)->bind == 'l')
      {
        throw UndefinedLocalSymbol((*it)->name);
      }
    }
  }
  catch (UndefinedLocalSymbol &e)
  {
    e.what();
    exit(-6);
  }

  string s = outputFile.substr(0, outputFile.length() - 2);
  string sb = s + "_b.bin";
  int tmp = 0;
  char c = 0xF0;
  ;
  char tmpNum = (char)symTable->getEntries()->size();
  unsigned short pos = 0;
  outfile_b.open(sb, ios::binary);

  outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char)); //+1 1.
  c = 0x00;
  outfile_b.write(reinterpret_cast<const char *>(&pos), sizeof(short));   // +3 2. 3.
  outfile_b.write(reinterpret_cast<const char *>(&tmpNum), sizeof(char)); // +4  4. broj simbola u tabeli simbola
  tmpNum = (char)sectionContent->size();
  outfile_b.write(reinterpret_cast<const char *>(&tmpNum), sizeof(char)); // +5 5.
  c = 0x00;
  outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char)); // +7 6.
  for (int i = 0; i < tmpNum * 8 + 2; i++)
  {
    outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char)); // 7. 8. i ostalo
  }
  pos = tmpNum * 8 + 8;

  outfile.open(outputFile);
  // cout << "------------------------------------------" << endl;
  for (auto it = sectionContent->begin(); it != sectionContent->end(); ++it)
  {
    // cout << it->first << endl;
    outfile << it->first << endl;
    int n = 0;
    outfile_b.seekp((tmp + 1) * 8, ios::beg);
    c = (char)(symTable->getSymbol(it->first)->mId & 0xFF);
    outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
    c = (char)(pos & 0xFF);
    outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
    c = (char)(pos >> 8);
    outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
    c = (char)((unsigned short)it->second->size() & 0xFF);
    outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
    c = (char)((unsigned short)it->second->size() >> 8);
    outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
    outfile_b.seekp(0, ios::end);
    tmp++;
    pos += (unsigned short)it->second->size();
    for (auto it2 = it->second->begin(); it2 != it->second->end(); ++it2)
    {
      // printf("%02hhx ", *it2);
      std::stringstream sstream;
      sstream << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)(*it2 - 0);
      std::string result = sstream.str();
      // cout << result.substr(result.length() - 2, result.length()) << " ";
      outfile << result.substr(result.length() - 2, result.length()) << " ";
      outfile_b << *it2;
      n++;
      // if(n == 4)
      // cout << "  ";
      // outfile << "  ";
      if (n == 8)
      {
        // cout << endl;
        outfile << endl;
        n = 0;
      }
    }
    // cout << endl;

    outfile << endl;
  }
  tmp = 0;
  // cout << "Rel Tables: " << endl;
  outfile << "Rel Tables: " << endl;
  for (auto it = relaTables->begin(); it != relaTables->end(); ++it)
  {
    // cout << it->first << endl;
    outfile << it->first << endl;
    // it->second->printRelaTable(&outfile);
    if (it->second->getEntries()->size() != 0)
    {
      outfile_b.seekp((tmp + 1) * 8 + 5, ios::beg);
      c = (char)(pos & 0xFF);
      outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
      c = (char)(pos >> 8);
      outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
      c = (char)((unsigned short)it->second->getEntries()->size() & 0xFF);
      outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
      outfile_b.seekp(0, ios::end);
    }
    tmp++;
    // cout << "Offset\tType\tSymbol\tAddend\n";
    outfile << "Offset\tType\tSymbol\tAddend\n";
    for (relEntry *ent : *(it->second->getEntries()))
    {
      // cout << ent->offset << "\t" << ent->typ << "\t" << ent->symbol->name << "\t" << ent->addend << endl;
      outfile << ent->offset << "\t" << ent->typ << "\t" << ent->symbol->name << "\t" << ent->addend << endl;
      // outfile_b << ent->offset << ent->typ << ent->symbol->name << ent->addend;
      outfile_b.write(reinterpret_cast<const char *>(&ent->offset), sizeof(ent->offset));
      outfile_b << ent->typ;
      outfile_b << (char)0;
      outfile_b << ent->symbol->name;
      outfile_b << (char)0;
      outfile_b.write(reinterpret_cast<const char *>(&ent->addend), sizeof(ent->addend));
      pos += 6 + ent->typ.length() + ent->symbol->name.length();
    }
    // cout << endl;
    outfile << endl;
  }
  // cout << endl;
  outfile << endl;
  // symTable->printSymTable();
  // cout << "Num\tValue\tSize\tType\tBind\tNdx\tName" << endl;
  // cout << "0\t0\t0\tNOTYP\tl\tUND\t\t" << endl;
  outfile << "Num\tValue\tSize\tType\tBind\tNdx\tName" << endl;
  outfile << "0\t0\t0\tNOTYP\tl\tUND\t\t" << endl;
  outfile_b.seekp(1, ios::beg);
  c = (char)(pos & 0xFF);
  outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
  c = (char)(pos >> 8);
  outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
  outfile_b.seekp(0, ios::end);
  for (symb *ent : *symTable->getEntries())
  {
    // cout << ent->mId << "\t" << ent->value << "\t" << ent->size << "\t" << ent->typ << "\t" << ent->bind << "\t" << ent->sect << "\t" << ent->name << "\t";
    outfile << ent->mId << "\t" << ent->value << "\t" << ent->size << "\t" << ent->typ << "\t" << ent->bind << "\t" << ent->sect << "\t" << ent->name << "\t";
    // outfile_b << ent->mId << ent->value << ent->size << ent->typ << ent->bind << ent->sect << ent->name;
    outfile_b.write(reinterpret_cast<const char *>(&ent->mId), sizeof(ent->mId));
    outfile_b.write(reinterpret_cast<const char *>(&ent->value), sizeof(ent->value));
    outfile_b.write(reinterpret_cast<const char *>(&ent->size), sizeof(ent->size));
    outfile_b << ent->typ;
    outfile_b << (char)0;
    outfile_b << ent->bind;
    outfile_b << ent->sect;
    outfile_b << (char)0;
    outfile_b << ent->name;
    outfile_b << (char)0;
    // cout << endl;
    outfile << endl;
  }
  outfile.close();
  outfile_b.close();

  delete lex;
  // cout << "Upseh\n";
  return 0;
}
