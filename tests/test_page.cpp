#include "test_framework.hpp"

#include "../src/storage/Page.hpp"

#include <cstdint>

namespace {

using storage::Page;
using storage::UpdateResult;

Page::Buffer bytes(std::size_t n, std::uint8_t fill) { return Page::Buffer(n, fill); }

struct Fixture {
  Page page{64};
  storage::SlotID a, b, c;
  Fixture()
  {
    a = *page.insert(bytes(10, 0xA1));
    b = *page.insert(bytes(20, 0xB2));
    c = *page.insert(bytes(15, 0xC3));
  }
};

}  // namespace

TEST_CASE(page_accounting_matches_the_hand_exercise) {
  Fixture f;
  EXPECT_EQ(f.page.slotCount(), 3U);
  EXPECT_EQ(f.page.freeSpace(), 3U);
  EXPECT_TRUE(f.page.erase(f.b));
  EXPECT_EQ(f.page.freeSpace(), 23U);
}

TEST_CASE(insert_fails_when_it_does_not_fit_even_compacting) {
  Fixture f;
  f.page.erase(f.b);
  EXPECT_TRUE(!f.page.insert(bytes(25, 0xFF)).has_value());
  EXPECT_TRUE(!f.page.canInsert(25));
  EXPECT_EQ(f.page.get(f.a)->size(), 10U);
}

TEST_CASE(insert_compacts_when_total_space_is_enough) {
  Fixture f;
  f.page.erase(f.b);
  EXPECT_TRUE(f.page.canInsert(22));
  const auto slot = f.page.insert(bytes(22, 0xD4));
  EXPECT_TRUE(slot.has_value());
  EXPECT_EQ(*slot, f.b);
  EXPECT_TRUE(*f.page.get(f.a) == bytes(10, 0xA1));
  EXPECT_TRUE(*f.page.get(f.c) == bytes(15, 0xC3));
  EXPECT_TRUE(*f.page.get(*slot) == bytes(22, 0xD4));
  EXPECT_EQ(f.page.slotCount(), 3U);
}

TEST_CASE(erase_on_empty_or_missing_slot_returns_false) {
  Fixture f;
  EXPECT_TRUE(f.page.erase(f.b));
  EXPECT_TRUE(!f.page.erase(f.b));
  EXPECT_TRUE(!f.page.erase(99));
}

TEST_CASE(get_on_empty_or_missing_slot_returns_nullopt) {
  Fixture f;
  f.page.erase(f.b);
  EXPECT_TRUE(!f.page.get(f.b).has_value());
  EXPECT_TRUE(!f.page.get(99).has_value());
}

TEST_CASE(erase_then_insert_reuses_the_same_slot) {
  Fixture f;
  f.page.erase(f.b);
  const auto slot = f.page.insert(bytes(5, 0x11));
  EXPECT_TRUE(slot.has_value());
  EXPECT_EQ(*slot, f.b);
}

TEST_CASE(update_that_does_not_fit_keeps_the_old_record) {
  Fixture f;
  EXPECT_TRUE(f.page.update(f.a, bytes(40, 0x77)) == UpdateResult::NoSpace);
  EXPECT_TRUE(*f.page.get(f.a) == bytes(10, 0xA1));
  EXPECT_TRUE(f.page.update(99, bytes(1, 0)) == UpdateResult::InvalidSlot);
}

TEST_CASE(update_grows_shrinks_and_compacts_without_moving_slots) {
  Fixture f;
  EXPECT_TRUE(f.page.update(f.b, bytes(8, 0x55)) == UpdateResult::Ok);
  EXPECT_TRUE(*f.page.get(f.b) == bytes(8, 0x55));
  EXPECT_TRUE(f.page.update(f.a, bytes(14, 0x66)) == UpdateResult::Ok);
  EXPECT_TRUE(*f.page.get(f.a) == bytes(14, 0x66));
  EXPECT_TRUE(*f.page.get(f.b) == bytes(8, 0x55));
  EXPECT_TRUE(*f.page.get(f.c) == bytes(15, 0xC3));
}

TEST_CASE(page_round_trips_through_raw_bytes) {
  Fixture f;
  f.page.erase(f.b);
  Page copy(f.page.raw());
  EXPECT_TRUE(*copy.get(f.a) == bytes(10, 0xA1));
  EXPECT_TRUE(!copy.get(f.b).has_value());
  EXPECT_TRUE(*copy.get(f.c) == bytes(15, 0xC3));
}

TEST_CASE(zero_length_records_are_not_confused_with_empty_slots) {
  Page page(64);
  const auto s = page.insert(Page::Buffer{});
  EXPECT_TRUE(s.has_value());
  EXPECT_TRUE(page.get(*s).has_value());
  EXPECT_EQ(page.get(*s)->size(), 0U);
}

TEST_CASE(full_page_rejects_inserts_until_something_is_erased) {
  Page page;
  std::size_t count = 0;
  while (page.insert(bytes(100, 0x01))) ++count;
  EXPECT_EQ(count, 39U);
  EXPECT_TRUE(page.erase(0));
  EXPECT_TRUE(page.insert(bytes(100, 0x02)).has_value());
}
