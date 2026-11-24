#include "test_framework.hpp"

#include "storage/RecordManager.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

using storage::Tuple;

Tuple big(std::int64_t key) { return Tuple{key, std::string(1000, 'x')}; }

struct TempPath {
  std::string path;
  explicit TempPath(const std::string& name)
    : path((std::filesystem::temp_directory_path() / name).string())
  { std::remove(path.c_str()); }
  ~TempPath() { std::remove(path.c_str()); }
};

}  // namespace

TEST_CASE(page_manager_round_trips_tuples_through_disk_pages) {
  storage::RecordManager records;
  const Tuple t{std::int64_t{7}, 2.5, std::string{"hola"}};
  const auto id = records.insert(t);
  EXPECT_TRUE(records.exists(id));
  EXPECT_TRUE(records.get(id)->tuple == t);
  EXPECT_EQ(records.get(id)->id, id);
  EXPECT_EQ(records.size(), 1U);
}

TEST_CASE(page_manager_opens_a_new_page_when_the_last_is_full) {
  storage::RecordManager records;
  storage::RowID ids[6];
  for (int i = 0; i < 6; ++i) ids[i] = records.insert(big(i));
  // ~1013 bytes por registro: 4 caben en una pagina de 4096
  EXPECT_EQ(storage::pageID(ids[0]), static_cast<storage::PageID>(1));
  EXPECT_EQ(storage::slotID(ids[3]), static_cast<storage::SlotID>(3));
  EXPECT_EQ(storage::pageID(ids[4]), static_cast<storage::PageID>(2));
  EXPECT_EQ(storage::slotID(ids[4]), static_cast<storage::SlotID>(0));
  EXPECT_EQ(records.pageCount(), 2U);
}

TEST_CASE(page_manager_erase_update_and_invalid_ids) {
  storage::RecordManager records;
  const auto id = records.insert(Tuple{std::int64_t{1}, std::string{"a"}});
  EXPECT_TRUE(records.update(id, Tuple{std::int64_t{1}, std::string{"mas largo"}}));
  EXPECT_TRUE(records.get(id)->tuple ==
              (Tuple{std::int64_t{1}, std::string{"mas largo"}}));
  EXPECT_TRUE(records.erase(id));
  EXPECT_TRUE(!records.exists(id));
  EXPECT_TRUE(!records.erase(id));
  EXPECT_TRUE(!records.get(id).has_value());
  EXPECT_TRUE(!records.exists(storage::INVALID_ROW_ID));
  EXPECT_TRUE(!records.exists(storage::makeRowID(99, 0)));
  EXPECT_EQ(records.size(), 0U);
}

TEST_CASE(page_manager_update_that_does_not_fit_returns_false_and_keeps_old) {
  storage::RecordManager records;
  const auto first = records.insert(big(1));
  for (int i = 2; i <= 4; ++i) records.insert(big(i));   // pagina 1 llena
  EXPECT_TRUE(!records.update(first, Tuple{std::int64_t{1}, std::string(2000, 'y')}));
  EXPECT_TRUE(records.get(first)->tuple == big(1));
}

TEST_CASE(page_manager_rejects_records_larger_than_a_page) {
  storage::RecordManager records;
  bool rejected = false;
  try {
    records.insert(Tuple{std::string(5000, 'z')});
  } catch (const std::length_error&) {
    rejected = true;
  }
  EXPECT_TRUE(rejected);
  EXPECT_EQ(records.size(), 0U);
  EXPECT_EQ(records.pageCount(), 0U);
}

TEST_CASE(page_manager_scan_returns_live_records_in_storage_order) {
  storage::RecordManager records;
  const auto a = records.insert(Tuple{std::int64_t{1}});
  const auto b = records.insert(Tuple{std::int64_t{2}});
  const auto c = records.insert(Tuple{std::int64_t{3}});
  records.erase(b);
  const auto all = records.scan();
  EXPECT_EQ(all.size(), 2U);
  EXPECT_EQ(all[0].id, a);
  EXPECT_EQ(all[1].id, c);
}

TEST_CASE(page_manager_data_survives_reopening_the_file) {
  TempPath file("db_test_pm_reopen.dat");
  storage::RowID id = 0;
  {
    storage::PageManager pages(file.path);
    id = pages.insert(Tuple{std::int64_t{42}, std::string{"persistente"}});
    pages.insert(Tuple{std::int64_t{43}});
  }
  storage::PageManager reopened(file.path);
  EXPECT_EQ(reopened.size(), 2U);
  EXPECT_TRUE(reopened.get(id)->tuple ==
              (Tuple{std::int64_t{42}, std::string{"persistente"}}));
}

TEST_CASE(page_manager_clear_empties_the_file) {
  storage::RecordManager records;
  for (int i = 0; i < 10; ++i) records.insert(big(i));
  records.clear();
  EXPECT_TRUE(records.empty());
  EXPECT_EQ(records.pageCount(), 0U);
  const auto id = records.insert(Tuple{std::int64_t{1}});
  EXPECT_EQ(storage::pageID(id), static_cast<storage::PageID>(1));
}

// DOCUMENTA EL RIESGO PENDIENTE: el slot liberado se reutiliza con el mismo
// RowID. Cuando implementes la proteccion (generacion por slot), esta prueba
// debe cambiar a EXPECT_TRUE(next != id).
TEST_CASE(page_manager_reuses_rowid_of_freed_slot_in_last_page) {
  storage::RecordManager records;
  const auto id = records.insert(Tuple{std::int64_t{1}});
  records.erase(id);
  const auto next = records.insert(Tuple{std::int64_t{2}});
  EXPECT_EQ(next, id);
}
