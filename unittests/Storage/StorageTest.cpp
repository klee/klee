#include "gtest/gtest.h"

#include "klee/ADT/FixedSizeStorageAdapter.h"
#include "klee/ADT/StorageAdapter.h"

using namespace klee;

TEST(StorageTest, StorageAdapter) {
  std::unique_ptr<StorageAdapter<unsigned char>> uma(
      new UnorderedMapAdapder<unsigned char>());
  std::unique_ptr<StorageAdapter<unsigned char>> puma(
      new PersistenUnorderedMapAdapder<unsigned char>());
  std::unique_ptr<StorageAdapter<unsigned char>> saa(
      new SparseArrayAdapter<unsigned char>(0, 12));
  uma->set(0, 1);
  puma->set(0, 1);
  saa->set(0, 1);
  ASSERT_EQ(uma->at(0), 1);
  ASSERT_EQ(puma->at(0), 1);
  ASSERT_EQ(saa->at(0), 1);
  char sum = 0;
  for (const auto &val : *uma) {
    sum += val.second;
  }
  for (const auto &val : *puma) {
    sum += val.second;
  }
  for (const auto &val : *saa) {
    sum += val.second;
  }
  ASSERT_EQ(sum, 3);
}

TEST(StorageTest, FixedSizeStorageAdapter) {
  std::unique_ptr<FixedSizeStorageAdapter<unsigned char>> va(
      new VectorAdapter<unsigned char>(32));
  std::unique_ptr<FixedSizeStorageAdapter<unsigned char>> pva(
      new PersistentVectorAdapter<unsigned char>(32));
  std::unique_ptr<FixedSizeStorageAdapter<unsigned char>> aa(
      new ArrayAdapter<unsigned char>(32));
  va->set(0, 1);
  pva->set(0, 1);
  aa->set(0, 1);
  ASSERT_EQ(va->at(0), 1);
  ASSERT_EQ(pva->at(0), 1);
  ASSERT_EQ(aa->at(0), 1);
  char sum = 0;
  for (const auto &val : *va) {
    sum += val;
  }
  for (const auto &val : *pva) {
    sum += val;
  }
  for (const auto &val : *aa) {
    sum += val;
  }
  ASSERT_EQ(sum, 3);
}
