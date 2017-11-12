#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <tuple>

#include <clang-c/Index.h>

using namespace std;;

struct EnumInfo {
  string name;
  vector<string> enumconstants;
};

struct FuncDecl {
  string name;
  string resulttype;
  vector<string> paramtypes;
};

struct StructDecl {
  string name;
  vector<tuple<string, string>> fieldnames;
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

string typeToString(CXType ty) {
  switch(ty.kind) {
  case CXType_Void: return "T.void";
  case CXType_UInt: return "T.size_t";
  case CXType_Int: return "T.int";
  case CXType_Bool: return "T.bool";
  case CXType_Char_S: return "T.string";
  case CXType_Char_U: return "T.string";
  //case CXType_Unexposed: return "UNEXPOSED";  <--- This is returned for a function pointer in a struct
  // Solution here maybe? http://clang-developers.42468.n3.nabble.com/Function-pointer-in-a-struct-declaration-td4044706.html 

  case CXType_Pointer: {
    CXType ptee = clang_getPointeeType(ty);
    return "T.ptr " + typeToString(ptee);
  }

  default: return getFromCXString(clang_getTypeSpelling(ty)); 
  }
}


CXChildVisitResult gatherStructDecls(CXCursor c, CXCursor parent, CXClientData client_data) {
  switch(clang_getCursorKind(c)) {
  case CXCursor_StructDecl: {
    vector<StructDecl>* v = (vector<StructDecl>*) client_data;

    StructDecl s;
    s.name = getFromCXString(clang_getCursorSpelling(c));

    clang_visitChildren(c,
			[](CXCursor c, CXCursor p, CXClientData cd) {
			  switch(clang_getCursorKind(c)) {
			  case CXCursor_FieldDecl: {
			    CXType f_type = clang_getCursorType(c);
			    vector<tuple<string, string>>* fieldnames = (vector<tuple<string, string>>*) cd;
			    string fieldname = getFromCXString(clang_getCursorSpelling(c));
			    fieldnames->push_back(make_tuple(fieldname, typeToString(f_type)));
			    break;
			  }
			  default: break;
			  }
			  return CXChildVisit_Recurse;
			}, (CXClientData) &s.fieldnames);
    v->push_back(s);
    break;
  }

  default: break;
    
  }

  return CXChildVisit_Recurse;
}

void printStructDecl(StructDecl s) {
  cout << "type " << s.name << endl;
  cout << "let " << s.name << " : " << s.name << " Ctypes.structure T.typ = T.structure \"" << s.name << "\"" << endl;
  for( auto f : s.fieldnames ) {
    cout << "let " << s.name + "_" + get<0>(f) << " = T.field " << s.name << " \"" << get<0>(f) << "\" (" << get<1>(f) << ")" << endl;
  }
}


CXChildVisitResult gatherFuncDecls(CXCursor c, CXCursor parent, CXClientData client_data) {
  switch(clang_getCursorKind(c)) {
  case CXCursor_FunctionDecl: {
    vector<FuncDecl>* v = (vector<FuncDecl>*) client_data;

    FuncDecl f;
    CXType fty = clang_getCursorType(c);
    CXType retty = clang_getResultType(fty);

    f.name= getFromCXString(clang_getCursorSpelling(c));
    f.resulttype = typeToString(retty);

    clang_visitChildren(c,
			[](CXCursor c, CXCursor p, CXClientData cd) {
			  switch(clang_getCursorKind(c)) {
			  case CXCursor_ParmDecl: {
			    CXType c_type = clang_getCursorType(c);
			    vector<string>* paramtypes = (vector<string>*) cd;
			    string s = typeToString(c_type);
			    paramtypes->push_back(s);
			    break;
			  }
			  default: break;
			  }
			  return CXChildVisit_Recurse;
			}, (CXClientData) &f.paramtypes);
    v->push_back(f);
    break;
  }
  default: break;
    
  }

  return CXChildVisit_Recurse;
}

void printFuncDecl(FuncDecl f) {
  cout << "let " << f.name << " = F.foreign \"" << f.name << "\" (";
  for(auto e : f.paramtypes) {
    cout << e << " @-> ";
  }
  cout << "returning (" << f.resulttype << "))" << endl;
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
  string enum_lowername = lowerCase(e.name);
  cout << "type " << enum_lowername << " =" << endl;
  for(auto s : e.enumconstants) {
    cout << "\t | " << s << endl;
  }
  cout << endl;

  for(auto s : e.enumconstants) {
    cout << "let " << lowerCase(s) << " = T.constant \"" << s << "\" T.int64_t" << endl;
  }
  cout << endl;

  cout << "let " << enum_lowername << " = T.Enum " << e.name << " [" << endl;

  for(auto s : e.enumconstants) {
    cout << "\t" << s << ", " << lowerCase(s) << ";" << endl;
  }
  cout << "]" << endl;
}

int main(int argc, char *argv[]) {
  if(argc < 2) {
    cout << "Requires a C/C++ header file to parse\n";
	    exit(1);
  }

  
  CXIndex index = clang_createIndex(0,0);
  auto unit = parseFile(index, argv[1]);

  CXCursor cur = clang_getTranslationUnitCursor(unit);

  vector<EnumInfo> enums;
  clang_visitChildren(cur, gatherEnumInfo, &enums);

  vector<FuncDecl> funcdecls;
  clang_visitChildren(cur, gatherFuncDecls, &funcdecls);

  vector<StructDecl> structdecls;
  clang_visitChildren(cur, gatherStructDecls, &structdecls);


  for(auto e : enums) {
    printEnumBindings(e);
    cout << endl;
  }

  cout << "FuncDecls Size: " << funcdecls.size() << endl;

  for (auto f : funcdecls) {
    printFuncDecl(f);
    cout << endl;
  }
  
  cout << "StructDecls Size: " << structdecls.size() << endl;

  for(auto s : structdecls) {
    printStructDecl(s);
    cout << endl;
  }

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
  
  return 0;
}
