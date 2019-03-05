#include <gtest/gtest.h>

#include "status.h"

namespace soir {
namespace {

TEST(StatusTest, Default) {
  Status status;

  EXPECT_EQ(status, StatusCode::OK);
}

TEST(StatusTest, Reuse) {
  Status status;

  status = StatusCode::INTERNAL_ERROR;

  EXPECT_EQ(status, StatusCode::INTERNAL_ERROR);
}

TEST(StatusOr, Default) {
  StatusOr<int> status;

  EXPECT_FALSE(status.Ok());
}

TEST(StatusOr, Ok) {
  StatusOr<int> status(42);

  EXPECT_TRUE(status.Ok());
  EXPECT_EQ(status.ValueOrDie(), 42);
}

TEST(StatusOr, Ko) {
  StatusOr<int> status(StatusCode::INTERNAL_ERROR);

  EXPECT_FALSE(status.Ok());
  EXPECT_DEATH(status.ValueOrDie(), "");
}

TEST(StatusOr, Death) {
  StatusOr<int> status;

  EXPECT_DEATH(status.ValueOrDie(), "");
}

} // namespace

} // namespace soir
