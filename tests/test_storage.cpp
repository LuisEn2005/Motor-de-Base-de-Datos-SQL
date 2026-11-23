#include "test_framework.hpp"

#include "storage/RecordManager.hpp"
#include "storage/RowID.hpp"
#include "storage/TupleSerializer.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE(row_id_maps_records_to_page_and_slot) {
    storage::RecordManager records;
    const auto first = records.insert(storage::Tuple{std::int64_t{1}});
    const auto second = records.insert(storage::Tuple{std::int64_t{2}});
    const auto third = records.insert(storage::Tuple{std::int64_t{3}});

    EXPECT_EQ(storage::pageID(first), static_cast<storage::PageID>(1));
    EXPECT_EQ(storage::slotID(first), static_cast<storage::SlotID>(0));
    EXPECT_EQ(storage::pageID(second), static_cast<storage::PageID>(1));
    EXPECT_EQ(storage::slotID(second), static_cast<storage::SlotID>(1));
    EXPECT_EQ(storage::pageID(third), static_cast<storage::PageID>(2));
    EXPECT_EQ(storage::slotID(third), static_cast<storage::SlotID>(0));
    EXPECT_EQ(records.pageCount(), 2U);
}

TEST_CASE(record_manager_supports_crud_and_stable_row_ids) {
    storage::RecordManager records;
    const storage::Tuple original{std::int64_t{7}, std::string{"original"}};
    const storage::Tuple updated{std::int64_t{7}, std::string{"updated"}};
    const auto id = records.insert(original);

    EXPECT_TRUE(records.exists(id));
    EXPECT_TRUE(records.get(id)->tuple == original);
    EXPECT_TRUE(records.update(id, updated));
    EXPECT_TRUE(records.get(id)->tuple == updated);
    EXPECT_TRUE(records.erase(id));
    EXPECT_TRUE(!records.exists(id));
    EXPECT_TRUE(!records.erase(id));

    const auto next = records.insert(storage::Tuple{std::int64_t{8}});
    EXPECT_TRUE(next != id);
    records.clear();
    EXPECT_TRUE(records.empty());
    EXPECT_EQ(records.pageCount(), 0U);
}

TEST_CASE(tuple_serializer_round_trips_every_supported_type) {
    const storage::Tuple tuple{
        std::int64_t{-9'223'372'036'854'775'000LL}, 3.141592653589793,
        std::string{"Árbol B / UTF-8"}};
    const auto bytes = storage::TupleSerializer::serialize(tuple);
    const auto restored = storage::TupleSerializer::deserialize(bytes);
    EXPECT_TRUE(restored == tuple);
}

TEST_CASE(tuple_serializer_rejects_corrupted_buffers) {
    const auto valid = storage::TupleSerializer::serialize(
        storage::Tuple{std::int64_t{42}, std::string{"record"}});

    for (std::size_t size = 0; size < valid.size(); ++size) {
        bool rejected = false;
        try {
            (void)storage::TupleSerializer::deserialize(
                storage::TupleSerializer::Buffer(valid.begin(), valid.begin() + size));
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        EXPECT_TRUE(rejected);
    }

    auto trailing = valid;
    trailing.push_back(0xFF);
    bool rejectedTrailing = false;
    try {
        (void)storage::TupleSerializer::deserialize(trailing);
    } catch (const std::runtime_error&) {
        rejectedTrailing = true;
    }
    EXPECT_TRUE(rejectedTrailing);
}
