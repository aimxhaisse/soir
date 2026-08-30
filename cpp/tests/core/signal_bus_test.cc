#include "core/signal_bus.hh"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace soir {
namespace {

// Distinct data per block: sample i of block b encodes b and i so
// that blocks (and overwrites) are easy to tell apart.
std::vector<float> MakeBlockData(int block) {
  std::vector<float> data(kBlockSize);
  for (int i = 0; i < kBlockSize; ++i) {
    data[i] = static_cast<float>(block * 10000 + i) * 0.0001f;
  }
  return data;
}

}  // namespace

TEST(SignalBusTest, LatestWithoutPublishIsNull) {
  SignalBus bus;
  bus.Declare("a");

  // No block published yet, and no previous block at tick 0.
  EXPECT_EQ(bus.Latest("a", kBlockSize), nullptr);
  EXPECT_EQ(bus.Latest("a", 0), nullptr);

  // Unknown names are nullptr too.
  EXPECT_EQ(bus.Latest("unknown", kBlockSize), nullptr);
  EXPECT_FALSE(bus.Declared("unknown"));
  EXPECT_TRUE(bus.Declared("a"));
}

TEST(SignalBusTest, PublishAndReadPreviousBlock) {
  SignalBus bus;
  bus.Declare("a");

  auto data = MakeBlockData(0);
  bus.Publish("a", 0, data.data(), data.data(), kBlockSize);

  // A reader rendering block 1 sees block 0.
  const Block* block = bus.Latest("a", kBlockSize);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->tick, 0);
  for (int i = 0; i < kBlockSize; ++i) {
    EXPECT_FLOAT_EQ(block->left[i], data[i]);
    EXPECT_FLOAT_EQ(block->right[i], data[i]);
  }

  // A reader rendering block 0 has no previous block.
  EXPECT_EQ(bus.Latest("a", 0), nullptr);
}

TEST(SignalBusTest, AlwaysReadsPreviousBlock) {
  SignalBus bus;
  bus.Declare("a");

  const int num_blocks = SignalBus::kDepth + 4;
  std::vector<std::vector<float>> data(num_blocks + 1);
  for (int b = 0; b <= num_blocks; ++b) {
    data[b] = MakeBlockData(b);
  }

  // Mirror the engine: block b is published while block b is rendered,
  // then the reader of block b + 1 (the next engine iteration) reads
  // it back. This holds across the ring wrap-around.
  for (int b = 0; b < num_blocks; ++b) {
    bus.Publish("a", b * kBlockSize, data[b].data(), data[b].data(),
                kBlockSize);
    const Block* block = bus.Latest("a", (b + 1) * kBlockSize);
    ASSERT_NE(block, nullptr) << "reader of block " << b + 1;
    EXPECT_EQ(block->tick, b * kBlockSize);
    EXPECT_FLOAT_EQ(block->left[7], data[b][7]);
    EXPECT_FLOAT_EQ(block->right[7], data[b][7]);
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

  // The most recent kDepth blocks are readable...
  for (int r = num_blocks - SignalBus::kDepth + 1; r <= num_blocks; ++r) {
    const Block* block = bus.Latest("a", r * kBlockSize);
    ASSERT_NE(block, nullptr) << "reader of block " << r;
    EXPECT_EQ(block->tick, (r - 1) * kBlockSize);
  }

  // ...but block 0's slot has been overwritten by block 4, so a
  // reader asking for it gets nullptr rather than stale data.
  EXPECT_EQ(bus.Latest("a", kBlockSize), nullptr);
}

TEST(SignalBusTest, ReaderPointerStableAcrossCurrentBlockPublishes) {
  SignalBus bus;
  bus.Declare("a");
  bus.Declare("b");

  auto d0 = MakeBlockData(0);
  auto d1 = MakeBlockData(1);
  bus.Publish("a", 0, d0.data(), d0.data(), kBlockSize);

  // A reader of block 1 reads the slot of block 0. Publishing block 1
  // (what happens concurrently in the engine, to any signal) writes
  // slot 1, not slot 0: the held pointer stays valid with its data.
  const Block* held = bus.Latest("a", kBlockSize);
  ASSERT_NE(held, nullptr);
  EXPECT_EQ(held->tick, 0);

  bus.Publish("a", kBlockSize, d1.data(), d1.data(), kBlockSize);
  bus.Publish("b", kBlockSize, d1.data(), d1.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i) {
    EXPECT_FLOAT_EQ(held->left[i], d0[i]);
    EXPECT_FLOAT_EQ(held->right[i], d0[i]);
  }

  // The next read of "a" now returns block 1.
  const Block* block = bus.Latest("a", 2 * kBlockSize);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->tick, kBlockSize);
  EXPECT_FLOAT_EQ(block->left[3], d1[3]);
}

TEST(SignalBusTest, SignalsAreIndependent) {
  SignalBus bus;
  bus.Declare("a");
  bus.Declare("b");

  auto da = MakeBlockData(1);
  auto db = MakeBlockData(2);
  bus.Publish("a", 0, da.data(), da.data(), kBlockSize);
  bus.Publish("b", 0, db.data(), db.data(), kBlockSize);

  const Block* block_a = bus.Latest("a", kBlockSize);
  const Block* block_b = bus.Latest("b", kBlockSize);
  ASSERT_NE(block_a, nullptr);
  ASSERT_NE(block_b, nullptr);
  EXPECT_FLOAT_EQ(block_a->left[5], da[5]);
  EXPECT_FLOAT_EQ(block_b->left[5], db[5]);
}

TEST(SignalBusTest, PublishUndeclaredIsDropped) {
  SignalBus bus;

  auto data = MakeBlockData(0);
  // No crash, no allocation: the block is dropped.
  bus.Publish("never-declared", 0, data.data(), data.data(), kBlockSize);
  EXPECT_EQ(bus.Latest("never-declared", kBlockSize), nullptr);
}

TEST(SignalBusTest, UndeclareStopsReading) {
  SignalBus bus;
  bus.Declare("a");

  auto d0 = MakeBlockData(0);
  auto d1 = MakeBlockData(1);
  bus.Publish("a", 0, d0.data(), d0.data(), kBlockSize);

  bus.Undeclare("a");
  EXPECT_FALSE(bus.Declared("a"));
  EXPECT_EQ(bus.Latest("a", kBlockSize), nullptr);

  // Late publishes from a not-yet-joined track are dropped.
  bus.Publish("a", kBlockSize, d1.data(), d1.data(), kBlockSize);
  EXPECT_EQ(bus.Latest("a", 2 * kBlockSize), nullptr);

  // Undeclaring unknown names is a no-op.
  bus.Undeclare("unknown");
}

TEST(SignalBusTest, ReDeclareRevivesSignal) {
  SignalBus bus;
  bus.Declare("a");

  auto d0 = MakeBlockData(0);
  auto d2 = MakeBlockData(2);
  bus.Publish("a", 0, d0.data(), d0.data(), kBlockSize);

  bus.Undeclare("a");
  EXPECT_EQ(bus.Latest("a", kBlockSize), nullptr);

  // A track with the same name comes back: the signal is revived and
  // the new publisher's blocks are readable again.
  bus.Declare("a");
  EXPECT_TRUE(bus.Declared("a"));

  bus.Publish("a", 2 * kBlockSize, d2.data(), d2.data(), kBlockSize);
  const Block* block = bus.Latest("a", 3 * kBlockSize);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->tick, 2 * kBlockSize);
  EXPECT_FLOAT_EQ(block->left[1], d2[1]);
}

TEST(SignalBusTest, MasterSignalName) {
  // The master signal has a reserved internal name.
  SignalBus bus;
  bus.Declare(std::string(kMasterSignal));
  EXPECT_TRUE(bus.Declared(std::string(kMasterSignal)));
}

}  // namespace soir
