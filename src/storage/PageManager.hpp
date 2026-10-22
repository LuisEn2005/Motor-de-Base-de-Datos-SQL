#pragma once

#include "Record.hpp"
#include "TupleSerializer.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace storage {

class PageManager {
public:
  static constexpr std::size_t DEFAULT_PAGE_CAPACITY = 128;

  explicit PageManager(std::size_t pageCapacity = DEFAULT_PAGE_CAPACITY)
    : m_pageCapacity(pageCapacity)
  {
    if (m_pageCapacity <= 0) {
      throw std::invalid_argument("PageManager: page capacity must be positive");
    }
  }

  RowID insert(Tuple tuple)
  {
    const auto pageIndex = m_nextPosition / m_pageCapacity;
    const auto slotIndex = m_nextPosition % m_pageCapacity;
    ++m_nextPosition;
    if (pageIndex == m_pages.size()) {
      m_pages.emplace_back(m_pageCapacity);
    }
    return occupy(pageIndex, slotIndex, std::move(tuple));
  }

  [[nodiscard]] std::optional<Record> get(RowID id) const
  {
    const auto* slot = findSlot(id);
    if (slot == nullptr || !slot->has_value()) {
      return std::nullopt;
    }
    return Record{slot->value().id,
                  TupleSerializer::deserialize(slot->value().bytes)};
  }

  [[nodiscard]] bool exists(RowID id) const
  {
    const auto* slot = findSlot(id);
    return slot != nullptr && slot->has_value();
  }

  bool update(RowID id, Tuple tuple)
  {
    auto* slot = findSlot(id);
    if (slot == nullptr || !slot->has_value()) {
      return false;
    }
    slot->value().bytes = TupleSerializer::serialize(tuple);
    return true;
  }

  bool erase(RowID id)
  {
    auto* slot = findSlot(id);
    if (slot == nullptr || !slot->has_value()) {
      return false;
    }
    slot->reset();
    --m_size;
    return true;
  }

  [[nodiscard]] std::vector<Record> scan() const
  {
    std::vector<Record> records;
    records.reserve(m_size);
    for (const auto& page : m_pages) {
      for (const auto& slot : page) {
        if (slot.has_value()) {
          records.push_back(Record{
            slot->id, TupleSerializer::deserialize(slot->bytes)});
        }
      }
    }
    return records;
  }

  void clear()
  {
    m_pages.clear();
    m_size = 0;
    m_nextPosition = 0;
  }

  [[nodiscard]] std::size_t size() const { return m_size; }
  [[nodiscard]] bool empty() const { return m_size == 0; }
  [[nodiscard]] std::size_t pageCount() const { return m_pages.size(); }
  [[nodiscard]] std::size_t pageCapacity() const { return m_pageCapacity; }

private:
  struct StoredRecord {
    RowID id;
    TupleSerializer::Buffer bytes;
  };

  using Slot = std::optional<StoredRecord>;
  using Page = std::vector<Slot>;

  RowID occupy(std::size_t pageIndex, std::size_t slotIndex, Tuple tuple)
  {
    const auto page = static_cast<PageID>(pageIndex + 1);
    const auto slot = static_cast<SlotID>(slotIndex);
    const RowID id = makeRowID(page, slot);
    m_pages[pageIndex][slotIndex] =
      StoredRecord{id, TupleSerializer::serialize(tuple)};
    ++m_size;
    return id;
  }

  Slot* findSlot(RowID id)
  {
    if (!isValidRowID(id)) {
      return nullptr;
    }
    const auto pageIndex = static_cast<std::size_t>(pageID(id) - 1);
    const auto slotIndex = static_cast<std::size_t>(slotID(id));
    if (pageIndex >= m_pages.size() || slotIndex >= m_pageCapacity) {
      return nullptr;
    }
    return &m_pages[pageIndex][slotIndex];
  }

  const Slot* findSlot(RowID id) const
  {
    return const_cast<PageManager*>(this)->findSlot(id);
  }

  std::size_t m_pageCapacity;
  std::vector<Page> m_pages;
  std::size_t m_size{0};
  std::size_t m_nextPosition{0};
};

}  // namespace storage
