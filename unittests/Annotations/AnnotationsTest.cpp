//===-- AnnotationTest.cpp ------------------------------------------------===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "klee/Module/Annotation.h"

#include <vector>

using namespace klee;

TEST(AnnotationsTest, Empty) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [[]],
        "properties" : []
    }
}
)");
  const AnnotationsMap expected = {
      {"foo",
       {/*functionName*/ "foo",
        /*returnStatements*/ std::vector<Statement::Ptr>(),
        /*argsStatements*/ std::vector<std::vector<Statement::Ptr>>(),
        /*properties*/ std::set<Statement::Property>()}}};
  const AnnotationsMap actual = parseAnnotationsJson(j);
  ASSERT_EQ(expected, actual);
}

TEST(AnnotationsTest, KnownProperties) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [[]],
        "properties" : ["deterministic", "noreturn", "deterministic"]
    }
}
)");
  const std::set<Statement::Property> properties{
      Statement::Property::Deterministic, Statement::Property::Noreturn};
  const AnnotationsMap expected = {
      {"foo",
       {/*functionName*/ "foo",
        /*returnStatements*/ std::vector<Statement::Ptr>(),
        /*argsStatements*/ std::vector<std::vector<Statement::Ptr>>(),
        /*properties*/ properties}}};
  const AnnotationsMap actual = parseAnnotationsJson(j);
  ASSERT_EQ(expected, actual);
}

TEST(AnnotationsTest, UnknownProperties) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [[]],
        "properties" : ["noname", "noreturn", "deterministic"]
    }
}
)");
  const std::set<Statement::Property> properties{
      Statement::Property::Deterministic, Statement::Property::Noreturn,
      Statement::Property::Unknown};
  const AnnotationsMap expected = {
      {"foo",
       {/*functionName*/ "foo",
        /*returnStatements*/ std::vector<Statement::Ptr>(),
        /*argsStatements*/ std::vector<std::vector<Statement::Ptr>>(),
        /*properties*/ properties}}};
  const AnnotationsMap actual = parseAnnotationsJson(j);
  ASSERT_EQ(expected, actual);
}

TEST(AnnotationsTest, UnknownAnnotations) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [[], ["Ooo"]],
        "properties" : []
    }
}
)");
  const AnnotationsMap actual = parseAnnotationsJson(j);
  ASSERT_EQ(actual.at("foo").argsStatements[0][0]->getKind(),
            Statement::Kind::Unknown);
}

TEST(AnnotationsTest, KnownAnnotations) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [["MaybeInitNull"], ["Deref", "InitNull"]],
        "properties" : []
    }
}
)");
  const AnnotationsMap actual = parseAnnotationsJson(j);
  ASSERT_EQ(actual.at("foo").returnStatements[0]->getKind(),
            Statement::Kind::MaybeInitNull);
  ASSERT_EQ(actual.at("foo").argsStatements[0][0]->getKind(),
            Statement::Kind::Deref);
  ASSERT_EQ(actual.at("foo").argsStatements[0][1]->getKind(),
            Statement::Kind::InitNull);
}

TEST(AnnotationsTest, WithOffsets) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [["InitNull:*[10]*&"]],
        "properties" : []
    }
}
)");
  const AnnotationsMap actual = parseAnnotationsJson(j);
  ASSERT_EQ(actual.at("foo").returnStatements[0]->getKind(),
            Statement::Kind::InitNull);
  const std::vector<std::string> expectedOffset{"*", "10", "*", "&"};
  ASSERT_EQ(actual.at("foo").returnStatements[0]->offset, expectedOffset);
}
