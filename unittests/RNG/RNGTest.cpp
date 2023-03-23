#include "klee/ADT/RNG.h"

#include "gtest/gtest.h"

using namespace klee;

/* test equality with default seed */
TEST(RNG, InitialSeedEquality) {
  RNG noseed;
  RNG seed(5489U);

  ASSERT_EQ(noseed.getBool(), seed.getBool());
  ASSERT_EQ(noseed.getInt32(), seed.getInt32());
  ASSERT_EQ(noseed.getDouble(), seed.getDouble());
  ASSERT_EQ(noseed.getDoubleL(), seed.getDoubleL());
}


/* test inequality with default seed */
TEST(RNG, InitialSeedInEquality) {
  RNG noseed;
  RNG seed(42U);

  ASSERT_NE(noseed.getInt32(), seed.getInt32());
}


/* test inequality with zero seed */
TEST(RNG, InitialSeedZeroInEquality) {
  RNG noseed;
  RNG seed(0U);

  ASSERT_NE(noseed.getInt32(), seed.getInt32());
}


/* test equality with seed provided by ctor and seed() */
TEST(RNG, SeedEquality) {
  RNG noseed;
  noseed.seed(42U);
  RNG seed(42U);

  ASSERT_EQ(noseed.getInt32(), seed.getInt32());
}
