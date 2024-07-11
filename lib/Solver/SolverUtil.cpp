#include "klee/Solver/SolverUtil.h"
#include "klee/Support/CompilerWarning.h"

namespace klee {

const char *pv_to_str(PartialValidity pv) {
  switch (pv) {
  case PValidity::MustBeTrue:
    return "MustBeTrue";
  case PValidity::MustBeFalse:
    return "MustBeFalse";
  case PValidity::MayBeTrue:
    return "MayBeTrue";
  case PValidity::MayBeFalse:
    return "MayBeFalse";
  case PValidity::TrueOrFalse:
    return "TrueOrFalse";
  case PValidity::None:
    return "None";
  default:
    unreachable();
  }
}

Validity fromPartial(PartialValidity pv) {
  switch (pv) {
  case PValidity::MustBeTrue:
    return Validity::True;
  case PValidity::MustBeFalse:
    return Validity::False;
  case PValidity::TrueOrFalse:
    return Validity::Unknown;
  default:
    assert(0 && "PV not convertible to Validity");
    unreachable();
  }
}

PartialValidity toPartial(Validity v) {
  switch (v) {
  case Validity::True:
    return PValidity::MustBeTrue;
  case Validity::False:
    return PValidity::MustBeFalse;
  case Validity::Unknown:
    return PValidity::TrueOrFalse;
  default:
    unreachable();
  }
}

PartialValidity negatePartialValidity(PartialValidity pv) {
  switch (pv) {
  case PValidity::MustBeTrue:
    return PValidity::MustBeFalse;
  case PValidity::MustBeFalse:
    return PValidity::MustBeTrue;
  case PValidity::MayBeTrue:
    return PValidity::MayBeFalse;
  case PValidity::MayBeFalse:
    return PValidity::MayBeTrue;
  case PValidity::TrueOrFalse:
    return PValidity::TrueOrFalse;
  default:
    assert(0 && "invalid partial validity");
    unreachable();
  }
}

}; // namespace klee
