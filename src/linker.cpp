#include "../inc/linker.hpp"
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <iomanip>

unsigned short startSymbLinker = 0;
string instrSect = "";
list<string> *sections = new list<string>();
map<string, list<char> *> *sectionContentLinker = new map<string, list<char> *>();
list<symbLinker *> *symTableLinker = new list<symbLinker *>();
list<fileInfo *> *fileInfos = new list<fileInfo *>();

void processFile(fstream *inF)
{
  char c;
  unsigned short num, num1;
  list<char> *listOfChars = new list<char>();
  string str = "", str1 = "";
  fileInfos->push_back(new fileInfo());
  auto iter = fileInfos->rbegin();
  try
  {
    inF->read(&c, sizeof(char));

    if (c != (char)0xf0)
    {
      throw FileError();
    }
    inF->read(&c, sizeof(char));
    (*iter)->addrSymTable = (unsigned short)c;
    inF->read(&c, sizeof(char));
    (*iter)->addrSymTable = ((unsigned short)c << 8) | ((*iter)->addrSymTable & 0xFF);
    inF->read(&c, sizeof(char));
    (*iter)->numOfSymbs = (unsigned short)c;
    inF->read(&c, sizeof(char));
    (*iter)->numOfSects = (unsigned short)c;
    inF->read(&c, sizeof(char));
    if (startSymbLinker != 0 && c != (char)0)
    {
      throw MultipleStarts();
    }
    for (int i = 0; i < 2; i++)
    { // unused bits
      inF->read(&c, sizeof(char));
    }

    for (int i = 0; i < (*iter)->numOfSects; i++)
    { // info deo
      (*iter)->str->push_back(new infoStruct());
      auto iter1 = (*iter)->str->rbegin();
      inF->read(&c, sizeof(char));
      (*iter1)->indSect = c;
      inF->read((char *)&num, sizeof(short));
      (*iter1)->addr = num;
      inF->read((char *)&num, sizeof(short));
      (*iter1)->sz = num;
      inF->read((char *)&num, sizeof(short));
      (*iter1)->addrRelo = num;
      inF->read(&c, sizeof(char));
      (*iter1)->szRelo = c;
    }

    inF->seekg((*iter)->addrSymTable, ios::beg);

    for (int i = 0; i < (*iter)->numOfSymbs; i++)
    {
      (*iter)->lstSymb->push_back(new symbLinker(""));
      auto iter1 = (*iter)->lstSymb->rbegin();
      inF->read((char *)&num, sizeof(short));
      (*iter1)->mId = num;
      inF->read((char *)&num, sizeof(short));
      (*iter1)->value = num;
      inF->read((char *)&num, sizeof(short));
      (*iter1)->size = num;
      (*iter1)->typ = "";
      while (true)
      {
        inF->read(&c, sizeof(char));
        if (c == (char)0x00)
          break;
        (*iter1)->typ = (*iter1)->typ + c;
      }
      inF->read(&c, sizeof(char));
      (*iter1)->bind = c;
      while (true)
      {
        inF->read(&c, sizeof(char));
        if (c == (char)0x00)
          break;
        (*iter1)->sect = (*iter1)->sect + c;
      }
      while (true)
      {
        inF->read(&c, sizeof(char));
        if (c == (char)0x00)
          break;
        (*iter1)->name = (*iter1)->name + c;
      }
    }

    for (auto it2 = (*iter)->str->begin(); it2 != (*iter)->str->end(); it2++)
    {
      string sectName;
      for (auto it3 = (*iter)->lstSymb->begin(); it3 != (*iter)->lstSymb->end(); it3++)
      {
        if ((*it2)->indSect == (*it3)->mId)
        {
          sectName = (*it3)->name;
          break;
        }
      }
      (*iter)->sectCont->insert(make_pair(sectName, new list<char>()));
      (*iter)->lstRel->insert(make_pair(sectName, new list<relEntryLinker *>()));
      inF->seekg((*it2)->addr, ios::beg);
      for (int i = 0; i < (*it2)->sz; i++)
      {
        inF->read(&c, sizeof(char));
        (*iter)->sectCont->at(sectName)->push_back(c);
      }
      inF->seekg((*it2)->addrRelo, ios::beg);
      str = "";
      str1 = "";
      for (int i = 0; i < (*it2)->szRelo; i++)
      {
        str = "";
        str1 = "";
        inF->read((char *)&num, sizeof(short));
        while (true)
        {
          inF->read(&c, sizeof(char));
          if (c == (char)0x00)
            break;
          str = str + c;
        }
        while (true)
        {
          inF->read(&c, sizeof(char));
          if (c == (char)0x00)
            break;
          str1 = str1 + c;
        }
        inF->read((char *)&num1, sizeof(short));
        (*iter)->lstRel->at(sectName)->push_back(new relEntryLinker(num, str, str1, num1));
      }
    }
  }
  catch (exception &e)
  {
    e.what();
    exit(-3);
  }
  delete listOfChars;
}

void makeHex()
{
  // mora da se ide redom od 1. navedenog fajla u komandnoj liniji
  for (auto it = fileInfos->begin(); it != fileInfos->end(); it++)
  {
    short sctAddr;
    short val;
    char c;
    bool isSect = false;
    string strname;
    for (auto it2 = (*it)->str->begin(); it2 != (*it)->str->end(); it2++)
    {
      for (symbLinker *s : *(*it)->lstSymb)
      {
        if (s->mId == (*it2)->indSect)
          strname = s->name;
      }
      if (sectionContentLinker->count(strname) == 0)
      {
        sectionContentLinker->insert(make_pair(strname, new list<char>()));
      }
      for (int i = 0; i < (*it2)->sz; i++)
      {
        c = (*it)->sectCont->at(strname)->front();
        sectionContentLinker->at(strname)->push_back(c);
        (*it)->sectCont->at(strname)->pop_front();
      }
      for (auto iter = (*it)->lstRel->at(strname)->begin(); iter != (*it)->lstRel->at(strname)->end(); iter++)
      {
        for (symbLinker *s : *symTableLinker /**(*it)->lstSymb*/)
        {
          if (s->name == (*iter)->symbol)
          {
            /*for(string str : *sections) {
              if(str == s->name)
                isSect = true;
            }*/

            auto iiter = sectionContentLinker->at(strname)->begin();
            for (int tmp = 0; tmp < (*iter)->offset; tmp++)
            {
              iiter++;
            }
            for (symbLinker *sym : *symTableLinker)
            {
              if (sym->name == strname)
                (*iter)->offset += sym->value;
            }
            for (string str : *sections)
            {
              if (str == s->name)
              {
                for (symbLinker *sym : *(*it)->lstSymb)
                {
                  if (sym->name == str)
                  {
                    (*iter)->addend += sym->value;
                    isSect = true;
                  }
                }
              }
            }
            if ((*iter)->typ == "R_16")
            {

              c = (char)(((*iter)->addend + s->value) & 0xFF);
              sectionContentLinker->at(strname)->insert(iiter, c);
              c = (char)(((*iter)->addend + s->value) >> 8);
              sectionContentLinker->at(strname)->insert(iiter, c);

              iiter = sectionContentLinker->at(strname)->erase(iiter);
              iiter = sectionContentLinker->at(strname)->erase(iiter);
            }
            else
            {

              if (isSect)
              {
                c = (char)(((*iter)->addend + s->value - (*iter)->offset) & 0xFF);
                sectionContentLinker->at(strname)->insert(iiter, c);
                c = (char)(((*iter)->addend + s->value - (*iter)->offset) >> 8);
                sectionContentLinker->at(strname)->insert(iiter, c);
              }
              else
              {
                c = (char)(((*iter)->addend + s->value - (*iter)->offset) & 0xFF);
                sectionContentLinker->at(strname)->insert(iiter, c);
                c = (char)(((*iter)->addend + s->value - (*iter)->offset) >> 8);
                sectionContentLinker->at(strname)->insert(iiter, c);
              }
              iiter = sectionContentLinker->at(strname)->erase(iiter);
              iiter = sectionContentLinker->at(strname)->erase(iiter);
            }
          }
        }
      }
    }
  }
}

void makeRelo()
{
  // mora da se ide redom od 1. navedenog fajla u komandnoj liniji
  for (auto it = fileInfos->begin(); it != fileInfos->end(); it++)
  {
    short sctAddr;
    short val;
    char c;
    bool isSect = false;
    string strname;
    for (auto it2 = (*it)->str->begin(); it2 != (*it)->str->end(); it2++)
    {
      for (symbLinker *s : *(*it)->lstSymb)
      {
        if (s->mId == (*it2)->indSect)
          strname = s->name;
      }
      if (sectionContentLinker->count(strname) == 0)
      {
        sectionContentLinker->insert(make_pair(strname, new list<char>()));
      }
      for (int i = 0; i < (*it2)->sz; i++)
      {
        c = (*it)->sectCont->at(strname)->front();
        sectionContentLinker->at(strname)->push_back(c);
        (*it)->sectCont->at(strname)->pop_front();
      }
      for (auto iter = (*it)->lstRel->at(strname)->begin(); iter != (*it)->lstRel->at(strname)->end(); iter++)
      {
        if ((*iter)->typ == "R_PC16")
        {
          for (symbLinker *s : *symTableLinker)
          {
            if ((*iter)->symbol == s->name && s->sect == strname)
            {
              auto ii = sectionContentLinker->at(strname)->begin();
              for (int i = 0; i < (*iter)->offset; i++)
              {
                ii++;
              }
              short dist = s->value - (*iter)->offset;
              sectionContentLinker->at(strname)->insert(ii, (char)(dist & 0xFF));
              sectionContentLinker->at(strname)->insert(ii, (char)(dist >> 8));
              ii = sectionContentLinker->at(strname)->erase(ii);
              ii = sectionContentLinker->at(strname)->erase(ii);
              iter = (*it)->lstRel->at(strname)->erase(iter);
              iter--;
            }
          }
        }
      }
    }
  }
}

int main(int argc, char const *argv[])
{

  string outFileName = "";
  string str;
  list<string> *entryFiles = new list<string>();
  map<string, unsigned short> *sectionAddrs = new map<string, unsigned short>();
  unsigned short i = 1;
  unsigned short ind;
  bool hasHex = false;
  bool hasRelocateable = false;
  fstream inFile;
  ofstream outFile;
  ofstream outFileTxt;
  try
  {
    if (argc == 1)
    {
      throw BadLinkerUsage();
    }

    while (i < argc)
    {
      if ((string)argv[i] == "-o")
      {
        i++;
        outFileName = (string)argv[i++];
      }
      else if (((str = (string)argv[i]).substr(0, 7)) == "-place=")
      {
        i++;
        ind = str.rfind("@");
        if (sectionAddrs->count(str.substr(7, ind)) != 0)
        {
          stringstream ss;
          string numstr = str.substr(ind + 3, str.length() - ind - 3);
          ss << hex << numstr;
          unsigned short tmp;
          ss >> tmp;
          sectionAddrs->at(str.substr(7, ind - 7)) = tmp;
        }
        else
        {
          stringstream ss;
          string numstr = str.substr(ind + 3, str.length() - ind - 3);
          ss << hex << numstr;
          unsigned short tmp;
          ss >> tmp;
          sectionAddrs->insert(make_pair(str.substr(7, ind - 7), tmp));
        }
      }
      else if ((string)argv[i] == "-hex")
      {
        i++;
        hasHex = true;
      }
      else if ((string)argv[i] == "-relocateable")
      {
        i++;
        hasRelocateable = true;
      }
      else
      {
        entryFiles->push_back((string)argv[i++]);
      }
    }

    if (outFileName == "")
    {
      if (hasHex)
        outFileName = "defaultName.hex";
      else
        outFileName = "defaultName.o";
    }

    if ((!hasHex && !hasRelocateable) || (hasHex && hasRelocateable))
    {
      cout << "Potrebno je iskoristiti tacno jednu od opcija -hex i -relocateable" << endl;
      exit(-1);
    }
  }
  catch (exception &e)
  {
  }

  try
  {
    for (auto it = entryFiles->begin(); it != entryFiles->end(); it++)
    {
      inFile.open((*it));
      if (!inFile)
      {
        throw BadInputFile((*it));
      }
      processFile(&inFile);
      inFile.close();
    }
    bool inside;

    symbLinker *tmp;
    short ind = 0;

    for (auto it = fileInfos->begin(); it != fileInfos->end(); it++)
    {
      i = 0;
      inside = false;

      for (auto it2 = (*it)->lstSymb->begin(); it2 != (*it)->lstSymb->end(); it2++)
      {
        inside = false;
        if ((*it2)->name == (*it2)->sect)
        {
          sections->push_back((*it2)->name);
          for (auto it3 = symTableLinker->begin(); it3 != symTableLinker->end(); it3++)
          {
            if ((*it3)->name == (*it2)->name)
            {
              inside = true;
              tmp = *it3;
            }
          }
          if (!inside)
          {

            symTableLinker->push_back(new symbLinker((*it2)->name, "SCTN", 0, 'l'));
            tmp = *symTableLinker->rbegin();
            tmp->size = (*it2)->size;
            tmp->sect = tmp->name;
            tmp->mId = ++ind;
            if (sectionAddrs->count(tmp->name) != 0 && hasHex)
            {
              tmp->value = sectionAddrs->at(tmp->name);
            }
          }
          else
          {
            (*it2)->value = tmp->size;
            // cout << (*it2)->name << endl;
            //(*it)->printInfo();
            for (auto it3 = (*it)->lstRel->at((*it2)->name)->begin(); it3 != (*it)->lstRel->at((*it2)->name)->end(); it3++)
            {
              (*it3)->offset += (*it2)->value;
              // cout << "Sekc: " << (*it2)->name << " : " << (*it3)->offset << endl;
            }
            tmp->size += (*it2)->size;
          }
        }
      }
    }
    sections->unique();
    if (hasHex)
    {
      if (sectionAddrs->size() == 0)
      {
        symbLinker *s = *symTableLinker->begin();
        unsigned short addr = 0;
        unsigned short lastSize = s->size;
        auto iiter = symTableLinker->begin();
        iiter++;
        for (; iiter != symTableLinker->end(); iiter++)
        {
          (*iiter)->value = lastSize;
          lastSize += (*iiter)->size;
        }
      }
      else
      {
        unsigned short addr = sectionAddrs->begin()->second;
        unsigned short lastSize = 0;
        for (symbLinker *s : *symTableLinker)
        {
          if (s->name == sectionAddrs->begin()->first)
          {
            lastSize = s->size;
          }
        }
        for (auto it = sectionAddrs->begin(); it != sectionAddrs->end(); it++)
        {
          if (it->second > addr)
          {
            addr = it->second;
            for (symbLinker *s : *symTableLinker)
            {
              if (s->name == it->first)
              {
                lastSize = s->size;
              }
            }
          }
          for (symbLinker *s : *symTableLinker)
          {
            if (s->name == it->first)
            {
              for (auto it2 = sectionAddrs->begin(); it2 != sectionAddrs->end(); it2++)
              {
                if (it2->first != it->first && (it2->second < it->second + s->size) && (it2->second > it->second))
                {
                  throw OverlapingSections(it->first, it2->first);
                }
              }
              break;
            }
          }
        }
        for (symbLinker *s : *symTableLinker)
        {
          if (s->value == 0 && sectionAddrs->count(s->name) == 0)
          {
            s->value = addr + lastSize;
            lastSize += s->size;
          }
        }
      }
    }

    for (auto it = fileInfos->begin(); it != fileInfos->end(); it++)
    {
      str = "UND";
      i = 0;
      bool inside = false;
      bool defined = false;
      for (auto it2 = (*it)->lstSymb->begin(); it2 != (*it)->lstSymb->end(); it2++)
      {
        str = "UND";
        i = 0;
        defined = false;
        inside = false;
        if ((*it2)->bind == 'g')
        {
          for (auto it3 = symTableLinker->begin(); it3 != symTableLinker->end(); it3++)
          {
            if ((*it3)->name == (*it2)->name)
            {
              inside = true;
              if ((*it3)->sect != "UND")
              {
                defined = true;
              }
            }
          }
          if (!inside)
          {
            for (symbLinker *s : *(*it)->lstSymb)
            {
              if (s->name == (*it2)->sect)
              {
                i = s->value;
                str = s->name;
              }
            }
            for (symbLinker *s : *symTableLinker)
            {
              if ((*it2)->sect == s->name)
              {
                i += s->value;
              }
            }
            symTableLinker->push_back(new symbLinker((*it2)->name, "NOTYP", i + (*it2)->value, 'g'));
            tmp = *symTableLinker->rbegin();
            tmp->sect = str;
            tmp->mId = ++ind;
          }
          else if ((*it2)->sect != "UND")
          {
            if (defined)
            {
              throw MultipleDef((*it2)->name);
            }
            for (auto it3 = symTableLinker->begin(); it3 != symTableLinker->end(); it3++)
            {
              if ((*it3)->name == (*it2)->name)
              {
                (*it3)->value += (*it2)->value;
                for (symbLinker *s : *(*it)->lstSymb)
                {
                  if (s->name == (*it2)->sect)
                  {
                    (*it3)->value += s->value;
                  }
                }
                for (symbLinker *s : *symTableLinker)
                {
                  if ((*it2)->sect == s->name)
                  {
                    (*it3)->value += s->value;
                  }
                }
                (*it3)->sect = (*it2)->sect;
              }
            }
          }
        }
      }
    }
    /*cout << "\nKrajnja tabela simbola\n";
    for(auto it = symTableLinker->begin(); it != symTableLinker->end(); it++) {
      cout << (*it)->name << "\t" << (*it)->sect << "\t" << (*it)->value << "\t" << (*it)->size << "\t" << (*it)->bind << endl;
    }*/

    for (symbLinker *s : *symTableLinker)
    {
      if (s->sect == "UND")
        throw UndefinedSymb(s->name);
    }

    if (hasHex)
    {
      makeHex();
    }
    else
    {
      makeRelo();
    }

    /*for(auto it = fileInfos->begin(); it != fileInfos->end(); it++) {
      cout << "Izlged u fileInfos\n";
      (*it)->printInfo();
      cout << endl;
    }*/

    /*cout << "Izlged posle makeHex-a\n";
    for(auto it = sectionContentLinker->begin(); it != sectionContentLinker->end(); it++) {
      short n = 0;
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
    }*/

    if (hasHex)
    {
      char c;
      unsigned short sectCnt = 0;
      unsigned short tmpAddr = 0;
      string outFileNameBin = outFileName.substr(0, outFileName.length() - 4) + "_b.bin";
      outFile.open(outFileNameBin, ios::binary);
      outFileTxt.open(outFileName, ios::out);
      if (!outFile.is_open() || !outFileTxt.is_open())
      {
        throw FileWontOpen();
      }
      c = 0xFF;
      outFile.write(reinterpret_cast<const char *>(&c), sizeof(char));

      for (symbLinker *s : *symTableLinker)
      {
        if (s->name == s->sect && s->size > 0)
        {
          sectCnt++;
        }
      }
      c = (char)(sectCnt & 0xFF);
      outFile.write(reinterpret_cast<const char *>(&c), sizeof(char));
      int tmp = 0;
      for (int i = 0; i < sectCnt; i++)
      {
        outFile.write(reinterpret_cast<const char *>(&tmp), sizeof(int));
      }
      tmpAddr = sectCnt * 4 + 2;
      // cout << "ovde\n";
      for (auto it = sectionContentLinker->begin(); it != sectionContentLinker->end(); it++)
      {
        for (symbLinker *s : *symTableLinker)
        {
          if (s->name == it->first)
          {
            outFile.seekp(2 + tmp * 4, ios::beg);
            outFile.write(reinterpret_cast<const char *>(&s->value), sizeof(short));
            outFile.write(reinterpret_cast<const char *>(&s->size), sizeof(short));
            tmp++;
          }
        }
        outFile.seekp(tmpAddr);
        for (auto it2 = it->second->begin(); it2 != it->second->end(); it2++)
        {
          c = *it2;
          outFile.write(reinterpret_cast<const char *>(&c), sizeof(char));
        }
        tmpAddr += it->second->size();
      }
      unsigned short min = (unsigned short)0xFFFF;
      string nm = "";
      map<string, int> *addrs = new map<string, int>();
      list<string> *ordered = new list<string>();
      for (int i = 0; i < sectCnt; i++)
      {
        for (symbLinker *s : *symTableLinker)
        {
          if (s->name == s->sect && s->value < min && s->size > 0 && addrs->count(s->name) == 0)
          {
            min = s->value;
            nm = s->name;
          }
        }
        addrs->insert(make_pair(nm, min));
        ordered->push_back(nm);
        min = (unsigned short)0xFFFF;
        nm = "";
      }

      short addr = 0;
      tmpAddr = 0;
      string result;
      stringstream sstream;
      for (auto iter = ordered->begin(); iter != ordered->end(); iter++)
      {
        for (auto it = sectionContentLinker->begin(); it != sectionContentLinker->end(); it++)
        {
          if (it->first != *iter)
            continue;
          for (symbLinker *s : *symTableLinker)
          {
            if (s->name == it->first)
            {
              addr = s->value;
            }
          }
          for (auto it2 = it->second->begin(); it2 != it->second->end(); it2++)
          {
            if (tmpAddr % 8 == 0)
            {
              if (it2 != it->second->begin())
                outFileTxt << endl;
              sstream << std::hex << setw(4) << setfill('0') << addr;
              result = sstream.str();
              outFileTxt << result.substr(result.length() - 4, result.length());
              outFileTxt << ": ";
              tmpAddr = 0;
            }
            addr++;
            tmpAddr++;
            c = *it2;
            sstream << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)(*it2 - 0);
            result = sstream.str();
            outFileTxt << result.substr(result.length() - 2, result.length()) << " ";
          }
          if (tmpAddr != 0)
          {
            tmpAddr = 0;
            outFileTxt << endl;
          }
        }
      }

      delete addrs;
      delete ordered;
      outFile.close();
      outFileTxt.close();
    }
    else
    {
      // relo
      /**
       * @brief Dosao sam do pravljenja -relocateable opcije, da li treba ici onu for petlju kroz sve fileInfos?
       * Da li mozda da ako ima vise sekcija da sve njihove relokacione tabele spojim u jednu pa onda dole vrsim upisivanje, ili da ostavim kako je sad
       * pa da nastavim i dole prodjem kroz sve tabele? O tome razmisliti
       * POGLEDAJ NAPOMENU.TXT koje greske jos imam
       *
       */
      // cout << "_______________RELOKATEABLE_______________________\n";
      ofstream outfile_b;
      ofstream outfile;
      string s = outFileName.substr(0, outFileName.length() - 2);
      string sb = s + "_b.bin";
      int tmp = 0;
      char c = 0xF0;
      ;
      char tmpNum = 0;
      for (symbLinker *s : *symTableLinker)
      {
        tmpNum++;
      }
      // cout << sb;
      unsigned short pos = 0;
      outfile_b.open(sb, ios::binary); // ovo treba doraditi lepo
      // neka info struktura na pocetku
      // itd.
      outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char)); //+1 1.
      c = 0x00;
      outfile_b.write(reinterpret_cast<const char *>(&pos), sizeof(short));   // +3 2. 3.
      outfile_b.write(reinterpret_cast<const char *>(&tmpNum), sizeof(char)); // +4  4. broj simbola u tabeli simbola
      tmpNum = (char)sectionContentLinker->size();
      outfile_b.write(reinterpret_cast<const char *>(&tmpNum), sizeof(char)); // +5 5.
      c = 0x00;
      outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char)); // +7 6.
      for (int i = 0; i < tmpNum * 8 + 2; i++)
      {
        outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char)); // 7. 8. i ostalo
      }
      pos = tmpNum * 8 + 8;

      sections = new list<string>();
      outfile.open(outFileName);
      // cout << "------------------------------------------" << endl;
      for (auto it = sectionContentLinker->begin(); it != sectionContentLinker->end(); ++it)
      {
        // cout << it->first << endl;
        outfile << it->first << endl;
        sections->push_back(it->first);
        int n = 0;
        outfile_b.seekp((tmp + 1) * 8, ios::beg);
        for (symbLinker *s : *symTableLinker)
        {
          if (it->first == s->name)
          {
            c = (char)(s->mId & 0xFF);
            break;
          }
        }
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
          outfile << *it2 << " ";
          outfile_b << *it2;
          n++;
          if (n == 4)
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
      string strname = "";
      // cout << "Rel Tables: " << endl;
      outfile << "Rel Tables: " << endl;
      short entNum = 0;
      short tmpPos = 0;
      for (auto n = sections->begin(); n != sections->end(); n++)
      {
        // cout << *n << endl;
        outfile << *n << endl;
        for (auto ii = fileInfos->begin(); ii != fileInfos->end(); ii++)
        {
          for (auto i2 = (*ii)->lstRel->begin(); i2 != (*ii)->lstRel->end(); i2++)
          {
            if (i2->first == *n && i2->second->size() > 0)
            {
              for (relEntryLinker *ss : *i2->second)
              {
                // cout << ss->offset << "\t" << ss->typ << "\t" << ss->symbol << "\t" << ss->addend << endl;
                outfile << ss->offset << "\t" << ss->typ << "\t" << ss->symbol << "\t" << ss->addend << endl;
                // outfile_b << ent->offset << ent->typ << ent->symbol->name << ent->addend;
                outfile_b.write(reinterpret_cast<const char *>(&ss->offset), sizeof(ss->offset));
                outfile_b << ss->typ;
                outfile_b << (char)0;
                outfile_b << ss->symbol;
                outfile_b << (char)0;
                outfile_b.write(reinterpret_cast<const char *>(&ss->addend), sizeof(ss->addend));
                tmpPos += 6 + ss->typ.length() + ss->symbol.length();
                entNum++;
              }
            }
          }
        }
        if (entNum > 0)
        {
          outfile_b.seekp((tmp + 1) * 8 + 5, ios::beg);
          c = (char)(pos & 0xFF);
          outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
          c = (char)(pos >> 8);
          outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
          c = (char)((unsigned short)entNum & 0xFF);
          outfile_b.write(reinterpret_cast<const char *>(&c), sizeof(char));
          outfile_b.seekp(0, ios::end);
        }
        tmp++;
        entNum = 0;
        pos += tmpPos;
        tmpPos = 0;
      }
      /*for(auto it = relaTables->begin(); it != relaTables->end(); ++it) {
        cout << it->first << endl;
        outfile << it->first << endl;
        //it->second->printRelaTable(&outfile);
        if(it->second->getEntries()->size() != 0) {
          outfile_b.seekp((tmp+1)*8+5, ios::beg);
          c = (char)(pos & 0xFF);
          outfile_b.write(reinterpret_cast<const char*>(&c), sizeof(char));
          c = (char)(pos >> 8);
          outfile_b.write(reinterpret_cast<const char*>(&c), sizeof(char));
          c = (char)((unsigned short)it->second->getEntries()->size() & 0xFF);
          outfile_b.write(reinterpret_cast<const char*>(&c), sizeof(char));
          outfile_b.seekp(0, ios::end);
        }
        tmp++;
        cout << "Offset\tType\tSymbol\tAddend\n";
        outfile << "Offset\tType\tSymbol\tAddend\n";
        for(relEntry* ent : *(it->second->getEntries())) {
          cout << ent->offset << "\t" << ent->typ << "\t" << ent->symbol->name << "\t" << ent->addend << endl;
          outfile << ent->offset << "\t" << ent->typ << "\t" << ent->symbol->name << "\t" << ent->addend << endl;
          //outfile_b << ent->offset << ent->typ << ent->symbol->name << ent->addend;
          outfile_b.write(reinterpret_cast<const char *>(&ent->offset), sizeof(ent->offset));
          outfile_b << ent->typ;
          outfile_b << (char)0;
          outfile_b << ent->symbol->name;
          outfile_b << (char)0;
          outfile_b.write(reinterpret_cast<const char *>(&ent->addend), sizeof(ent->addend));
          pos += 6 + ent->typ.length() + ent->symbol->name.length();
        }
        cout << endl;
        outfile << endl;
      }*/
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
      for (symbLinker *ent : *symTableLinker)
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
    }
  }
  catch (exception &e)
  {
    // cout << "shit happend";
    e.what();
    exit(-2);
  }

  delete sections;
  delete sectionAddrs;
  // cout << "Uspeh\n";
  return 0;
}
