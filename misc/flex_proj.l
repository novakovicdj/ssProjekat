%option noyywrap
%{

enum tok {
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


struct inf{
  char* val;
  char* typ;
};

extern struct inf i1;
extern int linebr;
extern bool newLine;
%}

%%
\.[a-zA-Z]+ { i1.val = (char*)YYText(); i1.typ="DIR";  return DIR;}

0x[a-fA-F0-9]{1,4} { i1.val = (char*)YYText(); i1.typ="HEX"; return HEX;}

[a-zA-Z_0-9]+: { i1.val = (char*)YYText(); i1.typ="LAB";  return LAB;}

[0-9]+ { i1.val = (char*)YYText(); i1.typ="NUM";  return NUM;}

r[0-7] { i1.val = (char*)YYText(); i1.typ="REG";  return REG;}

[a-zA-Z_0-9]+ { i1.val = (char*)YYText(); i1.typ="WRD"; return WRD;}

%|\$|\*|\[|\] { i1.val = (char*)YYText(); i1.typ="SPC"; return SPC;}

\+|- { i1.val = (char*)YYText(); i1.typ="SGN"; return SGN;}

\".*\"|\'.*\' { i1.val = (char*)YYText(); i1.typ="STR"; return STR; }

#.{1,} {  }

\n     { newLine = true; linebr = linebr+ 1; return NWL;}

[ \t,]  {  }

.      { printf("Mystery character %s\n", YYText()); }


