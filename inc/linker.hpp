#ifndef _linker_hpp_
#define _linker_hpp_

#include <exception>
#include <iostream>
#include <list>
#include <string>
#include "asembler.hpp"
using namespace std;




struct infoStruct {
  char indSect;
  short addr;
  short sz;
  short addrRelo;
  char szRelo;
};

struct symbLinker {
  unsigned short mId;
  string name;
  string sect;
  string typ;
  short value;
  unsigned short size;
  char bind;
  bool isDef;
  frt* flink;

  symbLinker(string n, string t = "NOTYP", short v = 0, char b = 'l') {
    name = n;
    value = v;
    bind = b;
    isDef = false;
    flink = 0;
    mId = 0;
    sect = "";
    size = 0;
    typ = t;
  }
};

struct relEntryLinker {
  unsigned short offset;
  string typ;
  string symbol;
  short addend;
  //string sec;

  relEntryLinker(unsigned short off, string t, string s, int add) {
    offset = off;
    typ = t;
    symbol = s;
    addend = add;
    //sec = se;
  }
};



class fileInfo {
public:
  list<infoStruct*>* str = new list<infoStruct*>();
  list<symbLinker*>* lstSymb = new list<symbLinker*>();
  map<string, list<relEntryLinker*>*>* lstRel = new map<string, list<relEntryLinker*>*>();
  map<string, list<char>*>* sectCont = new map<string, list<char>*>();
  unsigned short addrSymTable;
  unsigned short numOfSymbs;
  unsigned short numOfSects;

  


  void printInfo() {
    cout << "Info" << endl;
    for(auto it = str->begin(); it != str->end(); it++) {
      cout << (unsigned short)(*it)->indSect << "\t" << (*it)->addr << "\t" << (*it)->sz << "\t" << (*it)->addrRelo << (unsigned short)(*it)->szRelo << endl;
    }
    cout << endl << "Symbs" << endl;
    for(auto it = lstSymb->begin(); it != lstSymb->end(); it++) {
      cout << (*it)->mId << "\t" << (*it)->name << "\t" << (*it)->sect << "\t" << (*it)->value << "\t" << (*it)->size << "\t" << (*it)->bind << endl;
    }
    cout << endl << "Relo" << endl;
    for(auto it = lstRel->begin(); it != lstRel->end(); it++) {
      cout << it->first <<  endl;
      for(auto it2 = it->second->begin(); it2 != it->second->end(); it2++)
        cout <<  (*it2)->offset << "\t" << (*it2)->typ << "\t" << (*it2)->symbol << "\t" << (*it2)->addend << endl;
      cout << endl << endl;
    }
    int n = 0;
    for(auto it = sectCont->begin(); it != sectCont->end(); it++) {
      n = 0;
      cout << it->first << "<-\n";
      for(auto it2 = it->second->begin(); it2 != it->second->end(); it2++) {
        printf("%02hhx ", (*it2));
        n++;
        if(n == 4) {
          cout << "  ";
        }
        if(n == 8) {
          cout << endl;
          n = 0;
        }
      }
      cout << endl;
    }
    cout << endl << endl;
  }
};


class BadLinkerUsage : public exception {
public:
  const char* what() const throw() {
    cout << "Greska: Losa upotreba linkera!\n";
    return 0;
  }
};

class BadInputFile : public exception {
  string name;
public:
  BadInputFile(string n) {
    name = n;
  }
  const char* what() const throw() {
    cout << "Greska: Nije moguce otvoriti fajl sa imenom: " << name << "!\n";
    return 0;
  }
};

class FileError : public exception {
public:

  const char* what() const throw() {
    cout << "Greska: Neispravan ulazni relokacioni fajl!\n";
    return 0;
  }
};

class FileWontOpen : public exception {
public:

  const char* what() const throw() {
    cout << "Greska: Greska pri otvaranju fajla\n";
    return 0;
  }
};

class UndefinedSymb : public exception {
  string name;
public:
  UndefinedSymb(string n) {
    name = n;
  }
  const char* what() const throw() {
    cout << "Greska: Nedefinisani simbol: " << name << "!\n";
    return 0;
  }
};

class OverlapingSections : public exception {
  string name1, name2;
public:
  OverlapingSections(string n1, string n2) {
    name1 = n1;
    name2 = n2;
  }
  const char* what() const throw() {
    cout << "Greska: Dolazi do preklapanja sekcija " << name1 << " i " << name2 << "!\n";
    return 0;
  }
};


class MultipleDef : public exception {
  string name;
public:
  MultipleDef(string n) {
    name = n;
  }
  const char* what() const throw() {
    cout << "Greska: Visestruka definicija simbola: " << name << "!\n";
    return 0;
  }
};

class MultipleStarts : public exception {
public:

  const char* what() const throw() {
    cout << "Greska: Nije moguce definisati vise od 1 ulazne tacke u program!\n";
    return 0;
  }
};


#endif