#pragma once

#include "DiskManager.hpp"
#include "Page.hpp"
#include "Record.hpp"
#include "TupleSerializer.hpp"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace storage {

class PageManager {
public:
  PageManager() : PageManager(temporaryPath(), true) {}
  explicit PageManager(const std::string& path) : PageManager(path, false) {}

  PageManager(const PageManager&) = delete;
  PageManager& operator=(const PageManager&) = delete;

  ~PageManager()
  {
    m_disk.reset();
    if (m_temporary) std::remove(m_path.c_str());
  }

  RowID insert(Tuple tuple)
  {
    const auto bytes = TupleSerializer::serialize(tuple);
    if (bytes.size() > Page::MAX_RECORD_SIZE) {
      throw std::length_error("PageManager: el registro no cabe en una pagina");
    }
    const auto count = m_disk->pageCount();
    if (count > 0) {
      const auto last = static_cast<PageID>(count);
      Page page = m_disk->readPage(last);
      if (page.canInsert(bytes.size())) {
        const auto slot = page.insert(bytes);
        if (!slot) throw std::logic_error("PageManager: canInsert y insert discrepan");
        m_disk->writePage(last, page);
        ++m_size;
        return makeRowID(last, *slot);
      }
    }
    Page fresh;
    const auto slot = fresh.insert(bytes);
    if (!slot) throw std::logic_error("PageManager: pagina nueva sin espacio");
    const auto id = static_cast<PageID>(count + 1);
    m_disk->writePage(id, fresh);
    ++m_size;
    return makeRowID(id, *slot);
  }

  [[nodiscard]] std::optional<Record> get(RowID id) const
  {
    if (!pageExists(id)) return std::nullopt;
    const auto page = m_disk->readPage(pageID(id));
    const auto bytes = page.get(slotID(id));
    if (!bytes) return std::nullopt;
    return Record{id, TupleSerializer::deserialize(*bytes)};
  }

  [[nodiscard]] bool exists(RowID id) const
  {
    if (!pageExists(id)) return false;
    return m_disk->readPage(pageID(id)).get(slotID(id)).has_value();
  }

  bool update(RowID id, Tuple tuple)
  {
    if (!pageExists(id)) return false;
    auto page = m_disk->readPage(pageID(id));
    const auto result = page.update(slotID(id), TupleSerializer::serialize(tuple));
    if (result != UpdateResult::Ok) return false;
    m_disk->writePage(pageID(id), page);
    return true;
  }

  bool erase(RowID id)
  {
    if (!pageExists(id)) return false;
    auto page = m_disk->readPage(pageID(id));
    if (!page.erase(slotID(id))) return false;
    m_disk->writePage(pageID(id), page);
    --m_size;
    return true;
  }

  [[nodiscard]] std::vector<Record> scan() const
  {
    std::vector<Record> records;
    records.reserve(m_size);
    for (std::size_t p = 1; p <= m_disk->pageCount(); ++p) {
      const auto page = m_disk->readPage(static_cast<PageID>(p));
      for (std::size_t s = 0; s < page.slotCount(); ++s) {
        const auto bytes = page.get(static_cast<SlotID>(s));
        if (bytes) {
          records.push_back(Record{makeRowID(static_cast<PageID>(p),
                                              static_cast<SlotID>(s)),
                                   TupleSerializer::deserialize(*bytes)});
        }
      }
    }
    return records;
  }

  void clear()
  {
    m_disk->reset();
    m_size = 0;
  }

  [[nodiscard]] std::size_t size() const { return m_size; }
  [[nodiscard]] bool empty() const { return m_size == 0; }
  [[nodiscard]] std::size_t pageCount() const { return m_disk->pageCount(); }

private:
  PageManager(const std::string& path, bool temporary)
    : m_path(path), m_temporary(temporary),
      m_disk(std::make_unique<DiskManager>(path))
  {
    for (std::size_t p = 1; p <= m_disk->pageCount(); ++p) {
      const auto page = m_disk->readPage(static_cast<PageID>(p));
      for (std::size_t s = 0; s < page.slotCount(); ++s) {
        if (page.get(static_cast<SlotID>(s))) ++m_size;
      }
    }
  }

  static std::string temporaryPath()
  {
    static std::uint64_t counter = 0;
    const auto stamp =
      std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("db_engine_" + std::to_string(stamp) + "_" +
             std::to_string(counter++) + ".tmp")).string();
  }

  [[nodiscard]] bool pageExists(RowID id) const
  {
    return isValidRowID(id) && pageID(id) <= m_disk->pageCount();
  }

  std::string m_path;
  bool m_temporary{false};
  std::unique_ptr<DiskManager> m_disk;
  std::size_t m_size{0};
};

}  // namespace storage
