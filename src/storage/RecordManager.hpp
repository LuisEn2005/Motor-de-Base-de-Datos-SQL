#pragma once

#include "PageManager.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace storage {

class RecordManager {
public:
  RecordManager() = default;
  explicit RecordManager(const std::string& path) : m_pages(path) {}

  RowID insert(Tuple tuple) { return m_pages.insert(std::move(tuple)); }
  [[nodiscard]] std::optional<Record> get(RowID id) const { return m_pages.get(id); }
  [[nodiscard]] bool exists(RowID id) const { return m_pages.exists(id); }
  bool update(RowID id, Tuple tuple) { return m_pages.update(id, std::move(tuple)); }
  bool erase(RowID id) { return m_pages.erase(id); }

  [[nodiscard]] std::size_t size() const { return m_pages.size(); }
  [[nodiscard]] bool empty() const { return m_pages.empty(); }
  [[nodiscard]] std::vector<Record> scan() const { return m_pages.scan(); }
  [[nodiscard]] std::size_t pageCount() const { return m_pages.pageCount(); }

  void clear() { m_pages.clear(); }

private:
  PageManager m_pages;
};

}  // namespace storage
