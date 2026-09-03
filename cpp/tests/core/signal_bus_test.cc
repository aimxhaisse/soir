#include "core/signal_bus.hh"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace soir {
namespace {

// Sample i of block b encodes both, so blocks are easy to tell apart.
std::vector<float> MakeBlockData(int block) {
  std::vector<float> data(kBlockSize);
  for (int i = 0; i < kBlockSize; ++i) {
    data[i] = static_cast<float>(block * 10000 + i) * 0.0001f;
  }
  return data;
}

}  // namespace

TEST(SignalBusTest, LatestWithoutPublishIsFalse) {
  SignalBus bus;
  bus.Declare("a");

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // No block published yet, and no previous block at tick 0.
  EXPECT_FALSE(bus.Latest("a", kBlockSize, left.data(), right.data()));
  EXPECT_FALSE(bus.Latest("a", 0, left.data(), right.data()));
  EXPECT_FALSE(bus.Latest("unknown", kBlockSize, left.data(), right.data()));
}

TEST(SignalBusTest, PublishToUnknownOrOversizedIsDropped) {
  SignalBus bus;
  bus.Declare("a");

  std::vector<float> data(kBlockSize);
  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  bus.Publish("unknown", 0, data.data(), data.data(), kBlockSize);
  bus.Publish("a", 0, data.data(), data.data(), kBlockSize + 1);
  bus.Publish("a", 0, data.data(), data.data(), 0);
  EXPECT_FALSE(bus.Latest("unknown", kBlockSize, left.data(), right.data()));
  EXPECT_FALSE(bus.Latest("a", kBlockSize, left.data(), right.data()));
}

TEST(SignalBusTest, PublishAndReadPreviousBlock) {
  SignalBus bus;
  bus.Declare("a");

  auto data = MakeBlockData(0);
  bus.Publish("a", 0, data.data(), data.data(), kBlockSize);

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // A reader rendering block 1 sees block 0.
  ASSERT_TRUE(bus.Latest("a", kBlockSize, left.data(), right.data()));
  for (int i = 0; i < kBlockSize; ++i) {
    EXPECT_FLOAT_EQ(left[i], data[i]);
    EXPECT_FLOAT_EQ(right[i], data[i]);
  }

  // A reader rendering block 0 has no previous block.
  EXPECT_FALSE(bus.Latest("a", 0, left.data(), right.data()));
}

TEST(SignalBusTest, AlwaysReadsPreviousBlock) {
  SignalBus bus;
  bus.Declare("a");

  const int num_blocks = SignalBus::kDepth + 4;
  std::vector<std::vector<float>> data(num_blocks + 1);
  for (int b = 0; b <= num_blocks; ++b) {
    data[b] = MakeBlockData(b);
  }

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // Engine pattern: publish block b, read it back from block b + 1,
  // across the ring wrap-around.
  for (int b = 0; b < num_blocks; ++b) {
    bus.Publish("a", b * kBlockSize, data[b].data(), data[b].data(),
                kBlockSize);
    ASSERT_TRUE(
        bus.Latest("a", (b + 1) * kBlockSize, left.data(), right.data()))
        << "reader of block " << b + 1;
    EXPECT_FLOAT_EQ(left[7], data[b][7]);
    EXPECT_FLOAT_EQ(right[7], data[b][7]);
  }
}

TEST(SignalBusTest, WrapAroundOverwritesOldestSlot) {
  SignalBus bus;
  bus.Declare("a");

  const int num_blocks = SignalBus::kDepth + 2;
  std::vector<std::vector<float>> data(num_blocks);
  for (int b = 0; b < num_blocks; ++b) {
    data[b] = MakeBlockData(b);
    bus.Publish("a", b * kBlockSize, data[b].data(), data[b].data(),
                kBlockSize);
  }

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // The most recent kDepth blocks are readable, but block 0's slot has
  // been overwritten: no block rather than stale data.
  for (int r = num_blocks - SignalBus::kDepth + 1; r <= num_blocks; ++r) {
    ASSERT_TRUE(bus.Latest("a", r * kBlockSize, left.data(), right.data()))
        << "reader of block " << r;
    EXPECT_FLOAT_EQ(left[0], data[r - 1][0]);
  }
  EXPECT_FALSE(bus.Latest("a", kBlockSize, left.data(), right.data()));
}

TEST(SignalBusTest, ReadCopyStableAcrossCurrentBlockPublishes) {
  SignalBus bus;
  bus.Declare("a");
  bus.Declare("b");

  auto d0 = MakeBlockData(0);
  auto d1 = MakeBlockData(1);
  bus.Publish("a", 0, d0.data(), d0.data(), kBlockSize);

  // Data copied out for block 1 is unaffected by publishing block 1.
  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);
  ASSERT_TRUE(bus.Latest("a", kBlockSize, left.data(), right.data()));

  bus.Publish("a", kBlockSize, d1.data(), d1.data(), kBlockSize);
  bus.Publish("b", kBlockSize, d1.data(), d1.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i) {
    EXPECT_FLOAT_EQ(left[i], d0[i]);
    EXPECT_FLOAT_EQ(right[i], d0[i]);
  }

  ASSERT_TRUE(bus.Latest("a", 2 * kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(left[3], d1[3]);
}

TEST(SignalBusTest, SignalsAreIndependent) {
  SignalBus bus;
  bus.Declare("a");
  bus.Declare("b");

  auto da = MakeBlockData(1);
  auto db = MakeBlockData(2);
  bus.Publish("a", 0, da.data(), da.data(), kBlockSize);
  bus.Publish("b", 0, db.data(), db.data(), kBlockSize);

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  ASSERT_TRUE(bus.Latest("a", kBlockSize, left.data(), right.data()));
  const float a_sample = left[5];
  ASSERT_TRUE(bus.Latest("b", kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(a_sample, da[5]);
  EXPECT_FLOAT_EQ(left[5], db[5]);
}

TEST(SignalBusTest, RemovedSignalReportsNoBlock) {
  SignalBus bus;
  bus.Declare("a");

  // Publish the last kDepth blocks, then the track is removed: the
  // storage stays (append-only) and the tick check stops serving.
  for (int b = 0; b < SignalBus::kDepth; ++b) {
    auto db = MakeBlockData(b);
    bus.Publish("a", b * kBlockSize, db.data(), db.data(), kBlockSize);
  }

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // Block 4 still sees block 3; block 5 has no block 4: "no block".
  ASSERT_TRUE(bus.Latest("a", 4 * kBlockSize, left.data(), right.data()));
  EXPECT_FALSE(bus.Latest("a", 5 * kBlockSize, left.data(), right.data()));

  // A late publish from a not-yet-joined thread is harmless: served
  // once, then "no block" again.
  auto dlate = MakeBlockData(99);
  bus.Publish("a", 4 * kBlockSize, dlate.data(), dlate.data(), kBlockSize);
  ASSERT_TRUE(bus.Latest("a", 5 * kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(left[1], dlate[1]);
  EXPECT_FALSE(bus.Latest("a", 6 * kBlockSize, left.data(), right.data()));
}

TEST(SignalBusTest, ReDeclareResumesOnSameStorage) {
  SignalBus bus;
  bus.Declare("a");

  auto d0 = MakeBlockData(0);
  bus.Publish("a", 0, d0.data(), d0.data(), kBlockSize);

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);
  ASSERT_TRUE(bus.Latest("a", kBlockSize, left.data(), right.data()));

  // Re-declaring is a no-op; the new publisher resumes on the same
  // storage, picked up from its first block on.
  bus.Declare("a");

  auto d2 = MakeBlockData(2);
  bus.Publish("a", 2 * kBlockSize, d2.data(), d2.data(), kBlockSize);
  ASSERT_TRUE(bus.Latest("a", 3 * kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(left[1], d2[1]);
}

TEST(SignalBusTest, DeclaredReflectsRegistry) {
  SignalBus bus;

  // Unknown until declared, and stays declared (append-only).
  EXPECT_FALSE(bus.Declared("a"));
  bus.Declare("a");
  EXPECT_TRUE(bus.Declared("a"));
  bus.Declare("a");
  EXPECT_TRUE(bus.Declared("a"));
  EXPECT_FALSE(bus.Declared("b"));
}

TEST(SignalBusTest, MasterSignalName) {
  // The master signal is a reserved internal name, like any other.
  SignalBus bus;
  bus.Declare(std::string(kMasterSignal));

  auto data = MakeBlockData(1);
  bus.Publish(std::string(kMasterSignal), 0, data.data(), data.data(),
              kBlockSize);

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);
  ASSERT_TRUE(bus.Latest(std::string(kMasterSignal), kBlockSize, left.data(),
                         right.data()));
  EXPECT_FLOAT_EQ(left[0], data[0]);
}

}  // namespace soir
