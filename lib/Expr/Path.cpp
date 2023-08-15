#include "klee/Expr/Path.h"

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Casting.h"
DISABLE_WARNING_POP

using namespace klee;
using namespace llvm;

void Path::advance(KInstruction *ki) {
  if (KBlocks.empty()) {
    firstInstruction = ki->getIndex();
    lastInstruction = ki->getIndex();
    KBlocks.push_back(ki->parent);
    return;
  }
  auto lastBlock = KBlocks.back();
  if (ki->parent != lastBlock) {
    KBlocks.push_back(ki->parent);
  }
  lastInstruction = ki->getIndex();
}

unsigned Path::KBlockSize() const { return KBlocks.size(); }

const Path::path_ty &Path::getBlocks() const { return KBlocks; }

unsigned Path::getFirstIndex() const { return firstInstruction; }

unsigned Path::getLastIndex() const { return lastInstruction; }

Path::PathIndex Path::getCurrentIndex() const {
  return {KBlocks.size() - 1, lastInstruction};
}

std::vector<stackframe_ty> Path::getStack(bool reversed) const {
  std::vector<stackframe_ty> stack;
  for (unsigned i = 0; i < KBlocks.size(); i++) {
    auto current = reversed ? KBlocks[KBlocks.size() - 1 - i] : KBlocks[i];
    // Previous for reversed is the next
    KBlock *prev = nullptr;
    if (i != 0) {
      prev = reversed ? KBlocks[KBlocks.size() - i] : KBlocks[i - 1];
    }
    if (i == 0) {
      stack.push_back({nullptr, current->parent});
      continue;
    }
    if (reversed) {
      auto kind = getTransitionKind(current, prev);
      if (kind == TransitionKind::StepInto) {
        if (!stack.empty()) {
          stack.pop_back();
        }
      } else if (kind == TransitionKind::StepOut) {
        assert(isa<KCallBlock>(prev));
        stack.push_back({prev->getFirstInstruction(), current->parent});
      }
    } else {
      auto kind = getTransitionKind(prev, current);
      if (kind == TransitionKind::StepInto) {
        stack.push_back({prev->getFirstInstruction(), current->parent});
      } else if (kind == TransitionKind::StepOut) {
        if (!stack.empty()) {
          stack.pop_back();
        }
      }
    }
  }
  return stack;
}

std::vector<std::pair<KFunction *, Path::BlockRange>>
Path::asFunctionRanges() const {
  assert(!KBlocks.empty());
  std::vector<std::pair<KFunction *, BlockRange>> ranges;
  BlockRange range{0, 0};
  KFunction *function = KBlocks[0]->parent;
  for (unsigned i = 1; i < KBlocks.size(); i++) {
    if (getTransitionKind(KBlocks[i - 1], KBlocks[i]) == TransitionKind::None) {
      if (i == KBlocks.size() - 1) {
        range.last = i;
        ranges.push_back({function, range});
        return ranges;
      } else {
        continue;
      }
    }
    range.last = i - 1;
    ranges.push_back({function, range});
    range.first = i;
    function = KBlocks[i]->parent;
  }
  llvm_unreachable("asFunctionRanges reached the end of the for!");
}

Path Path::concat(const Path &l, const Path &r) {
  Path path = l;
  for (auto block : r.KBlocks) {
    path.KBlocks.push_back(block);
  }
  path.lastInstruction = r.lastInstruction;
  return path;
}

std::string Path::toString() const {
  std::string blocks = "";
  unsigned depth = 0;
  for (unsigned i = 0; i < KBlocks.size(); i++) {
    auto current = KBlocks[i];
    KBlock *prev = nullptr;
    if (i != 0) {
      prev = KBlocks[i - 1];
    }
    auto kind =
        i == 0 ? TransitionKind::StepInto : getTransitionKind(prev, current);
    if (kind == TransitionKind::StepInto) {
      blocks += " (" + current->parent->getName().str() + ":";
      depth++;
    } else if (kind == TransitionKind::StepOut) {
      blocks += ")";
      if (depth > 0) {
        depth--;
      }
      if (depth == 0) {
        blocks = "(" + current->parent->getName().str() + ":" + blocks;
        ++depth;
      }
    }
    blocks += " " + current->getLabel();
    if (i == KBlocks.size() - 1) {
      blocks += ")";
      if (depth > 0) {
        depth--;
      }
    }
  }
  blocks += std::string(depth, ')');
  return "(path: " + llvm::utostr(firstInstruction) + blocks + " " +
         utostr(lastInstruction) + ")";
}

Path Path::parse(const std::string &str, const KModule &km) {
  unsigned index = 0;
  assert(str.substr(index, 7) == "(path: ");
  index += 7;

  std::string firstInstructionStr;
  while (index < str.size() && str[index] != ' ') {
    firstInstructionStr += str[index];
    index++;
  }
  auto firstInstruction = std::stoul(firstInstructionStr);

  std::stack<KFunction *> stack;
  path_ty KBlocks;
  bool firstParsed = false;
  while (!stack.empty() || !firstParsed) {
    while (index < str.size() && str[index] == ' ') {
      index++;
    }
    assert(index < str.size());
    if (str[index] == '(') {
      index++;
      std::string functionName;
      while (str[index] != ':') {
        functionName += str[index];
        ++index;
      }
      assert(km.functionNameMap.count(functionName));
      stack.push(km.functionNameMap.at(functionName));
      firstParsed = true;
      ++index;
    } else if (str[index] == ')') {
      index++;
      stack.pop();
    } else if (str[index] == '%') {
      std::string label = "%";
      ++index;
      while (str[index] != ' ' && str[index] != ')') {
        label += str[index];
        ++index;
      }
      KBlocks.push_back(stack.top()->getLabelMap().at(label));
    }
  }
  assert(index < str.size());
  assert(str[index] == ' ');
  index++;

  std::string lastInstructionStr;
  while (index < str.size() && str[index] != ' ') {
    lastInstructionStr += str[index];
    index++;
  }
  auto lastInstruction = std::stoul(lastInstructionStr);
  assert(index < str.size() && str[index] == ')');
  return Path(firstInstruction, KBlocks, lastInstruction);
}

Path::TransitionKind Path::getTransitionKind(KBlock *a, KBlock *b) {
  if (auto cb = dyn_cast<KCallBlock>(a)) {
    if (cb->calledFunctions.count(b->parent->function) &&
        b == b->parent->entryKBlock) {
      return TransitionKind::StepInto;
    }
  }
  if (auto rb = dyn_cast<KReturnBlock>(a)) {
    return TransitionKind::StepOut;
  }
  assert(a->parent == b->parent);
  return TransitionKind::None;
}
