#ifndef CATALOG_H
#define CATALOG_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "data_type.h"

namespace sem {

  struct ColumnInfo {
    std::string name;
    std::string table;
    int ordinal = 0;
    DataType type = DataType::UNKNOWN;
    bool primaryKey = false;
    bool nullable = true;
  };

  struct TableInfo {
    std::string name;
    std::string storage;
    std::vector<ColumnInfo> columns;
    std::unordered_map<std::string, std::size_t> columnIndex;
  };

  class CatalogManager {
    public:
      bool addTable(const std::string& name, const std::string& storage, std::string& error);

      bool addColumn(const std::string& table, const std::string& name, DataType type,
          bool primaryKey, bool nullable, std::string& error);

      bool loadFromFile(const std::string& path, std::string& error);
      bool loadFromString(const std::string& text, std::string& error);

      const TableInfo*  findTable(const std::string& name) const;
      const ColumnInfo* findColumn(const std::string& table, const std::string& column) const;

      std::size_t tableCount() const { return tables.size(); }
      std::vector<std::string> tableNames() const;

    private:
      std::unordered_map<std::string, TableInfo> tables;
  };

}

#endif // CATALOG_H
