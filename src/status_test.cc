#include <gtest/gtest.h>

#include "status.h"

namespace soir {
namespace {

TEST(StatusTest, Default) {
  Status status;

  EXPECT_EQ(status, StatusCode::OK);
}

} // namespace

} // namespace soir
