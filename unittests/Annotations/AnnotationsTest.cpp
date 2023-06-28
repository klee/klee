#include "gtest/gtest.h"

#include "klee/Module/Annotation.h"

#include <vector>

using namespace klee;

TEST(AnnotationsTest, Empty) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [],
        "properties" : []
    }
}
)");
  const Annotations expected = Annotations{
      {"foo", Annotation{"foo", std::vector<Annotation::StatementPtrs>(),
                         std::set<Annotation::Property>()}}};
  const Annotations actual = parseAnnotationsFile(j);
  ASSERT_EQ(expected, actual);
}

TEST(AnnotationsTest, KnownProperties) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [],
        "properties" : ["determ", "noreturn", "determ"]
    }
}
)");
  const std::set<Annotation::Property> properties{
      Annotation::Property::Determ, Annotation::Property::Noreturn};
  const Annotations expected = Annotations{
      {"foo", Annotation{"foo", std::vector<Annotation::StatementPtrs>(),
                         properties}}};
  const Annotations actual = parseAnnotationsFile(j);
  ASSERT_EQ(expected, actual);
}

TEST(AnnotationsTest, UnknownProperties) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [],
        "properties" : ["derm", "noreturn", "determ"]
    }
}
)");
  const std::set<Annotation::Property> properties{
      Annotation::Property::Determ, Annotation::Property::Noreturn,
      Annotation::Property::Unknown};
  const Annotations expected = Annotations{
      {"foo", Annotation{"foo", std::vector<Annotation::StatementPtrs>(),
                         properties}}};
  const Annotations actual = parseAnnotationsFile(j);
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
  const Annotations actual = parseAnnotationsFile(j);
  ASSERT_EQ(actual.at("foo").statements[1][0]->getStatementName(),
            Annotation::StatementKind::Unknown);
}

TEST(AnnotationsTest, KnownAnnotations) {
  const json j = json::parse(R"(
{
    "foo" : {
        "annotation" : [["InitNull"], ["Deref", "InitNull"]],
        "properties" : []
    }
}
)");
  const Annotations actual = parseAnnotationsFile(j);
  ASSERT_EQ(actual.at("foo").statements[0][0]->getStatementName(),
            Annotation::StatementKind::InitNull);
  ASSERT_EQ(actual.at("foo").statements[1][0]->getStatementName(),
            Annotation::StatementKind::Deref);
  ASSERT_EQ(actual.at("foo").statements[1][1]->getStatementName(),
            Annotation::StatementKind::InitNull);
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
  const Annotations actual = parseAnnotationsFile(j);
  ASSERT_EQ(actual.at("foo").statements[0][0]->getStatementName(),
            Annotation::StatementKind::InitNull);
  const std::vector<std::string> expectedOffset{"*", "10", "*", "&"};
  ASSERT_EQ(actual.at("foo").statements[0][0]->offset, expectedOffset);
}
