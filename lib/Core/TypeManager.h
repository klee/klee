#ifndef KLEE_TYPEMANAGER_H
#define KLEE_TYPEMANAGER_H

#include <memory>
#include <unordered_map>
#include <vector>

namespace llvm {
class Type;
class Function;
} // namespace llvm

namespace klee {

class Expr;
class KType;
class KModule;
struct KInstruction;

template <class> class ref;
template <class> class ExprHashMap;

/**
 * Default class for managing type system.
 * Works with *raw* llvm types. By extending this
 * class you can add more rules to type system.
 */
class TypeManager {
private:
  void initTypesFromGlobals();
  void initTypesFromStructs();
  void initTypesFromInstructions();
  void initTypeInfo();

protected:
  KModule *parent;
  std::vector<std::unique_ptr<KType>> types;
  std::unordered_map<llvm::Type *, KType *> typesMap;

  /**
   * Make specified post initialization in initModule(). Note, that
   * it is intentionally separated from initModule, as initModule
   * order of function calls in it important. By default do nothing.
   */
  virtual void onFinishInitModule();

public:
  TypeManager(KModule *);

  /**
   * Initializes type system for current module.
   */
  void initModule();

  virtual KType *getWrappedType(llvm::Type *);
  KType *getUnknownType();

  virtual KType *handleAlloc(ref<Expr> size);
  virtual KType *handleRealloc(KType *, ref<Expr>);

  virtual ~TypeManager() = default;
};

} /*namespace klee*/

#endif /* KLEE_TYPEMANAGER_H */
