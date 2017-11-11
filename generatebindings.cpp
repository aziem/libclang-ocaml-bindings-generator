#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#include <clang-c/Index.h>

using namespace std;;

struct EnumInfo {
  string name;
  vector<string> enumconstants;
};

ostream& operator<<(ostream& stream, const CXString& str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

string getFromCXString(const CXString& str) {
  string s(clang_getCString(str));
  clang_disposeString(str);
  return s;
}

string lowerCase(string s) {
    string lowername = s;
    transform(lowername.begin(), lowername.end(), lowername.begin(), ::tolower);
    return lowername;
}

CXTranslationUnit parseFile(CXIndex index, string filename) {
  CXTranslationUnit unit = clang_parseTranslationUnit(index, filename.c_str(), nullptr, 0, nullptr, 0, CXTranslationUnit_None);
  if(unit==nullptr) {
    cerr << "Unable to parse file." << endl;
    exit(-1);
  } 
  else {
    return unit;
  }
}

CXChildVisitResult gatherEnumInfo(CXCursor c, CXCursor parent, CXClientData client_data) {
  switch(clang_getCursorKind(c)) {
  case CXCursor_EnumDecl: {

    vector<EnumInfo>* v = (vector<EnumInfo>*) client_data;
    
    EnumInfo e; 
    e.name = getFromCXString(clang_getCursorSpelling(c));

    //grrr lambda capture doesn't seem to work here - use client data instead
    clang_visitChildren(c,
    			[](CXCursor c, CXCursor p, CXClientData cd) {
    			  switch(clang_getCursorKind(c)) {
    			  case CXCursor_EnumConstantDecl: {
			    vector<string>* e = (vector<string>*) cd;
    			    string s = getFromCXString(clang_getCursorSpelling(c));
			    e->push_back(s);
    			    break;
    			  }
    			  default: break;
    			  }
    			  return CXChildVisit_Recurse;
    			},
			(CXClientData) &e.enumconstants);
    v->push_back(e);
    break;
    
  }
  
  default: break;

  }
  return CXChildVisit_Recurse;
};

void printEnumBindings(EnumInfo e) {
  cout << "type " << e.name << " =" << endl;
  for(auto s : e.enumconstants) {
    cout << "\t | " << s << endl;
  }
  cout << endl;

  for(auto s : e.enumconstants) {
    cout << "let " << lowerCase(s) << " = T.constant \"" << s << "\" T.int64_t" << endl;
  }
  cout << endl;

  string enum_lowername = lowerCase(e.name);
  cout << "let " << enum_lowername << " T.Enum " << e.name << " [" << endl;

  for(auto s : e.enumconstants) {
    cout << "\t" << s << ", " << lowerCase(s) << ";" << endl;
  }
  cout << "]" << endl;
}

int main() {
  CXIndex index = clang_createIndex(0,0);
  auto unit = parseFile(index, "binaryninjacore.h");

  CXCursor cur = clang_getTranslationUnitCursor(unit);

  vector<EnumInfo> enums;
  clang_visitChildren(cur, gatherEnumInfo, &enums);

  for(auto e : enums) {
    printEnumBindings(e);
    cout << endl;
  }

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
  
  return 0;
}
