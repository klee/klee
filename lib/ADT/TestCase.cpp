#include "klee/ADT/TestCase.h"

#include <stdio.h>

#include <klee/Misc/json.hpp>
using json = nlohmann::json;


TestCase* TC_fromFile(const char* path) {
  FILE *f = fopen(path, "rb");
  json js = json::parse(f);
  TestCase *ret = new TestCase;
  ret->n_objects = js.at("n_objects");
  ret->numArgs = js.at("numArgs");
  ret->args = new char*[ret->numArgs];
  for(size_t i = 0; i<ret->numArgs; i++) {
    ret->args[i] = new char[js.at("args").at(i).get<std::string>().size()+1];
    strcpy(ret->args[i], js.at("args").at(i).get<std::string>().c_str());
  }
  ret->symArgvs = js.at("symArgvs");
  ret->symArgvLen = js.at("symArgvLen");
  ret->objects = new ConcretizedObject[ret->n_objects];
  for(size_t i = 0; i<ret->n_objects; i++) {
    ret->objects[i].name = new char[js.at("objects").at(i).at("name").get<std::string>().size()+1];
    strcpy(ret->objects[i].name, js.at("objects").at(i).at("name").get<std::string>().c_str());
    
    ret->objects[i].size = js.at("objects").at(i).at("size").get<unsigned>();
    
    ret->objects[i].values = new unsigned char[ret->objects[i].size];
    
    std::copy(js.at("objects").at(i).at("values").begin(), js.at("objects").at(i).at("values").end(),
	      ret->objects[i].values);
    
    ret->objects[i].n_offsets = js.at("objects").at(i).at("n_offsets");
    
    ret->objects[i].offsets = new Offset[ret->objects[i].n_offsets];
    
    for(size_t j = 0; j<ret->objects[i].n_offsets; j++) {
      ret->objects[i].offsets[j].index = js.at("objects").at(i).at("offsets").at(j).at("index");
      ret->objects[i].offsets[j].offset =
        js.at("objects").at(i).at("offsets").at(j).at("offset");
    }
  }
  return ret;
}

void ConcretizedObject_free(ConcretizedObject* obj) {
  if(obj->offsets != nullptr) {
    delete [] obj->offsets;
  }
  delete [] obj->values;
  delete [] obj->name;
  return;
}

void TestCase_free(TestCase* tc) {
  for(size_t i = 0; i < tc->n_objects; i++) {
    ConcretizedObject_free(&tc->objects[i]);
  }
  delete [] tc->objects;
  for(size_t i = 0; i < tc->numArgs; i++) {
    delete [] tc->args[i];
  }
  delete [] tc->args;
  return;
}
