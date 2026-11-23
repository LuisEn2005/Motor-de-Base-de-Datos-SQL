#include "catalog.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

namespace sem {

  bool CatalogManager::addTable(const std::string& name, const std::string& storage, std::string& error) {
    if (tables.find(name) != tables.end()) {
      error = "La tabla ya existe: " + name;
      return false;
    }
    TableInfo t;
    t.name = name;
    t.storage = storage;
    tables[name] = t;
    return true;
  }

  bool CatalogManager::addColumn(const std::string& table, const std::string& name, DataType type,
      bool primaryKey, bool nullable, std::string& error) {
    auto it = tables.find(table);
    if (it == tables.end()) {
      error = "Tabla no encontrada: " + table;
      return false;
    }

    TableInfo& tInfo = it->second;
    if (tInfo.columnIndex.find(name) != tInfo.columnIndex.end()) {
      error = "La columna " + name + " ya existe en la tabla " + table;
      return false;
    }

    ColumnInfo cInfo;
    cInfo.name = name;
    cInfo.table = table;
    cInfo.type = type;
    cInfo.primaryKey = primaryKey;
    cInfo.nullable = nullable;
    cInfo.ordinal = static_cast<int>(tInfo.columns.size());

    tInfo.columnIndex[name] = tInfo.columns.size();
    tInfo.columns.push_back(cInfo);

    return true;
  }

  bool CatalogManager::loadFromFile(const std::string& path, std::string& error) {
    std::ifstream file(path);
    if (!file.is_open()) {
      error = "No se pudo abrir el archivo: " + path;
      return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str(), error);
  }

  bool CatalogManager::loadFromString(const std::string& text, std::string& error) {
    std::istringstream file(text);
    std::string line;
    bool isFirstLine = true;

    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '\r') continue;

      if (isFirstLine) {
        isFirstLine = false;
        continue;
      }

      std::stringstream ss(line);
      std::string tName, cName, typeStr, pkStr;

      std::getline(ss, tName, ',');
      std::getline(ss, cName, ',');
      std::getline(ss, typeStr, ',');
      std::getline(ss, pkStr, ',');
      if (!ss && pkStr.empty()) continue;

      if (!pkStr.empty() && pkStr.back() == '\r') pkStr.pop_back();

      if (tables.find(tName) == tables.end()) {
        addTable(tName, "disk", error);
      }

      auto dtypeOpt = parseDataType(typeStr);
      DataType dtype = dtypeOpt ? *dtypeOpt : DataType::UNKNOWN;
      bool isPk = (pkStr == "true" || pkStr == "1");

      if (!addColumn(tName, cName, dtype, isPk, !isPk, error)) {
        return false;
      }
    }
    return true;
  }

  std::vector<std::string> CatalogManager::tableNames() const {
    std::vector<std::string> names;
    for (const auto& kv : tables) names.push_back(kv.first);
    std::sort(names.begin(), names.end());
    return names;
  }

  const TableInfo* CatalogManager::findTable(const std::string& name) const {
    auto it = tables.find(name);
    return (it != tables.end()) ? &it->second : nullptr;
  }

  const ColumnInfo* CatalogManager::findColumn(const std::string& table, const std::string& column) const {
    auto tIt = tables.find(table);
    if (tIt != tables.end()) {
      auto cIt = tIt->second.columnIndex.find(column);
      if (cIt != tIt->second.columnIndex.end()) {
        return &tIt->second.columns[cIt->second];
      }
    }
    return nullptr;
  }
}
