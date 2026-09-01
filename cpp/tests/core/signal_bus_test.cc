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

TEST(SignalBusTest, LatestWithoutPublishIsFalse) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // No block published yet, and no previous block at tick 0.
  EXPECT_FALSE(SignalBus::Latest(a, kBlockSize, left.data(), right.data()));
  EXPECT_FALSE(SignalBus::Latest(a, 0, left.data(), right.data()));

  // Unknown names have no handle.
  EXPECT_EQ(bus.Find("unknown"), nullptr);
  EXPECT_NE(bus.Find("a"), nullptr);
}

TEST(SignalBusTest, PublishAndReadPreviousBlock) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");

  auto data = MakeBlockData(0);
  SignalBus::Publish(a, 0, data.data(), data.data(), kBlockSize);

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // A reader rendering block 1 sees block 0.
  ASSERT_TRUE(SignalBus::Latest(a, kBlockSize, left.data(), right.data()));
  for (int i = 0; i < kBlockSize; ++i) {
    EXPECT_FLOAT_EQ(left[i], data[i]);
    EXPECT_FLOAT_EQ(right[i], data[i]);
  }

  // A reader rendering block 0 has no previous block.
  EXPECT_FALSE(SignalBus::Latest(a, 0, left.data(), right.data()));
}

TEST(SignalBusTest, AlwaysReadsPreviousBlock) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");

  const int num_blocks = SignalBus::kDepth + 4;
  std::vector<std::vector<float>> data(num_blocks + 1);
  for (int b = 0; b <= num_blocks; ++b) {
    data[b] = MakeBlockData(b);
  }

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // Mirror the engine: block b is published while block b is rendered,
  // then the reader of block b + 1 (the next engine iteration) reads
  // it back. This holds across the ring wrap-around.
  for (int b = 0; b < num_blocks; ++b) {
    SignalBus::Publish(a, b * kBlockSize, data[b].data(), data[b].data(),
                       kBlockSize);
    ASSERT_TRUE(
        SignalBus::Latest(a, (b + 1) * kBlockSize, left.data(), right.data()))
        << "reader of block " << b + 1;
    EXPECT_FLOAT_EQ(left[7], data[b][7]);
    EXPECT_FLOAT_EQ(right[7], data[b][7]);
  }
}

TEST(SignalBusTest, WrapAroundOverwritesOldestSlot) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");

  const int num_blocks = SignalBus::kDepth + 2;
  std::vector<std::vector<float>> data(num_blocks);
  for (int b = 0; b < num_blocks; ++b) {
    data[b] = MakeBlockData(b);
    SignalBus::Publish(a, b * kBlockSize, data[b].data(), data[b].data(),
                       kBlockSize);
  }

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // The most recent kDepth blocks are readable...
  for (int r = num_blocks - SignalBus::kDepth + 1; r <= num_blocks; ++r) {
    ASSERT_TRUE(SignalBus::Latest(a, r * kBlockSize, left.data(), right.data()))
        << "reader of block " << r;
    EXPECT_FLOAT_EQ(left[0], data[r - 1][0]);
  }

  // ...but block 0's slot has been overwritten by block 4, so a
  // reader asking for it gets no block rather than stale data.
  EXPECT_FALSE(SignalBus::Latest(a, kBlockSize, left.data(), right.data()));
}

TEST(SignalBusTest, ReadCopyStableAcrossCurrentBlockPublishes) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");
  SignalBus::Signal* b = bus.Declare("b");

  auto d0 = MakeBlockData(0);
  auto d1 = MakeBlockData(1);
  SignalBus::Publish(a, 0, d0.data(), d0.data(), kBlockSize);

  // A reader of block 1 reads block 0 into its own buffers. Publishing
  // block 1 (what happens concurrently in the engine, to any signal)
  // cannot affect the already-copied data.
  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);
  ASSERT_TRUE(SignalBus::Latest(a, kBlockSize, left.data(), right.data()));

  SignalBus::Publish(a, kBlockSize, d1.data(), d1.data(), kBlockSize);
  SignalBus::Publish(b, kBlockSize, d1.data(), d1.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i) {
    EXPECT_FLOAT_EQ(left[i], d0[i]);
    EXPECT_FLOAT_EQ(right[i], d0[i]);
  }

  // The next read of "a" now returns block 1.
  ASSERT_TRUE(SignalBus::Latest(a, 2 * kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(left[3], d1[3]);
}

TEST(SignalBusTest, SignalsAreIndependent) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");
  SignalBus::Signal* b = bus.Declare("b");

  auto da = MakeBlockData(1);
  auto db = MakeBlockData(2);
  SignalBus::Publish(a, 0, da.data(), da.data(), kBlockSize);
  SignalBus::Publish(b, 0, db.data(), db.data(), kBlockSize);

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  ASSERT_TRUE(SignalBus::Latest(a, kBlockSize, left.data(), right.data()));
  const float a_sample = left[5];
  ASSERT_TRUE(SignalBus::Latest(b, kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(a_sample, da[5]);
  EXPECT_FLOAT_EQ(left[5], db[5]);
}

TEST(SignalBusTest, PublishToNullIsDropped) {
  SignalBus bus;

  std::vector<float> data(kBlockSize);
  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // No crash, no write: null handles and oversized blocks are dropped.
  SignalBus::Publish(nullptr, 0, data.data(), data.data(), kBlockSize);
  SignalBus::Publish(nullptr, 0, data.data(), data.data(), kBlockSize + 1);
  EXPECT_FALSE(
      SignalBus::Latest(nullptr, kBlockSize, left.data(), right.data()));
  EXPECT_EQ(bus.Find("never-declared"), nullptr);
}

TEST(SignalBusTest, RemovedSignalReportsNoBlock) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");

  // Publish the last kDepth blocks, then the track is removed: the
  // publisher is gone, the signal keeps its storage (the registry is
  // append-only).
  for (int b = 0; b < SignalBus::kDepth; ++b) {
    auto db = MakeBlockData(b);
    SignalBus::Publish(a, b * kBlockSize, db.data(), db.data(), kBlockSize);
  }

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);

  // A reader of block 4 still sees block 3, the last published block...
  ASSERT_TRUE(SignalBus::Latest(a, 4 * kBlockSize, left.data(), right.data()));

  // ...but a reader of block 5 expects block 4, which was never
  // published: "no block", i.e. the compressor passes through.
  EXPECT_FALSE(SignalBus::Latest(a, 5 * kBlockSize, left.data(), right.data()));

  // The handle stays valid: a late publish from a not-yet-joined old
  // track thread is harmless — it serves at most the single block it
  // carries, then the signal reports "no block" again.
  auto dlate = MakeBlockData(99);
  SignalBus::Publish(a, 4 * kBlockSize, dlate.data(), dlate.data(), kBlockSize);
  ASSERT_TRUE(SignalBus::Latest(a, 5 * kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(left[1], dlate[1]);
  EXPECT_FALSE(SignalBus::Latest(a, 6 * kBlockSize, left.data(), right.data()));
}

TEST(SignalBusTest, ReDeclareReturnsSameStorage) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");

  auto d0 = MakeBlockData(0);
  auto d2 = MakeBlockData(2);
  SignalBus::Publish(a, 0, d0.data(), d0.data(), kBlockSize);

  std::vector<float> left(kBlockSize);
  std::vector<float> right(kBlockSize);
  ASSERT_TRUE(SignalBus::Latest(a, kBlockSize, left.data(), right.data()));

  // A track with the same name is created again (SetupTracks declares
  // every new track name up front): the same stable handle is returned
  // and the new publisher resumes on the same storage, so readers pick
  // it up from its first block on.
  SignalBus::Signal* a2 = bus.Declare("a");
  EXPECT_EQ(a2, a);

  SignalBus::Publish(a, 2 * kBlockSize, d2.data(), d2.data(), kBlockSize);
  ASSERT_TRUE(SignalBus::Latest(a, 3 * kBlockSize, left.data(), right.data()));
  EXPECT_FLOAT_EQ(left[1], d2[1]);
}

TEST(SignalBusTest, HandleStableAcrossBusLifetime) {
  SignalBus bus;
  SignalBus::Signal* a = bus.Declare("a");
  ASSERT_NE(a, nullptr);

  // The registry is append-only: re-declaring and lookups never
  // change the pointer, so audio threads may cache it.
  EXPECT_EQ(bus.Declare("a"), a);
  EXPECT_EQ(bus.Find("a"), a);
}

TEST(SignalBusTest, MasterSignalName) {
  // The master signal has a reserved internal name.
  SignalBus bus;
  SignalBus::Signal* master = bus.Declare(std::string(kMasterSignal));
  ASSERT_NE(master, nullptr);
  EXPECT_EQ(bus.Find(std::string(kMasterSignal)), master);
}

}  // namespace soir
