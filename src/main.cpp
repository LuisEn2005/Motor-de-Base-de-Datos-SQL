#include "benchmark/ScanBenchmark.hpp"
#include "demo/DemoEngine.hpp"
#include "sql/executor/SQLEngine.hpp"
#include "sql/semantic/catalog.h"
#include "storage/RecordManager.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void showMenu()
{
  std::cout << "\n=== MOTOR DE BASE DE DATOS — FASE 2 ===\n"
            << "1. Carga e insercion masiva\n"
            << "2. Mostrar splits y estructura del B-Tree\n"
            << "3. Comparar Index Scan vs Full Table Scan\n"
            << "4. Modo SQL interactivo\n"
            << "5. Salir\n"
            << "========================================\n"
            << "Seleccione una opcion: ";
}

std::size_t readPositive(const std::string& prompt)
{
  std::cout << prompt;
  std::size_t value = 0;
  if (!(std::cin >> value) || value == 0) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    throw std::invalid_argument("se esperaba un entero positivo");
  }
  return value;
}

void printMetric(const char* name, const db::benchmark::ScanMetrics& metric)
{
  std::cout << std::left << std::setw(20) << name << std::right
            << std::setw(14) << db::benchmark::milliseconds(metric.elapsed)
            << std::setw(16) << metric.pageAccesses
            << std::setw(14) << metric.found << '\n';
}

// ── Imprime una tabla de resultados de forma alineada ──────────
void printQueryResult(const db::sql::QueryResult& res)
{
  if (res.rows.empty()) {
    std::cout << "(0 filas)\n";
    return;
  }

  // Calcular ancho de cada columna
  std::vector<std::size_t> widths(res.columnNames.size(), 0);
  for (std::size_t i = 0; i < res.columnNames.size(); ++i)
    widths[i] = res.columnNames[i].size();
  for (const auto& row : res.rows)
    for (std::size_t i = 0; i < row.columns.size() && i < widths.size(); ++i)
      widths[i] = std::max(widths[i], row.columns[i].second.size());

  // Separador
  auto printSep = [&]() {
    std::cout << '+';
    for (auto w : widths) std::cout << std::string(w + 2, '-') << '+';
    std::cout << '\n';
  };

  // Encabezado
  printSep();
  std::cout << '|';
  for (std::size_t i = 0; i < res.columnNames.size(); ++i)
    std::cout << ' ' << std::left << std::setw(static_cast<int>(widths[i]))
              << res.columnNames[i] << " |";
  std::cout << '\n';
  printSep();

  // Filas
  for (const auto& row : res.rows) {
    std::cout << '|';
    for (std::size_t i = 0; i < row.columns.size(); ++i)
      std::cout << ' ' << std::left << std::setw(static_cast<int>(widths[i]))
                << row.columns[i].second << " |";
    std::cout << '\n';
  }
  printSep();
  std::cout << res.rows.size() << " fila(s)\n";
}

// ── Modo SQL interactivo ───────────────────────────────────────
void runSQLMode(db::demo::DemoEngine& demo)
{
  if (!demo.loaded()) {
    std::cout << "Primero cargue datos con la opcion 1.\n";
    return;
  }

  // Cargar catálogo
  sem::CatalogManager catalog;
  std::string catError;
  if (!catalog.loadFromFile("data/catalog.csv", catError)) {
    // Si no encuentra el archivo, crear un catálogo mínimo en memoria
    std::string csvInline =
      "table_name,column_name,data_type,is_primary_key\n"
      "registros,id,INT,true\n"
      "registros,nombre,VARCHAR,false\n";
    catalog.loadFromString(csvInline, catError);
    std::cout << "[aviso] catalog.csv no encontrado; usando catalogo minimo (registros: id, nombre).\n";
  }

  // Obtener RecordManager del DemoEngine a través del index (solo lectura)
  // Como RecordManager vive dentro del DemoEngine, creamos uno de demostración
  // sincronizado con la vista del motor.
  // Para el SQL interactivo usamos un RecordManager fresco que el executor pueda consultar.
  // En Fase 3 se unificará el RecordManager en el motor central.
  storage::RecordManager localRecords;

  db::sql::SQLEngine engine(std::move(catalog), localRecords);

  std::cout << "\n=== SQL Server — modo interactivo ===\n"
            << "Escribe una consulta SELECT y presiona Enter dos veces.\n"
            << "Escribe 'exit' para volver al menu principal.\n\n";

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  while (true) {
    std::cout << "sql> ";
    std::string line, sql;
    while (std::getline(std::cin, line)) {
      if (line == "exit") { return; }
      if (line.empty()) break;
      sql += line + " ";
    }
    if (line == "exit") break;
    if (sql.empty()) continue;

    auto resp = engine.query(sql);
    if (resp.ok) {
      printQueryResult(resp.result);
    } else {
      std::cout << "[Error " << resp.stage << "] " << resp.errorMsg << '\n';
    }
  }
}

}  // namespace

int main()
{
  db::demo::DemoEngine demo;
  bool running = true;
  while (running) {
    showMenu();
    int option = 0;
    if (!(std::cin >> option)) {
      std::cout << "\nEntrada finalizada.\n";
      break;
    }

    try {
      switch (option) {
      case 1: {
        const auto count = readPositive("Cantidad de registros: ");
        const auto degree = readPositive("Grado minimo t (>= 2): ");
        const auto tuples = db::demo::DemoEngine::makeWorkload(count);
        const auto summary = demo.load(tuples, degree, 256);
        std::cout << "\n[Carga completada]\n"
                  << "Tuplas insertadas: " << summary.load.inserted << '\n'
                  << "Tuplas rechazadas: " << summary.load.rejected << '\n'
                  << "Paginas de datos: " << summary.pages << '\n'
                  << "Altura del B-Tree: " << summary.height << '\n'
                  << "Splits: " << summary.splits << '\n'
                  << "Tiempo: " << std::fixed << std::setprecision(3)
                  << db::benchmark::milliseconds(summary.load.elapsed)
                  << " ms\n";
        break;
      }
      case 2:
        std::cout << '\n' << demo.splitReport() << demo.treeReport();
        break;
      case 3: {
        const auto queries = readPositive("Cantidad de consultas: ");
        const auto result = demo.benchmarkScans(queries);
        std::cout << '\n' << std::left << std::setw(20) << "Metodo" << std::right
                  << std::setw(14) << "Tiempo (ms)" << std::setw(16)
                  << "Paginas I/O" << std::setw(14) << "Encontrados" << '\n';
        printMetric("Index Scan", result.indexScan);
        printMetric("Full Table Scan", result.fullTableScan);
        break;
      }
      case 4:
        runSQLMode(demo);
        break;
      case 5:
        running = false;
        break;
      default:
        std::cout << "Opcion no valida.\n";
      }
    } catch (const std::exception& error) {
      std::cout << "Error: " << error.what() << '\n';
    }
  }
  return 0;
}
