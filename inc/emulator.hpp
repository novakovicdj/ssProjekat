#ifndef _emulator_hpp_
#define _emulator_hpp_

#include <exception>
#include <iostream>

using namespace std;

#define Z_FLAG 1
#define O_FLAG 2
#define C_FLAG 4
#define N_FLAG 8
#define Tr_FLAG 8192
#define Tl_FLAG 16384
#define I_FLAG 32768

#define term_in 0xFF02
#define term_out 0xFF00
#define tim_cfg 0xFF10


class Cpu {
public:
  short regs[8] = {0};

  unsigned short psw = 0;

  Cpu() {
    
  }
};


class BadUsageException : public exception {
public:
  const char* what() const throw(){
    cout << "Greska: Pogresna upotreba emulatora!\n";
    return 0;
  }
};

class BadInputFileException : public exception {
public:
  const char* what() const throw(){
    cout << "Greska: Los ulazni fajl u emulator!\n";
    return 0;
  }
};

class UnknownAddresing : public exception {
public:
  const char* what() const throw(){
    cout << "Greska: Nepoznati tip adresiranja!\n";
    return 0;
  }
};

class BadOperandCombination: public exception {
public:
  const char* what() const throw(){
    cout << "Greska: Losa kombinacija operanda i instrukcije!\n";
    return 0;
  }
};

class DividingWithZero: public exception {
public:
  const char* what() const throw(){
    cout << "Greska: Pokusaj deljenja sa 0!\n";
    return 0;
  }
};





#endif