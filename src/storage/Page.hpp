#pragma once

#include "RowID.hpp"
#include "TupleSerializer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <vector>

namespace storage {

enum class UpdateResult { Ok, InvalidSlot, NoSpace };

class Page {
public:
  using Buffer = TupleSerializer::Buffer;
  static constexpr std::size_t DEFAULT_SIZE = 4096;
  static constexpr std::size_t HEADER_SIZE = 4;
  static constexpr std::size_t ENTRY_SIZE = 4;
  static constexpr std::uint16_t EMPTY = 0xFFFF;

  explicit Page(std::size_t size = DEFAULT_SIZE) : m_bytes(size, 0)
  {
    checkSize(size);
    setFreeStart(HEADER_SIZE);
  }

  explicit Page(std::vector<std::uint8_t> raw) : m_bytes(std::move(raw))
  {
    checkSize(m_bytes.size());
    if (freeStart() < HEADER_SIZE ||
        freeStart() + ENTRY_SIZE * slotCount() > m_bytes.size()) {
      throw std::runtime_error("Page: cabecera corrupta");
    }
  }

  std::optional<SlotID> insert(const Buffer& bytes)
  {
    const auto len = bytes.size();
    const auto reusable = firstEmptySlot();
    const std::size_t needed = len + (reusable ? 0 : ENTRY_SIZE);
    if (len > 0xFFFF || !fitsTotal(needed)) {
      return std::nullopt;
    }
    if (contiguousFree() < needed) {
      compact();
    }
    SlotID slot;
    if (reusable) {
      slot = *reusable;
    } else {
      slot = static_cast<SlotID>(slotCount());
      setSlotCount(slotCount() + 1);
    }
    place(slot, bytes);
    return slot;
  }

  std::optional<Buffer> get(SlotID slot) const
  {
    if (!isLive(slot)) return std::nullopt;
    const auto off = entryOffset(slot);
    const auto len = entryLength(slot);
    return Buffer(m_bytes.begin() + off, m_bytes.begin() + off + len);
  }

  bool erase(SlotID slot)
  {
    if (!isLive(slot)) return false;
    setEntry(slot, EMPTY, 0);
    return true;
  }

  UpdateResult update(SlotID slot, const Buffer& bytes)
  {
    if (!isLive(slot)) return UpdateResult::InvalidSlot;
    const auto oldLen = entryLength(slot);
    const auto len = bytes.size();
    if (len > 0xFFFF) return UpdateResult::NoSpace;

    if (len <= oldLen) {
      std::memcpy(&m_bytes[entryOffset(slot)], bytes.data(), len);
      setEntry(slot, entryOffset(slot), static_cast<std::uint16_t>(len));
      return UpdateResult::Ok;
    }
    if (freeSpace() + oldLen < len) return UpdateResult::NoSpace;

    if (contiguousFree() >= len) {
      place(slot, bytes);
      return UpdateResult::Ok;
    }
    setEntry(slot, EMPTY, 0);
    compact();
    place(slot, bytes);
    return UpdateResult::Ok;
  }

  std::size_t freeSpace() const
  {
    return m_bytes.size() - HEADER_SIZE - ENTRY_SIZE * slotCount() - liveBytes();
  }

  bool canInsert(std::size_t length) const
  {
    const auto needed = length + (firstEmptySlot() ? 0 : ENTRY_SIZE);
    return length <= 0xFFFF && fitsTotal(needed);
  }

  std::size_t slotCount() const { return readU16(0); }
  std::size_t size() const { return m_bytes.size(); }
  const std::vector<std::uint8_t>& raw() const { return m_bytes; }

private:
  static void checkSize(std::size_t size)
  {
    if (size < HEADER_SIZE + ENTRY_SIZE || size > 0xFFFF) {
      throw std::invalid_argument("Page: tamaño invalido");
    }
  }

  bool fitsTotal(std::size_t needed) const { return needed <= freeSpace(); }

  std::uint16_t readU16(std::size_t pos) const
  {
    return static_cast<std::uint16_t>(m_bytes[pos] | (m_bytes[pos + 1] << 8));
  }
  void writeU16(std::size_t pos, std::uint16_t v)
  {
    m_bytes[pos] = static_cast<std::uint8_t>(v & 0xFF);
    m_bytes[pos + 1] = static_cast<std::uint8_t>(v >> 8);
  }

  std::size_t freeStart() const { return readU16(2); }
  void setFreeStart(std::size_t v) { writeU16(2, static_cast<std::uint16_t>(v)); }
  void setSlotCount(std::size_t v) { writeU16(0, static_cast<std::uint16_t>(v)); }

  std::size_t entryPos(SlotID slot) const
  {
    return m_bytes.size() - ENTRY_SIZE * (static_cast<std::size_t>(slot) + 1);
  }
  std::uint16_t entryOffset(SlotID s) const { return readU16(entryPos(s)); }
  std::uint16_t entryLength(SlotID s) const { return readU16(entryPos(s) + 2); }
  void setEntry(SlotID s, std::uint16_t off, std::uint16_t len)
  {
    writeU16(entryPos(s), off);
    writeU16(entryPos(s) + 2, len);
  }

  bool isLive(SlotID slot) const
  {
    return slot < slotCount() && entryOffset(slot) != EMPTY;
  }

  std::optional<SlotID> firstEmptySlot() const
  {
    for (std::size_t i = 0; i < slotCount(); ++i) {
      if (entryOffset(static_cast<SlotID>(i)) == EMPTY) {
        return static_cast<SlotID>(i);
      }
    }
    return std::nullopt;
  }

  std::size_t liveBytes() const
  {
    std::size_t total = 0;
    for (std::size_t i = 0; i < slotCount(); ++i) {
      if (entryOffset(static_cast<SlotID>(i)) != EMPTY) {
        total += entryLength(static_cast<SlotID>(i));
      }
    }
    return total;
  }

  std::size_t contiguousFree() const
  {
    return m_bytes.size() - ENTRY_SIZE * slotCount() - freeStart();
  }

  void place(SlotID slot, const Buffer& bytes)
  {
    const auto off = freeStart();
    if (!bytes.empty()) std::memcpy(&m_bytes[off], bytes.data(), bytes.size());
    setEntry(slot, static_cast<std::uint16_t>(off),
             static_cast<std::uint16_t>(bytes.size()));
    setFreeStart(off + bytes.size());
  }

  void compact()
  {
    std::vector<SlotID> live;
    for (std::size_t i = 0; i < slotCount(); ++i) {
      if (entryOffset(static_cast<SlotID>(i)) != EMPTY) {
        live.push_back(static_cast<SlotID>(i));
      }
    }
    std::sort(live.begin(), live.end(), [this](SlotID a, SlotID b) {
      return entryOffset(a) < entryOffset(b);
    });
    std::size_t cursor = HEADER_SIZE;
    for (const auto slot : live) {
      const auto off = entryOffset(slot);
      const auto len = entryLength(slot);
      if (off != cursor) std::memmove(&m_bytes[cursor], &m_bytes[off], len);
      setEntry(slot, static_cast<std::uint16_t>(cursor), len);
      cursor += len;
    }
    setFreeStart(cursor);
  }

  std::vector<std::uint8_t> m_bytes;
};

}  // namespace storage
