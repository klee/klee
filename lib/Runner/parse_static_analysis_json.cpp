#include "klee/Runner/run_klee.h"

/* -*- mode: c++; c-basic-offset: 2; -*- */

//===-- main.cpp ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/SarifReport.h"
#include "klee/Support/ErrorHandling.h"

#include <fstream>
#include <klee/Misc/json.hpp>

using json = nlohmann::json;

using namespace llvm;
using namespace klee;

SarifReport parseInputPathTree(const std::string &inputPathTreePath) {
  std::ifstream file(inputPathTreePath);
  if (file.fail())
    klee_error("Cannot read file %s", inputPathTreePath.c_str());
  json reportJson = json::parse(file);
  SarifReportJson sarifJson = reportJson.get<SarifReportJson>();
  klee_warning("we are parsing file %s", inputPathTreePath.c_str());
  return klee::convertAndFilterSarifJson(sarifJson);
}
