#ifndef DATA_TYPE_H
#define DATA_TYPE_H


#include <cctype>
#include <optional>
#include <string>

namespace sem {

  enum class DataType {
    INT,
    FLOAT,
    VARCHAR,
    BOOLEAN,
    UNKNOWN
  };

  inline std::string toString(DataType t) {
    switch (t) {
      case DataType::INT:     return "INT";
      case DataType::FLOAT:   return "FLOAT";
      case DataType::VARCHAR: return "VARCHAR";
      case DataType::BOOLEAN: return "BOOLEAN";
      case DataType::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
  }

  inline std::optional<DataType> parseDataType(const std::string& name) {
    std::string up = name;
    for (char& c : up) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (up == "INT")     return DataType::INT;
    if (up == "FLOAT")   return DataType::FLOAT;
    if (up == "VARCHAR") return DataType::VARCHAR;
    if (up == "BOOLEAN") return DataType::BOOLEAN;
    return std::nullopt;
  }

  inline bool isNumeric(DataType t) { return t == DataType::INT || t == DataType::FLOAT; }

  inline DataType promoteNumeric(DataType a, DataType b) {
    if (!isNumeric(a) || !isNumeric(b)) return DataType::UNKNOWN;
    return (a == DataType::FLOAT || b == DataType::FLOAT) ? DataType::FLOAT : DataType::INT;
  }

}

#endif // DATA_TYPE_H
