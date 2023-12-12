#ifndef KVALUE_H
#define KVALUE_H

namespace llvm {
class Value;
class StringRef;
} // namespace llvm

namespace klee {

struct KValue {
public:
  /* Enum of kinds for llvm rtti purposes */
  enum Kind {
    BLOCK,
    CONSTANT,
    GLOBAL_VARIABLE,
    FUNCTION,
    INLINE_ASM,
    INSTRUCTION,
    VALUE,
  };

protected:
  /* Wrapped llvm::Value */
  llvm::Value *const value;

  /* Kind of KValue required for llvm rtti purposes */
  const Kind kind;

  /* Inner constructor for setting kind */
  KValue(llvm::Value *const value, const Kind kind)
      : value(value), kind(kind) {}

public:
  /* Construct KValue from raw llvm::Value. */
  KValue(llvm::Value *const value) : KValue(value, Kind::VALUE) {
    /* NB: heirs of this class should use inner constructor instead.
       This constructor exposes convenient public interface. */
  }

  /* Unwraps KValue to receive inner value. */
  [[nodiscard]] llvm::Value *unwrap() const;

  /* Returns name of inner value if so exists. */
  [[nodiscard]] virtual llvm::StringRef getName() const;

  [[nodiscard]] virtual bool operator<(const KValue &rhs) const;
  [[nodiscard]] virtual unsigned hash() const;

  /* Kind of value. Serves for llvm rtti purposes. */
  [[nodiscard]] Kind getKind() const { return kind; }
  [[nodiscard]] static bool classof(const KValue *rhs) { return true; }

  virtual ~KValue() = default;
};

} // namespace klee

#endif // KVALUE_H
