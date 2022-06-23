#ifndef _asembler_hpp_
#define _asembler_hpp_

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <exception>

using namespace std;

extern short id;
extern short currSectionOffset;
extern map<string, list<char>* >* sectionContent;
extern int linebr;


string currSection = "";

struct frt {
  unsigned short addr;
  frt* next;

  frt(unsigned short a) {
    addr = a;
    next = 0;
  }
};

struct symb {
  unsigned short mId;
  string name;
  string sect;
  string typ;
  short value;
  unsigned short size;
  char bind;
  bool isDef;
  frt* flink;

  symb(string n, string t = "NOTYP", short v = 0, char b = 'l') {
    name = n;
    value = v;
    bind = b;
    isDef = false;
    flink = 0;
    mId = id++;
    sect = "";
    size = 0;
    typ = t;
  }
};

struct relEntry {
  unsigned short offset;
  string typ;
  symb* symbol;
  short addend;
  //string sec;

  relEntry(unsigned short off, string t, symb* s, int add) {
    offset = off;
    typ = t;
    symbol = s;
    addend = add;
    //sec = se;
  }
};

class RelaTable {

  list<relEntry*> entries;

public:

  void addEntry(int off, string t, symb* s, int add) {
    entries.push_back(new relEntry(off, t, s, add));
  }

  list<relEntry*>* getEntries() {
    return &entries;
  }

  void printRelaTable(ofstream& outfile) {
    cout << "Offset\tType\tSymbol\tAddend\n";
    for(relEntry* ent : entries) {
      cout << ent->offset << "\t" << ent->typ << "\t" << ent->symbol->name << "\t" << ent->addend << endl;
    }
  }
  
};

extern map<string, RelaTable*>* relaTables;

class SymTable {

  //list<Elf64_Sym*> entries;
  list<symb*> entries;
  //list<string> symNames;
  
public:
  

  bool addSymbol(string name, int value = 0, bool isSection = false, bool isGlobalOrExtern = false, bool isLabel = false) {
    bool alreadyIn = false;

    for(symb* ent : entries) {
      if(ent->name == name) {
        alreadyIn = true;
        if(isSection || (ent->isDef && isLabel)) {
          cout << "Greska: Nije moguce vise puta definisati isti simbol!" << endl;
          exit(-5);
        }
        break;
      }
    }
    if(isSection) {
      currSection = name;
      currSectionOffset = 0;
      sectionContent->insert(make_pair(currSection, new list<char>()));
      relaTables->insert(make_pair(currSection, new RelaTable()));
    }

    if(!alreadyIn) {
      symb* newEnt = new symb(name, isSection?"SCTN":"NOTYP");
      if(currSection == "" && !isGlobalOrExtern) {
        cout << "Greska: Nepravilan ulazni fajl!" << endl;
        exit(-4);
      }
      if(isLabel || isSection) {
        newEnt->sect = currSection;
        newEnt->value = value;
        newEnt->isDef = true;
      }
      entries.push_back(newEnt);
      return true;
    } else {
      if(isLabel) {
        symb* ent = this->getSymbol(name);
        ent->sect = currSection;
        ent->isDef = true;
        ent->value = value;
      }
      return true;
    }
  
    return false;
  };

  symb* getSymbol(string name) {
    for(symb* ent : entries) {
      if(ent->name == name)
        return ent;
    }
    return 0;
  };

  void setGlobalType(string name) {
    symb* ent = this->getSymbol(name);
    ent->bind = 'g';
  };

  bool isDefined(string name) {
    return this->getSymbol(name)->isDef;
  }

  void addToFlink(string name, short off) {
    symb* ent = this->getSymbol(name);
    if(ent->flink != 0) {
      frt* tmp = ent->flink;
      while(tmp->next != 0) {
        tmp = tmp->next;
      }
      frt* newFrt = new frt(off);
      tmp->next = newFrt;

      //OVO PROVERI
    } else {
      ent->flink = new frt(off);
    }
  }

  list<symb*>* getEntries() {
    return &entries;
  }

  void printSymTable() {
    cout << "Num\tValue\tSize\tType\tBind\tNdx\tName" << endl;
    for(symb* ent : entries) {
      cout << ent->mId << "\t" << ent->value << "\t" << ent->size << "\t" << ent->typ << "\t" << ent->bind << "\t" << ent->sect << "\t" << ent->name << "\t";
      /*while(ent->flink != 0) {
        frt* fl = ent->flink;
        cout << fl->addr << " - ";
        fl = fl->next;
      }*/
      cout << endl;
    } 
  }

};

class BadUsageException : public exception {
public:
  const char* what() const throw(){
    cout << "Greska na liniji " << linebr << ": Losa upotreba asemblera!\n";
    return "";
  }
};

class UnknownFileException : public exception {
public:
  const char* what() const throw(){
    cout << "Greska na liniji " << linebr << ": Nepostojeci ulazni fajl!\n";
    return "";
  }
};

class BadFileException : public exception {
public:
  const char* what() const throw(){
    cout <<  "Greska na liniji " << linebr << ": Nepravilan ulazni fajl!\n";
    return "";
  }
};

class BadDirectiveException : public exception {
  string str;
public:
  BadDirectiveException(string s) {
    this->str = s;
  }

  const char* what() const throw(){
    cout << "Greska na liniji " << linebr << ": Nepravilna upotreba " << str << " direktive!\n";
    return "";
  }
};

class BadInstructionException : public exception {
  string str;
public:
  BadInstructionException(string s) {
    this->str = s;
  }

  const char* what() const throw(){
    cout << "Greska na liniji " << linebr << ": Nepravilna upotreba " << str << " instrukcije!\n";
    return "";
  }
};

class UndefinedLocalSymbol : public exception {
  string name;
public:
  UndefinedLocalSymbol(string n) {
    name = n;
  }
  const char* what() const throw(){
    cout << "Greska na liniji " << linebr << ": Nedefinisani lokalni simbol "  << name << endl;
    return "";
  }
};

class NoSectionError : public exception {

public:
  NoSectionError() {

  }
  const char* what() const throw(){
    cout << "Greska na liniji " << linebr << ": Nije definisana sekcija" << endl;
    return "";
  }
};


#endif