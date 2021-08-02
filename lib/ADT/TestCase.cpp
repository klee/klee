#include "klee/ADT/TestCase.h"

#include <stdio.h>

#include <klee/Misc/json.hpp>
using json = nlohmann::json;


TestCase* TC_fromFile(const char* path) {
  FILE *f = fopen(path, "rb");
  json js = json::parse(f);
  TestCase *ret = new TestCase;
  ret->n_objects = js["n_objects"];
  ret->numArgs = js["numArgs"];
  ret->args = new char*[ret->numArgs];
  for(size_t i = 0; i<ret->numArgs; i++) {
    ret->args[i] = const_cast<char*>(js["args"][i].get<std::string>().c_str());
  }
  ret->symArgvs = js["symArgvs"];
  ret->symArgvLen = js["symArgvLen"];
  ret->objects = new ConcretizedObject[ret->n_objects];
  for(size_t i = 0; i<ret->n_objects; i++) {
    ret->objects[i].name =
      const_cast<char*>(js["objects"][i]["name"].get<std::string>().c_str());
    
    ret->objects[i].size = js["objects"][i]["size"].get<unsigned>();
    
    ret->objects[i].values = new unsigned char[ret->objects[i].size];
    
    std::copy(js["objects"][i]["values"].begin(), js["objects"][i]["values"].end(),
	      ret->objects[i].values);
    
    ret->objects[i].n_offsets = js["objects"][i]["n_offsets"];
    
    ret->objects[i].offsets = new Offset[ret->objects[i].n_offsets];
    
    for(size_t j = 0; j<ret->objects[i].n_offsets; j++) {
      ret->objects[i].offsets[j].index = js["objects"][i]["offsets"][j]["index"];
      ret->objects[i].offsets[j].offset =
	js["objects"][i]["offsets"][j]["offset"];
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
  return;
}
