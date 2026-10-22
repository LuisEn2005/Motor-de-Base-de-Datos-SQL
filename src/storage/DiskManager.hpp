#pragma once

#include "Page.hpp"
#include "RowID.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

namespace storage {

class DiskManager {
public:
  static constexpr std::size_t PAGE_SIZE = Page::DEFAULT_SIZE;

  explicit DiskManager(const std::string& path) : m_path(path)
  {
    { std::ofstream create(path, std::ios::binary | std::ios::app); }
    m_file.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!m_file) {
      throw std::runtime_error("DiskManager: no se pudo abrir " + path);
    }
    m_file.seekg(0, std::ios::end);
    const auto bytes = static_cast<std::size_t>(m_file.tellg());
    if (bytes % PAGE_SIZE != 0) {
      throw std::runtime_error("DiskManager: tamaño de archivo corrupto");
    }
    m_pageCount = bytes / PAGE_SIZE;
  }

  Page readPage(PageID id)
  {
    if (id == INVALID_PAGE_ID || id > m_pageCount) {
      throw std::out_of_range("DiskManager: la pagina no existe");
    }
    std::vector<std::uint8_t> bytes(PAGE_SIZE);
    m_file.clear();
    m_file.seekg(offsetOf(id));
    m_file.read(reinterpret_cast<char*>(bytes.data()),
                static_cast<std::streamsize>(PAGE_SIZE));
    if (!m_file) throw std::runtime_error("DiskManager: fallo de lectura");
    return Page(std::move(bytes));
  }

  void writePage(PageID id, const Page& page)
  {
    if (id == INVALID_PAGE_ID || id > m_pageCount + 1) {
      throw std::out_of_range("DiskManager: PageID invalido o con hueco");
    }
    if (page.size() != PAGE_SIZE) {
      throw std::invalid_argument("DiskManager: tamaño de pagina incorrecto");
    }
    m_file.clear();
    m_file.seekp(offsetOf(id));
    m_file.write(reinterpret_cast<const char*>(page.raw().data()),
                 static_cast<std::streamsize>(PAGE_SIZE));
    m_file.flush();
    if (!m_file) throw std::runtime_error("DiskManager: fallo de escritura");
    if (id == m_pageCount + 1) ++m_pageCount;
  }

  [[nodiscard]] std::size_t pageCount() const { return m_pageCount; }

private:
  static std::streamoff offsetOf(PageID id)
  {
    return static_cast<std::streamoff>(id - 1) *
           static_cast<std::streamoff>(PAGE_SIZE);
  }

  std::string m_path;
  std::fstream m_file;
  std::size_t m_pageCount{0};
};

}  // namespace storage
