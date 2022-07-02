#include "../inc/emulator.hpp"
#include <string>
#include <fstream>
#include <cstdlib>
#include <bitset>
#include <chrono>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <mutex>

bool isHalt = false;
bool interTerm = false;
bool interTimer = false;
bool inReg = false;
bool isCall = false;
char *memory = new char[65536];
Cpu *cpu = new Cpu();
mutex mtx;
// struct pollfd pfdFake;

void terminalIn()
{

  char c;
  int retval;
  struct pollfd pfd;
  pfd.fd = 0;
  pfd.events = POLLIN;

  while (!isHalt)
  {
    retval = poll(&pfd, 1, 1);

    if (retval > 0)
    {
      c = fgetc(stdin);

      if (c != (char)1)
      {
        mtx.lock();
        memory[term_in] = c;
        mtx.unlock();
        if ((cpu->psw & I_FLAG) != 0 && ((cpu->psw & Tl_FLAG) != 0))
        {
          interTerm = true;
        }
      }
    }
  }
}

void terminalOut()
{
  char lastC = (char)0;
  while (!isHalt || inReg)
  {
    if (inReg)
    {
      // cout << "usao\n";
      mtx.lock();
      lastC = memory[term_out];
      mtx.unlock();
      write(1, &lastC, sizeof(char));
      std::flush(cout);
      inReg = false;
    }
  }
}

void timer()
{
  while (!isHalt)
  {
    switch (memory[tim_cfg])
    {
    case 0x0:
      this_thread::sleep_for(chrono::milliseconds(500));
      break;
    case 0x1:
      this_thread::sleep_for(chrono::milliseconds(1000));
      break;
    case 0x2:
      this_thread::sleep_for(chrono::milliseconds(1500));
      break;
    case 0x3:
      this_thread::sleep_for(chrono::milliseconds(2000));
      break;
    case 0x4:
      this_thread::sleep_for(chrono::milliseconds(5000));
      break;
    case 0x5:
      this_thread::sleep_for(chrono::milliseconds(10000));
      break;
    case 0x6:
      this_thread::sleep_for(chrono::milliseconds(30000));
      break;
    case 0x7:
      this_thread::sleep_for(chrono::milliseconds(60000));
      break;
    default:
      exit(-5);
      break;
    }
    // cout << "Memory[term_out]: ";
    // printf("%02hhx\n", memory[term_out]);
    if ((cpu->psw & I_FLAG) != 0 && ((cpu->psw & Tr_FLAG) != 0))
    {
      interTimer = true;
    }
  }
}

int main(int argc, const char *argv[])
{
  bool isHex = false;

  char c;
  ifstream file;

  unsigned char op, tmp, tmp1;
  short val1;
  unsigned short val1addr;
  unsigned short startAddr = 0;
  unsigned short ivtAddr = 0;
  unsigned short sectCnt = 0;
  unsigned short tempAddr = 0, tempSize = 0, tempPos = 0;
  unsigned short oldPc = 0;
  struct termios term;
  tcgetattr(fileno(stdin), &term);

  term.c_lflag &= ~ICANON;
  term.c_lflag &= ~ECHO;
  term.c_cc[VMIN] = 1; // 1
  term.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &term);

  try
  {
    if (argc != 2)
    {
      throw BadUsageException();
    }
    string fileName = argv[1];
    /*if (fileName.substr(fileName.length() - 4, fileName.length()) != ".hex")
    {
      throw BadInputFileException();
    }*/

    file.open(fileName, ios::binary | ios::in);
    file.read(&c, sizeof(char));
    if (c != (char)0xFF)
    {
      throw BadInputFileException();
    }
    file.read(&c, sizeof(char));
    sectCnt = (unsigned short)c;

    for (int i = 0; i < sectCnt; i++)
    {
      file.seekg(2 + i * 4, ios::beg);
      file.read((char *)&tempAddr, sizeof(short));
      file.read((char *)&tempSize, sizeof(short));
      // cout << tempAddr << " " << tempSize << endl;
      file.seekg(2 + sectCnt * 4 + tempPos, ios::beg);
      tempPos += tempSize;
      for (int j = 0; j < tempSize; j++)
      {
        file.read(&c, sizeof(char));
        memory[tempAddr + j] = c;
      }
    }
  }
  catch (BadInputFileException &e)
  {
    e.what();
    exit(-1);
  }
  catch (BadUsageException &ue)
  {
    ue.what();
    exit(-1);
  }
  catch (exception &e)
  {
    cout << "Nepoznata greska\n";
    exit(-1);
  }
  thread *t1 = new thread(terminalIn);
  //thread *t2 = new thread(terminalOut);
  thread *t3 = new thread(timer);
  cpu->regs[7] = (unsigned short)memory[0];
  cpu->regs[7] = ((unsigned short)memory[1] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
  cpu->psw |= I_FLAG;
  cpu->psw |= Tl_FLAG;
  cpu->psw |= Tr_FLAG;

  // pfdFake.fd = eventfd(1,ios::in);
  // pfdFake.events = POLLIN;
  while (!isHalt)
  {
    op = memory[(unsigned short)cpu->regs[7]++];
    switch (op)
    {
    case 0x00: // halt
      isHalt = true;
      break;
    case 0x10: // int
      //cout << "Int\n";

      tmp = memory[(unsigned short)cpu->regs[7]++];
      tmp = tmp >> 4;
      //cout << (unsigned short)tmp << endl;
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);

      cpu->regs[7] = (unsigned short)memory[cpu->regs[tmp] * 2];
      cpu->regs[7] = ((unsigned short)(memory[cpu->regs[tmp] * 2 + 1]) << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
      //cout << (unsigned short)cpu->regs[7];
      break;
    case 0x20: // iret
      //cout << "iret\n";
      cpu->psw = (unsigned short)memory[(unsigned short)cpu->regs[6]++];
      cpu->psw = (cpu->psw & 0xFF) | ((unsigned short)memory[(unsigned short)cpu->regs[6]++] << 8);
      cpu->regs[7] = (unsigned short)memory[(unsigned short)cpu->regs[6]++];
      cpu->regs[7] = ((unsigned short)cpu->regs[7] & 0xFF) | ((unsigned short)memory[(unsigned short)cpu->regs[6]++] << 8);
      break;
    case 0x30: // call

      isCall = true;
    case 0x50: // jmp - ovo proveri jer sam stavio da call samo propadne u jmp deo ali nisam siguran da to tako radi
      //cout << "call ili jmp\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      tmp1 = memory[(unsigned short)cpu->regs[7]++];
      if ((tmp1 & 0xF) == 0)
      {
        // neposredno
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        oldPc = (unsigned short)cpu->regs[7];
        cpu->regs[7] = val1;
        // cout << "pc " << (unsigned short)cpu->regs[7];
      }
      else if (tmp1 == 21)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        oldPc = (unsigned short)cpu->regs[7];
        cpu->regs[7] = (unsigned short)cpu->regs[7] - 2 + val1;
      }
      else if (tmp1 == 4)
      {
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        oldPc = (unsigned short)cpu->regs[7];
        cpu->regs[7] = (unsigned short)memory[val1];
        cpu->regs[7] = (unsigned short)memory[val1 + 1] << 8 | (unsigned short)cpu->regs[7] & 0xFF;
      }
      else if (tmp1 == 1)
      {
        tmp = tmp & 0x0F;
        
          oldPc = (unsigned short)cpu->regs[7];
          cpu->regs[7] = (unsigned short)cpu->regs[tmp % 8];
        
      }
      else if (tmp1 == 2)
      {
        tmp = tmp & 0x0F;
        
          oldPc = (unsigned short)cpu->regs[7];
          cpu->regs[7] = (unsigned short)memory[cpu->regs[tmp % 8]];
          cpu->regs[7] = ((unsigned short)memory[cpu->regs[tmp % 8] + 1] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
        
      }
      else if (tmp1 == 3)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        tmp = tmp & 0x0F;
        
          oldPc = (unsigned short)cpu->regs[7];
          cpu->regs[7] = memory[(cpu->regs[tmp % 6] + val1) % 65536];
        
      }
      else
      {
        // throw UnknownAddresing();
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
        cpu->regs[7] = memory[ivtAddr + 2];
        cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
        cpu->psw &= 0x7FFF;
        interTimer = 0;
      }
      if (isCall)
      {
        isCall = false;
        // cout << "pre " << oldPc;
        //cout << "here " << oldPc << endl; 
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(oldPc >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(oldPc & 0xFF);
      }
      break;
    case 0x40: // ret
      //cout << "ret\n";
      cpu->regs[7] = (unsigned short)memory[(unsigned short)cpu->regs[6]++];
      cpu->regs[7] = ((unsigned short)cpu->regs[7] & 0xFF) | ((unsigned short)memory[(unsigned short)cpu->regs[6]++] << 8);
      //cout << "ret pc " << (unsigned short)cpu->regs[7] << endl; 
      // cout << "\nposle " << (unsigned short)cpu->regs[7];
      break;
    case 0x51: // jeq
      //cout << "jeq\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      tmp1 = memory[(unsigned short)cpu->regs[7]++];

      if ((tmp1 & 0xF) == 0)
      {
        // neposredno
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if ((cpu->psw & Z_FLAG) != 0)
          cpu->regs[7] = (unsigned short)val1;
      }
      else if (tmp1 == 21)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if ((cpu->psw & Z_FLAG) != 0)
          cpu->regs[7] = (unsigned short)cpu->regs[7] - 2 + val1;
      }
      else if (tmp1 == 4)
      {
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if ((cpu->psw & Z_FLAG) != 0)
        {
          cpu->regs[7] = (unsigned short)memory[val1];
          cpu->regs[7] = (unsigned short)memory[val1 + 1] << 8 | (unsigned short)cpu->regs[7] & 0xFF;
        }
      }
      else if (tmp1 == 1)
      {
        if ((cpu->psw & Z_FLAG) != 0)
        {
          tmp = tmp & 0x0F;
          
            cpu->regs[7] = cpu->regs[tmp % 8];
          
        }
      }
      else if (tmp1 == 2)
      {
        if ((cpu->psw & Z_FLAG) != 0)
        {
          tmp = tmp & 0x0F;
          
            cpu->regs[7] = (unsigned short)memory[cpu->regs[tmp % 8]];
            cpu->regs[7] = ((unsigned short)memory[cpu->regs[tmp % 8] + 1] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          
        }
      }
      else if (tmp1 == 3)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if ((cpu->psw & Z_FLAG) != 0)
        {
          tmp = tmp & 0x0F;
          
            cpu->regs[7] = (unsigned short)memory[(cpu->regs[tmp % 6] + val1) % 65536];
            cpu->regs[7] = ((unsigned short)memory[(cpu->regs[tmp % 6] + val1 + 1) % 65536] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          
        }
      }
      else
      {
        // throw UnknownAddresing();
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
        cpu->regs[7] = memory[ivtAddr + 2];
        cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
        cpu->psw &= 0x7FFF;
        interTimer = 0;
      }

      break;
    case 0x52: // jne
      //cout << "jne\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      tmp1 = memory[(unsigned short)cpu->regs[7]++];
      if ((tmp1 & 0xF) == 0)
      {
        // neposredno
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if ((cpu->psw & Z_FLAG) == (short)0)
          cpu->regs[7] = val1;
      }
      else if (tmp1 == 21)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if ((cpu->psw & Z_FLAG) == (short)0)
          cpu->regs[7] = (unsigned short)cpu->regs[7] - 2 + val1;
      }
      else if (tmp1 == 4)
      {
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if ((cpu->psw & Z_FLAG) == (short)0)
        {
          cpu->regs[7] = (unsigned short)memory[val1];
          cpu->regs[7] = (unsigned short)memory[val1 + 1] << 8 | (unsigned short)cpu->regs[7] & 0xFF;
        }
      }
      else if (tmp1 == 1)
      {
        if ((cpu->psw & Z_FLAG) == (short)0)
        {
          tmp = tmp & 0x0F;
          
          cpu->regs[7] = cpu->regs[tmp % 6];
          
        }
      }
      else if (tmp1 == 2)
      {
        if ((cpu->psw & Z_FLAG) == (short)0)
        {
          tmp = tmp & 0x0F;
          
            cpu->regs[7] = (unsigned short)memory[cpu->regs[tmp % 8]];
            cpu->regs[7] = ((unsigned short)memory[cpu->regs[tmp % 8]] + 1) | ((unsigned short)cpu->regs[7] & 0xFF);
          
        }
      }
      else if (tmp1 == 3)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        tmp = tmp & 0x0F;
        if ((cpu->psw & Z_FLAG) == (short)0)
        {
          
            cpu->regs[7] = (unsigned short)memory[(cpu->regs[tmp % 6] + val1) % 65536];
            cpu->regs[7] = ((unsigned short)memory[(cpu->regs[tmp % 6] + val1 + 1) % 65536] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          
        }
      }
      else
      {
        // throw UnknownAddresing();
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
        cpu->regs[7] = memory[ivtAddr + 2];
        cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
        cpu->psw &= 0x7FFF;
        interTimer = 0;
      }

      break;
    case 0x53: // jgt
      //cout << "jgt\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      tmp1 = memory[(unsigned short)cpu->regs[7]++];

      if ((tmp1 & 0xF) == 0)
      {
        // neposredno
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if (((cpu->psw & Z_FLAG) == 0) && (((cpu->psw & O_FLAG) >> 1) == ((cpu->psw & N_FLAG) >> 3)))
        {
          cpu->regs[7] = val1;
        }
      }
      else if (tmp1 == 21)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if (((cpu->psw & Z_FLAG) == 0) && (((cpu->psw & O_FLAG) >> 1) == ((cpu->psw & N_FLAG) >> 3)))
          cpu->regs[7] = (unsigned short)cpu->regs[7] - 2 + val1;
      }
      else if (tmp1 == 4)
      {
        val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if (((cpu->psw & Z_FLAG) == 0) && (((cpu->psw & O_FLAG) >> 1) == ((cpu->psw & N_FLAG) >> 3)))
        {
          cpu->regs[7] = (unsigned short)memory[val1];
          cpu->regs[7] = (unsigned short)memory[val1 + 1] << 8 | (unsigned short)cpu->regs[7] & 0xFF;
        }
      }
      else if (tmp1 == 1)
      {
        if (((cpu->psw & Z_FLAG) == 0) && (((cpu->psw & O_FLAG) >> 1) == ((cpu->psw & N_FLAG) >> 3)))
        {
          tmp = tmp & 0x0F;
          
            cpu->regs[7] = cpu->regs[tmp % 6];
          
        }
      }
      else if (tmp1 == 2)
      {
        if (((cpu->psw & Z_FLAG) == 0) && (((cpu->psw & O_FLAG) >> 1) == ((cpu->psw & N_FLAG) >> 3)))
        {
          tmp = tmp & 0x0F;
          
            cpu->regs[7] = (unsigned short)memory[cpu->regs[tmp % 6]];
            cpu->regs[7] = ((unsigned short)memory[cpu->regs[tmp % 6] + 1] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          
        }
      }
      else if (tmp1 == 3)
      {
        val1 = (short)memory[(unsigned short)cpu->regs[7]++];
        val1 = ((short)memory[(unsigned short)cpu->regs[7]++] << 8) | val1 & 0xFF;
        if (((cpu->psw & Z_FLAG) == 0) && (((cpu->psw & O_FLAG) >> 1) == ((cpu->psw & N_FLAG) >> 3)))
        {
          tmp = tmp & 0x0F;
         
            cpu->regs[7] = (unsigned short)memory[(cpu->regs[tmp % 6] + val1) % 65536];
            cpu->regs[7] = ((unsigned short)memory[(cpu->regs[tmp % 6] + val1 + 1) % 65536] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          
        }
      }
      else
      {
        // throw UnknownAddresing();

        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
        cpu->regs[7] = memory[ivtAddr + 2];
        cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
        cpu->psw &= 0x7FFF;
        interTimer = 0;
      }
      break;
    case 0x60: // xchg
      //cout << "xchg\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      val1 = cpu->regs[tmp >> 4];
      cpu->regs[tmp >> 4] = cpu->regs[tmp & 0xF];
      cpu->regs[tmp & 0xFF] = val1;
      break;
    case 0x70: // add
      //cout << "add\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      cpu->regs[tmp >> 4] += cpu->regs[tmp & 0xF];
      break;
    case 0x71: // sub
      //cout << "sub\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      cpu->regs[tmp >> 4] -= cpu->regs[tmp & 0xF];
      break;
    case 0x72: // mul
      //cout << "mul\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      cpu->regs[tmp >> 4] *= cpu->regs[tmp & 0xF];
      break;
    case 0x73: // div
      //cout << "div\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      if (cpu->regs[tmp & 0xF] == 0)
      {
        // throw DividingWithZero();

        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
        cpu->regs[7] = memory[ivtAddr + 2];
        cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
        cpu->psw &= 0x7FFF;
        interTimer = 0;
      } else {
        // cout << cpu->regs[tmp >> 4] << " " << cpu->regs[tmp & 0xF];
        cpu->regs[tmp >> 4] = cpu->regs[tmp >> 4] / cpu->regs[tmp & 0xF];
      }
      break;
    case 0x74: // cmp, treba proveriti ove flegove
      //cout << "cmp\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      if (abs(cpu->regs[tmp >> 4]) < abs(cpu->regs[tmp & 0xF]))
      { // C
        cpu->psw |= 0x4;
      }
      else
      {
        cpu->psw &= 0xFFFB;
      }
      if ((int)cpu->regs[tmp >> 4] + (int)cpu->regs[tmp & 0xF] > 32767)
      { // O
        cpu->psw |= 0x2;
      }
      else
      {
        cpu->psw &= 0xFFFD;
      }
      val1 = cpu->regs[tmp >> 4] - cpu->regs[tmp & 0xF];
      if (val1 == 0)
      { // Z
        cpu->psw |= 0x1;
      }
      else
      {
        cpu->psw &= 0xFFFE;
      }
      if (val1 < 0)
      { // N
        cpu->psw |= 0x8;
      }
      else
      {
        cpu->psw &= 0xFFF7;
      }
      // setovanje flegova
      // cout << "C: " << ((cpu->psw & C_FLAG) >> 2) << endl;
      // cout << "O: " << ((cpu->psw & O_FLAG) >> 1) << endl;
      // cout << "Z: " << (cpu->psw & Z_FLAG) << endl;
      // cout << "N: " << ((cpu->psw & N_FLAG) >> 3) << endl;
      break;
    case 0x80: // not
      //cout << "not\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      cpu->regs[tmp >> 4] = ~cpu->regs[tmp >> 4];
      break;
    case 0x81: // and
      //cout << "and\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      cpu->regs[tmp >> 4] &= cpu->regs[tmp & 0xF];
      break;
    case 0x82: // or
      //cout << "or\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      cpu->regs[tmp >> 4] |= cpu->regs[tmp & 0xF];
      break;
    case 0x83: // xor
      //cout << "xor\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      cpu->regs[tmp >> 4] ^= cpu->regs[tmp & 0xF];
      break;
    case 0x84: // test
      //cout << "test\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      val1 = cpu->regs[tmp >> 4] & cpu->regs[tmp & 0xF];
      if (val1 == 0)
      { // Z
        cpu->psw |= 0x1;
      }
      else
      {
        cpu->psw &= 0xFFFE;
      }
      if (val1 < 0)
      { // N
        cpu->psw |= 0x8;
      }
      else
      {
        cpu->psw &= 0xFFF7;
      }
      // setovanje flegova
      break;
    case 0x90: // shl
      //cout << "shl\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      if (cpu->regs[tmp & 0xF] > 0)
      {
        val1 = 0x8000 >> (cpu->regs[tmp & 0xF] - 1);
        val1 = val1 & cpu->regs[tmp >> 4];
        if (val1 != 0)
        { // c
          cpu->psw |= 0x4;
        }
        else
        {
          cpu->psw |= 0xFFFB;
        }
        cpu->regs[tmp >> 4] = cpu->regs[tmp >> 4] << (unsigned short)cpu->regs[tmp & 0xF];
      }
      if (cpu->regs[tmp >> 4] == 0)
      { // z
        cpu->psw |= 0x1;
      }
      else
      {
        cpu->psw &= 0xFFFE;
      }
      if (cpu->regs[tmp >> 4] < 0)
      { // n
        cpu->psw |= 0x8;
      }
      else
      {
        cpu->psw &= 0xFFF7;
      }

      // setovanje flegova
      break;
    case 0x91: // shr
      //cout << "shr\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      if (cpu->regs[tmp & 0xF] > 0)
      {
        val1 = 0x0001 << (cpu->regs[tmp & 0xF] - 1);
        val1 = val1 & cpu->regs[tmp >> 4];
        if (val1 != 0)
        { // c
          cpu->psw |= 0x4;
        }
        else
        {
          cpu->psw &= 0xFFFB;
        }
        cpu->regs[tmp >> 4] = cpu->regs[tmp >> 4] >> (unsigned short)cpu->regs[tmp & 0xF];
      }
      if (cpu->regs[tmp >> 4] == 0)
      { // z
        cpu->psw |= 0x1;
      }
      else
      {
        cpu->psw &= 0xFFFE;
      }
      if (cpu->regs[tmp >> 4] < 0)
      { // n
        cpu->psw |= 0x8;
      }
      else
      {
        cpu->psw &= 0xFFF7;
      }
      // setovanje flegova
      break;
    case 0xA0: // ldr, pop
      //cout << "ldr ili pop\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      if (tmp >> 4 == (unsigned short)7)
      {
        // throw BadOperandCombination();

        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
        cpu->regs[7] = memory[ivtAddr + 2];
        cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
        cpu->psw &= 0x7FFF;
        interTimer = 0;
      }
      tmp1 = memory[(unsigned short)cpu->regs[7]++];
      if (tmp1  == 66)
      {
        // pop
        cpu->regs[tmp >> 4] = (unsigned short)memory[(unsigned short)cpu->regs[6]++];
        cpu->regs[tmp >> 4] = ((unsigned short)memory[(unsigned short)cpu->regs[6]++] << 8) | (cpu->regs[tmp >> 4] & 0xFF);
      }
      else
      {
        if (tmp1 == 0)
        {
          val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
          val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | (val1 & 0xFF);
          if (tmp >> 4 == 6)
          {
            cpu->regs[6] = (unsigned short)val1;
          }
          else
          {
            cpu->regs[tmp >> 4] = val1;
          }
        }
        else if (tmp1 == 19)
        {
          val1 = memory[(unsigned short)cpu->regs[7]++];
          val1 = (memory[(unsigned short)cpu->regs[7]++] << 8) | (val1 & 0xFF);
          // cout << "pc rel " << val1 << endl;
          cpu->regs[tmp >> 4] = (unsigned short)memory[(unsigned short)cpu->regs[7] - 2 + val1];
          cpu->regs[tmp >> 4] = ((unsigned short)memory[(unsigned short)cpu->regs[7] - 2 + val1 + 1] << 8) | (cpu->regs[tmp >> 4] & 0xFF);
        }
        else if (tmp1 == 3)
        {
          val1 = memory[(unsigned short)cpu->regs[7]++];
          val1 = (memory[(unsigned short)cpu->regs[7]++] << 8) | (val1 & 0xFF);
          cpu->regs[tmp >> 4] = (unsigned short)memory[(unsigned short)cpu->regs[tmp & 0xF] + val1];
          cpu->regs[tmp >> 4] = ((unsigned short)memory[(unsigned short)cpu->regs[tmp & 0xF] + val1 + 1] << 8) | (cpu->regs[tmp >> 4] & 0xFF);
        }
        else if (tmp1 == 2)
        {
          // cout << endl << (tmp>>4) << " " << (tmp & 0xF) << endl;
          // cout << cpu->regs[tmp >> 4] << " " << cpu->regs[tmp & 0xF];
          cpu->regs[tmp >> 4] = ((unsigned short)memory[(unsigned short)cpu->regs[tmp & 0xF] + 1] << 8) | ((unsigned short)memory[(unsigned short)cpu->regs[tmp & 0xF]] & 0xFF);
        }
        else if (tmp1 == 4)
        {
          val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
          val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | (val1 & 0xFF);
          if ((unsigned short)val1 == 0xFF02 || (unsigned short)val1 == 0xFF00)
          {
            mtx.lock();
            cpu->regs[tmp >> 4] = (short)memory[(unsigned short)val1];
            mtx.unlock();
          }
          else
          {
            cpu->regs[tmp >> 4] = (short)memory[(unsigned short)val1];
            cpu->regs[tmp >> 4] = ((short)memory[(unsigned short)val1 + 1] << 8) | (cpu->regs[tmp >> 4] & 0xFF);
          }
          /*cpu->regs[tmp >> 4] = (short)memory[(unsigned short)val1];
          cpu->regs[tmp >> 4] = ((short)memory[(unsigned short)val1 + 1] << 8) | (cpu->regs[tmp >> 4] & 0xFF);
          if ((unsigned short)val1 == 0xFF02 || (unsigned short)val1 == 0xFF00)
          {
            mtx.unlock();
          }*/
          // printf("memory[%04hhx](%04hhx) -> reg[%d](%d)\n", val1,memory[val1],  tmp>>4, cpu->regs[tmp>>4]);
          // if((unsigned short)val1 == 0xFF02) {
          //   cout << "\nldr " << cpu->regs[tmp>>4] << " - " << memory[(unsigned short)val1];
          // }
        }
        else if (tmp1 == 1)
        {
          
          cpu->regs[tmp >> 4] = cpu->regs[tmp & 0xF]; // ovo zabraniti?
        }
        else
        {
          // throw UnknownAddresing();

          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
          cpu->regs[7] = memory[ivtAddr + 2];
          cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          cpu->psw &= 0x7FFF;
          interTimer = 0;
        }
      }
      break;
    case 0xB0: // str, push
      //cout << "str ili push\n";
      tmp = memory[(unsigned short)cpu->regs[7]++];
      tmp1 = memory[(unsigned short)cpu->regs[7]++];
      if (tmp1 == 18)
      {
        // push
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->regs[tmp >> 4] >> 8);
        cpu->regs[6]--;
        memory[(unsigned short)cpu->regs[6]] = (char)(cpu->regs[tmp >> 4] & 0xFF);
      }
      else
      {
        if (tmp1 == 0)
        {
          // throw BadOperandCombination();

          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
          cpu->regs[7] = memory[ivtAddr + 2];
          cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          cpu->psw &= 0x7FFF;
          interTimer = 0;
        }
        else if (tmp1 == 19)
        { // ovde isto proveriti da li moze da reg bude pc i sp i to
          val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
          val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | (val1 & 0xFF);
          memory[(unsigned short)cpu->regs[7] - 2 + (unsigned short)val1] = (char)(cpu->regs[tmp >> 4] & 0xFF);
          memory[(unsigned short)cpu->regs[7] - 2 + (unsigned short)val1 + 1] = (char)(cpu->regs[tmp >> 4] >> 8);
          if ((unsigned short)cpu->regs[7] - 2 + val1 == 0xFF00)
          {
            inReg = true;
          }
        }
        else if (tmp1 == 3)
        {
          val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
          val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | (val1 & 0xFF);
          memory[(unsigned short)cpu->regs[tmp & 0xF] + (unsigned short)val1] = (char)(cpu->regs[tmp >> 4] & 0xFF);
          memory[(unsigned short)cpu->regs[tmp & 0xF] + (unsigned short)val1 + 1] = (char)(cpu->regs[tmp >> 4] >> 8);
          if ((unsigned short)(cpu->regs[tmp & 0xF] + val1) == 0xFF00)
          {
            inReg = true;
          }
        }
        else if (tmp1 == 2)
        {
          // cpu->regs[tmp>>4] = ((short)memory[cpu->regs[tmp&0xF]+1] << 8)|((short)memory[cpu->regs[tmp & 0xF]] & 0xFF);
          memory[(unsigned short)cpu->regs[tmp & 0xF]] = (char)(cpu->regs[tmp >> 4] & 0xFF);
          memory[(unsigned short)cpu->regs[tmp & 0xF] + 1] = (char)(cpu->regs[tmp >> 4] >> 8);
          if ((unsigned short)(cpu->regs[tmp & 0xF]) == 0xFF00)
          {
            inReg = true;
          }
        }
        else if (tmp1 == 4)
        {
          val1 = (unsigned short)memory[(unsigned short)cpu->regs[7]++];
          val1 = ((unsigned short)memory[(unsigned short)cpu->regs[7]++] << 8) | ((unsigned short)val1 & 0xFF);
          if ((unsigned short)val1 == 0xFF00 || (unsigned short)val1 == 0xFF02)
          {
            mtx.lock();
            memory[(unsigned short)val1] = (char)(cpu->regs[tmp >> 4] & 0xFF);
            inReg = true;
            mtx.unlock();
          }
          else
          {
            memory[(unsigned short)val1] = (char)(cpu->regs[tmp >> 4] & 0xFF);
            memory[(unsigned short)val1 + 1] = (char)(cpu->regs[tmp >> 4] >> 8);
          }
          /*if ((unsigned short)val1 == 0xFF00 || (unsigned short)val1 == 0xFF02)
          {
            // cout << "\nstr " << cpu->regs[tmp>>4] << " - " << memory[(unsigned short)val1] << endl;
            // cout << memory[0xFF00];
            mtx.unlock();
            if ((unsigned short)val1 == 0xFF00)
              inReg = true;
            //while (inReg)
             // ;
          }*/
        }
        else if (tmp1 == 1)
        {
          
          cpu->regs[tmp & 0xF] = cpu->regs[tmp >> 8]; // ovo zabraniti?
        }
        else
        {
          // throw UnknownAddresing();

          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
          cpu->regs[6]--;
          memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
          cpu->regs[7] = memory[ivtAddr + 2];
          cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
          cpu->psw &= 0x7FFF;
          interTimer = 0;
        }
      }
      break;
    default:
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
      cpu->regs[6]--;
      memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
      cpu->regs[7] = memory[ivtAddr + 2];
      cpu->regs[7] = (memory[ivtAddr + 3] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
      cpu->psw &= 0x7FFF;
      interTimer = 0;
    }
    if (inReg)
    {
      // cout << "usao\n";
      mtx.lock();
      write(1, &memory[term_out], sizeof(char));
      mtx.unlock();
      inReg = false;
    }
    if (interTerm)
    {
      cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
            cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
            cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
            cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
      cpu->regs[7] = memory[ivtAddr + 6];
      cpu->regs[7] = (memory[ivtAddr + 7] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
      cpu->psw &= 0x7FFF;
      interTerm = 0;
    }
    else if (interTimer)
    {
      cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] >> 8);
            cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)((unsigned short)cpu->regs[7] & 0xFF);
            cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw >> 8);
            cpu->regs[6]--;
            memory[(unsigned short)cpu->regs[6]] = (char)(cpu->psw & 0xFF);
      cpu->regs[7] = memory[ivtAddr + 4];
      cpu->regs[7] = (memory[ivtAddr + 5] << 8) | ((unsigned short)cpu->regs[7] & 0xFF);
      cpu->psw &= 0x7FFF;
      interTimer = 0;
    }
  }
  term.c_cc[VMIN] = 0;
  tcsetattr(0, TCSANOW, &term);
  char h = 1;
  ungetc((char)1, stdin);
  // char h = 1;
  // write(pfdFake.fd, &h, sizeof(char));
  t1->join();
  //t2->join();
  t3->join();

  cout << "\n--------------------------------------------------------------------" << endl;
  cout << "Emulated processor executed halt instruction" << endl;
  cout << "Emulated processor state: psw=0b";
  cout << bitset<16>(cpu->psw);
  cout << endl;
  for (int i = 0; i < 8; i++)
  {
    cout << "r" << i << "=0x";
    if (i < 6)
    {
      printf("%hX\t", cpu->regs[i]);
      if (i == 3)
        cout << endl;
    }
    else if (i == 6)
    {
      printf("%hX\t", (unsigned short)cpu->regs[6]);
    }
    else
    {
      printf("%hX\t", (unsigned short)cpu->regs[7]);
    }
  }

  term.c_lflag |= ICANON;
  term.c_lflag |= ECHO;
  tcsetattr(0, TCSADRAIN, &term);

  delete t1;
  //delete t2;
  delete t3;

  cout << "\nUspeh\n";
  /*
  {
  "files.associations": {
    "stdio.h": "c",
    "asembler.h": "c",
    "iosfwd": "cpp",
    "ostream": "cpp",
    "iostream": "cpp",
    "list": "cpp",
    "map": "cpp",
    "fstream": "cpp",
    "deque": "cpp",
    "string": "cpp",
    "vector": "cpp",
    "array": "cpp",
    "atomic": "cpp",
    "bit": "cpp",
    "*.tcc": "cpp",
    "cctype": "cpp",
    "clocale": "cpp",
    "cmath": "cpp",
    "cstdarg": "cpp",
    "cstddef": "cpp",
    "cstdint": "cpp",
    "cstdio": "cpp",
    "cstdlib": "cpp",
    "cstring": "cpp",
    "cwchar": "cpp",
    "cwctype": "cpp",
    "unordered_map": "cpp",
    "exception": "cpp",
    "algorithm": "cpp",
    "functional": "cpp",
    "iterator": "cpp",
    "memory": "cpp",
    "memory_resource": "cpp",
    "numeric": "cpp",
    "optional": "cpp",
    "random": "cpp",
    "string_view": "cpp",
    "system_error": "cpp",
    "tuple": "cpp",
    "type_traits": "cpp",
    "utility": "cpp",
    "initializer_list": "cpp",
    "istream": "cpp",
    "limits": "cpp",
    "new": "cpp",
    "sstream": "cpp",
    "stdexcept": "cpp",
    "streambuf": "cpp",
    "cinttypes": "cpp",
    "typeinfo": "cpp",
    "thread": "cpp",
    "chrono": "cpp",
    "bitset": "cpp",
    "condition_variable": "cpp",
    "ctime": "cpp",
    "ratio": "cpp",
    "mutex": "cpp",
    "iomanip": "cpp"
  }
}
  */

  return 0;
}