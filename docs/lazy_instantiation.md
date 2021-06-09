# Ленивая инстанциация

## Формат теста

Теперь тест представляется в формате `json`. Его структура не сильно отличается от
структуры `KTest` и содержит следующие данные:

* Передаваемые программе аргументы и их количество
* SymArgvs и symArgvLen
* Объекты и их количество

Каждый объект содержит следующие данные:

* Название
* Размер в байтах
* Массив значений для каждого байта
* Количество сдвигов, которые соответствуют указателям на другик лениво
  инстанциированные объекты
* Массив с описанием каждого сдвига

Описание каждого сдвига состоит из:

* Собственно размер сдвига с начала объекта в байтах
* Объект, указатель на который должен лежать по этому сдвигу

Вся информация, кроме сдвигов, аналогична той, которая записывается в `KTest`.
Пример формата теста:
```
{
  "args": [
    "simple_tree.bc"
  ],
  "n_objects": 4,
  "numArgs": 1,
  "objects": [
    {
      "n_offsets": 1,
      "name": "root",
      "offsets": [
        {
          "index": 1,
          "offset": 0
        }
      ],
      "size": 8,
      "values": [0,0,0,0,0,0,0,0]
    },
    {
      "n_offsets": 2,
      "name": "unnamed",
      "offsets": [
        {
          "index": 2,
          "offset": 8
        },
        {
          "index": 3,
          "offset": 16
        }
      ],
      "size": 24,
      "values": [255,255,255,255,255,255,255,255,0,0,0,0,255,0,0,0,0,0,0,0,0,0,0,255]
    },
    {
      "n_offsets": 0,
      "name": "unnamed",
      "size": 24,
      "values": [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    },
    {
      "n_offsets": 0,
      "name": "unnamed",
      "size": 24,
      "values": [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    }
  ],
  "symArgvLen": 0,
  "symArgvs": 0
}
```

Данный тест описывает результат выполнения `KLEE` на следующем тесте:
```
#include "klee/klee.h"
#include <iostream>

struct Node {
    int value;
    int height;
    Node* left;
    Node* right;

    Node(int v = 0, int h = 0, Node *l = nullptr, Node *r = nullptr) : value(v), left(l), right(r), height(h) {}
};

int max(int a, int b) {
  return (a > b ? a : b);
}

int check_height(Node* root) {
  if(!root->left && !root->right) {
    return 1;
  }
  return 1 + max(check_height(root->left),check_height(root->right));
}

bool check(Node* root) {
  std::cout << root->value << "\n";
  if(!root->left && !root->right) {
    return true;
  }
  if(root->left && root->right &&
     check_height(root->left) == check_height(root->right) &&
     check(root->left) && check(root->right)) {
    return true;
  }
  return false;
}

int main() {
  Node* root;
  klee_make_symbolic(&root, sizeof(root), "root");
  if(check(root) == true) {
    return 0;
  } else {
    return 1;
  }
}
```

Таким образом, `KLEE` породил лениво инстанциированное сбалансированное двоичное
дерево из корня и двух его потомков.

Для следующей программы:
```
#include "klee/klee.h"
#include <iostream>

struct Node {
  int value;
  Node* nodes[4];
  int color;
};

bool check_cycle(Node* now) {
  std::cout << "Got to node " << now << "\n";
  std::cout << "color: " << now->color << " value: " << now->value << "\n";
  now->color = 1;
  for(int i = 0; i<4; i++) {
    if(!now->nodes[i]) {
      continue;
    }
    if(now->nodes[i]->color == 1) {
      return true;
    } else {
      bool res = check_cycle(now->nodes[i]);
      if(res) return true;
    }
  }
  return false;
}

int main() {
  Node* node;
  klee_make_symbolic(&node, sizeof(node), "node");
  std::cout << sizeof(Node);
  if(check_cycle(node)) {
    return 0;
  } else {
    return 1;
  }
}
```

`KLEE` пока что порождает однотипные тесты вроде следующего:
```
{
  "args": [
    "simple_graph.bc"
  ],
  "n_objects": 6,
  "numArgs": 1,
  "objects": [
    {
      "n_offsets": 1,
      "name": "node",
      "offsets": [
        {
          "index": 1,
          "offset": 0
        }
      ],
      "size": 8,
      "values": [0,0,0,0,0,0,0,0]
    },
    {
      "n_offsets": 1,
      "name": "unnamed",
      "offsets": [
        {
          "index": 2,
          "offset": 8
        }
      ],
      "size": 48,
      "values": [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    },
    {
      "n_offsets": 1,
      "name": "unnamed",
      "offsets": [
        {
          "index": 3,
          "offset": 8
        }
      ],
      "size": 48,
      "values": [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    },
    {
      "n_offsets": 1,
      "name": "unnamed",
      "offsets": [
        {
          "index": 4,
          "offset": 8
        }
      ],
      "size": 48,
      "values": [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    },
    {
      "n_offsets": 1,
      "name": "unnamed",
      "offsets": [
        {
          "index": 5,
          "offset": 8
        }
      ],
      "size": 48,
	  "values": [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    },
    {
      "n_offsets": 0,
      "name": "unnamed",
      "size": 48,
      "values": [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0]
    }
  ],
  "symArgvLen": 0,
  "symArgvs": 0
}
```

Где постоянно инстаниируется только `node[0]`, в последней вершине в такое цепочке в `color` записывается `1`.

В нашем проекте для `json` сейчас используется [библиотека `nlohmann`](https://github.com/nlohmann/json).
Имплементации перевода из файла с тестом и обратно (класс для тестов нового формата в проекте назван `TestCase`,
`KTest` оставлен без изменений во избежание внезапной поломки `KLEE`):

Из файла в `TestCase`:
```
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
```

Из `TestCase` в файл:
```
void KleeHandler::writeTestCasePlain(const TestCase &tc, unsigned id) {
  json out;
  
  out["n_objects"] = tc.n_objects;
  out["numArgs"] = tc.numArgs;
  for(size_t i = 0; i<tc.numArgs; i++) {
    out["args"].push_back(std::string(tc.args[i]));
  }
  out["symArgvs"] = tc.symArgvs;
  out["symArgvLen"] = tc.symArgvLen;
  
  for(unsigned i = 0; i < tc.n_objects; i++) {
    json out_obj;
    out_obj["name"] = std::string(tc.objects[i].name);
    out_obj["values"] = std::vector<unsigned char>(tc.objects[i].values,
			tc.objects[i].values + tc.objects[i].size);
    out_obj["size"] = tc.objects[i].size;
    out_obj["n_offsets"] = tc.objects[i].n_offsets;
    for(unsigned j = 0; j<tc.objects[i].n_offsets; j++) {
      json offset_obj;
      offset_obj["offset"] = tc.objects[i].offsets[j].offset;
      offset_obj["index"] = tc.objects[i].offsets[j].index;
      out_obj["offsets"].push_back(offset_obj);
    }
    out["objects"].push_back(out_obj);
  }
  auto file = openTestFile("ktestjson", id);
  *file << out.dump();
  ++m_numGeneratedTests;
}
```

