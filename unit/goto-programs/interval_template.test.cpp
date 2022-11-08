#include <goto-programs/interval_template.h>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE(
  "Interval templates base functions are working for int",
  "[ai][interval-analysis]")
{
  interval_templatet<int> A;
  SECTION("Initialization tests")
  {
    REQUIRE(A.is_top());
    REQUIRE_FALSE(A.is_bottom());
    REQUIRE_FALSE(A.singleton());
  }
  SECTION("Lower bound is set")
  {
    A.make_ge_than(-3);
    REQUIRE_FALSE(A.is_top());
    REQUIRE_FALSE(A.is_bottom());
    REQUIRE_FALSE(A.singleton());
  }
  SECTION("Upper bound is set (singleton)")
  {
    A.make_ge_than(-3);
    A.make_le_than(0);
    REQUIRE_FALSE(A.is_top());
    REQUIRE_FALSE(A.is_bottom());
    REQUIRE_FALSE(A.singleton());
  }
  SECTION("Upper bound is set (singleton)")
  {
    A.make_ge_than(-3);
    A.make_le_than(-3);
    REQUIRE_FALSE(A.is_top());
    REQUIRE_FALSE(A.is_bottom());
    REQUIRE(A.singleton());
  }
  SECTION("Contradiction in set")
  {
    A.make_ge_than(0);
    A.make_le_than(-1);
    REQUIRE_FALSE(A.is_top());
    REQUIRE(A.is_bottom());
    REQUIRE_FALSE(A.singleton());
  }
}

TEST_CASE(
  "Interval templates relation functions are working for int",
  "[ai][interval-analysis]")
{
  interval_templatet<int> A;
  interval_templatet<int> B;

  SECTION("Union between two tops is a top")
  {
    REQUIRE(A.is_top());
    REQUIRE(B.is_top());
    A.join(B);
    REQUIRE(A.is_top());
  }

  SECTION("Union with a top results in a top")
  {
    A.make_ge_than(0);
    REQUIRE_FALSE(A.is_top());
    REQUIRE(B.is_top());
    A.join(B);
    REQUIRE(A.is_top());
  }

  SECTION("Intersection with a top results in itself")
  {
    A.make_ge_than(0);
    REQUIRE_FALSE(A.is_top());
    interval_templatet<int> previous = A;
    REQUIRE(B.is_top());
    A.meet(B);
    REQUIRE(A == previous);
  }

  SECTION("Union with a bottom results in itself")
  {
    A.make_ge_than(0);
    REQUIRE_FALSE(A.is_top());
    interval_templatet<int> previous = A;

    B.make_ge_than(1);
    B.make_le_than(0);
    REQUIRE(B.is_bottom());
    A.join(B);
    REQUIRE(A == previous);
  }

  SECTION("Intersect with a bottom results in a bottom")
  {
    REQUIRE(A.is_top());

    B.make_ge_than(1);
    B.make_le_than(0);
    REQUIRE(B.is_bottom());
    A.meet(B);
    REQUIRE(A.is_bottom());
  }

  SECTION("Union of two sets should over-aproximate")
  {
    // [0,4] U [10,20] = [0,20]
    A.make_ge_than(0);
    A.make_le_than(4);

    B.make_ge_than(10);
    B.make_le_than(20);

    interval_templatet<int> expected;
    expected.make_ge_than(0);
    expected.make_le_than(20);

    A.approx_union_with(B);
    REQUIRE(A == expected);
  }

  SECTION("Intersect should... intersect")
  {
    // [0,15] U [10,20] = [10,15]
    A.make_ge_than(0);
    A.make_le_than(15);

    B.make_ge_than(10);
    B.make_le_than(20);

    interval_templatet<int> expected;
    expected.make_ge_than(10);
    expected.make_le_than(15);

    A.meet(B);
    REQUIRE(A == expected);
  }

  SECTION("Intersect should... intersect 2")
  {
    // [0,5] U [10,20] = empty
    A.make_ge_than(0);
    A.make_le_than(5);

    B.make_ge_than(10);
    B.make_le_than(20);

    A.meet(B);
    REQUIRE(A.is_bottom());
  }
}