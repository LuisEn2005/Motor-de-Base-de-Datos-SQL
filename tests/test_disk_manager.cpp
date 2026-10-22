#include "test_framework.hpp"

#include "../src/storage/DiskManager.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

using storage::DiskManager;
using storage::Page;

struct TempFile {
  std::string path;
  explicit TempFile(const std::string& name)
    : path((std::filesystem::temp_directory_path() / name).string())
  {
    std::remove(path.c_str());
  }
  ~TempFile() { std::remove(path.c_str()); }
};

Page pageWith(std::uint8_t fill, std::size_t length = 10)
{
  Page page;
  page.insert(Page::Buffer(length, fill));
  return page;
}

}  // namespace

TEST_CASE(disk_page_survives_closing_and_reopening) {
  TempFile file("db_test_roundtrip.dat");
  {
    DiskManager disk(file.path);
    disk.writePage(1, pageWith(0xAB));
  }
  DiskManager reopened(file.path);
  EXPECT_EQ(reopened.pageCount(), 1U);
  const auto page = reopened.readPage(1);
  EXPECT_TRUE(page.get(0).has_value());
  EXPECT_TRUE(*page.get(0) == Page::Buffer(10, 0xAB));
}

TEST_CASE(disk_pages_keep_their_own_identity) {
  TempFile file("db_test_identity.dat");
  DiskManager disk(file.path);
  disk.writePage(1, pageWith(0x11));
  disk.writePage(2, pageWith(0x22));
  disk.writePage(3, pageWith(0x33));
  EXPECT_EQ(disk.pageCount(), 3U);
  EXPECT_TRUE(*disk.readPage(1).get(0) == Page::Buffer(10, 0x11));
  EXPECT_TRUE(*disk.readPage(2).get(0) == Page::Buffer(10, 0x22));
  EXPECT_TRUE(*disk.readPage(3).get(0) == Page::Buffer(10, 0x33));
}

TEST_CASE(disk_overwrite_replaces_page_without_growing_file) {
  TempFile file("db_test_overwrite.dat");
  DiskManager disk(file.path);
  disk.writePage(1, pageWith(0x11));
  disk.writePage(2, pageWith(0x22));
  disk.writePage(1, pageWith(0x99, 30));
  EXPECT_EQ(disk.pageCount(), 2U);
  EXPECT_TRUE(*disk.readPage(1).get(0) == Page::Buffer(30, 0x99));
  EXPECT_TRUE(*disk.readPage(2).get(0) == Page::Buffer(10, 0x22));
}

TEST_CASE(disk_read_of_missing_page_throws_out_of_range) {
  TempFile file("db_test_missing.dat");
  DiskManager disk(file.path);
  bool emptyFileRejected = false;
  bool zeroRejected = false;
  bool pastEndRejected = false;
  try { (void)disk.readPage(1); } catch (const std::out_of_range&) { emptyFileRejected = true; }
  disk.writePage(1, pageWith(0x01));
  try { (void)disk.readPage(0); } catch (const std::out_of_range&) { zeroRejected = true; }
  try { (void)disk.readPage(2); } catch (const std::out_of_range&) { pastEndRejected = true; }
  EXPECT_TRUE(emptyFileRejected);
  EXPECT_TRUE(zeroRejected);
  EXPECT_TRUE(pastEndRejected);
}

TEST_CASE(disk_write_rejects_gaps_and_invalid_ids) {
  TempFile file("db_test_gaps.dat");
  DiskManager disk(file.path);
  bool gapRejected = false;
  bool zeroRejected = false;
  try { disk.writePage(5, pageWith(0x01)); } catch (const std::out_of_range&) { gapRejected = true; }
  try { disk.writePage(0, pageWith(0x01)); } catch (const std::out_of_range&) { zeroRejected = true; }
  EXPECT_TRUE(gapRejected);
  EXPECT_TRUE(zeroRejected);
  EXPECT_EQ(disk.pageCount(), 0U);
}

TEST_CASE(disk_write_rejects_pages_of_the_wrong_size) {
  TempFile file("db_test_size.dat");
  DiskManager disk(file.path);
  bool rejected = false;
  try { disk.writePage(1, Page(64)); } catch (const std::invalid_argument&) { rejected = true; }
  EXPECT_TRUE(rejected);
  EXPECT_EQ(disk.pageCount(), 0U);
}

TEST_CASE(disk_page_with_erased_slots_round_trips) {
  TempFile file("db_test_erased.dat");
  {
    DiskManager disk(file.path);
    Page page;
    page.insert(Page::Buffer(10, 0xA1));
    page.insert(Page::Buffer(20, 0xB2));
    page.erase(0);
    disk.writePage(1, page);
  }
  DiskManager disk(file.path);
  const auto page = disk.readPage(1);
  EXPECT_TRUE(!page.get(0).has_value());
  EXPECT_TRUE(*page.get(1) == Page::Buffer(20, 0xB2));
}
