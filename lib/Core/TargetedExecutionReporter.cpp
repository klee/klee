//===-- TargetedExecutionReporter.cpp--------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Core/TargetedExecutionReporter.h"
#include "klee/Support/ErrorHandling.h"

#include <sstream>

using namespace klee;

void klee::reportFalsePositive(confidence::ty confidence,
                               const std::vector<ReachWithError> &errors,
                               const std::string &id,
                               std::string whatToIncrease) {
  std::ostringstream out;
  std::vector<ReachWithError> errorsSet(errors.begin(), errors.end());
  out << getErrorsString(errorsSet) << " False Positive at trace " << id;
  if (!confidence::isConfident(confidence)) {
    out << ". Advice: "
        << "increase --" << whatToIncrease << " command line parameter value";
  }
  klee_warning("%s%% %s", confidence::toString(confidence).c_str(),
               out.str().c_str());
}

bool confidence::isConfident(ty conf) { return conf > Confident; }

bool confidence::isVeryConfident(ty conf) { return conf > VeryConfident; }

template <typename... Args>
std::string string_format(
    const std::string &format,
    Args... args) { // from https://stackoverflow.com/a/26221725/10547926
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
               1; // Extra space for '\0'
  if (size_s <= 0)
    return "";
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1); // We don't want the '\0' inside
}

std::string confidence::toString(ty conf) {
  return string_format("%.2f", conf);
}

confidence::ty confidence::min(confidence::ty left, confidence::ty right) {
  return fmin(left, right);
}

bool confidence::isNormal(confidence::ty conf) {
  return confidence::MinConfidence <= conf && conf <= confidence::MaxConfidence;
}

confidence::ty confidence::MinConfidence = 0.0;
confidence::ty confidence::MaxConfidence = 100.0;
confidence::ty confidence::Confident = 90.0;
confidence::ty confidence::VeryConfident = 99.0;
