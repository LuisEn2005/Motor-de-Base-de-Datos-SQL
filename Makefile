CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2

TARGET           = db_engine
SRC              = src/main.cpp
TEST_TARGET      := build/tests
TEST_SOURCES     := $(wildcard tests/test_*.cpp)
BENCHMARK_TARGET := build/benchmark

# ── Cabeceras existentes ────────────────────────────────────────
INDEX_HEADERS     := $(wildcard src/index/*.hpp)
STORAGE_HEADERS   := $(wildcard src/storage/*.hpp)
BENCHMARK_HEADERS := $(wildcard src/benchmark/*.hpp)
DEMO_HEADERS      := $(wildcard src/demo/*.hpp)

# ── Compilador SQL (nuevos) ─────────────────────────────────────
SQL_LEXER_SRC   := src/sql/lexer/token.cpp \
                   src/sql/lexer/lexer.cpp  \
                   src/sql/lexer/input.cpp
SQL_PARSER_SRC  := src/sql/parser/grammar.cpp    \
                   src/sql/parser/parser.cpp      \
                   src/sql/parser/parse_table.cpp \
                   src/sql/parser/trace.cpp
SQL_SEM_SRC     := src/sql/semantic/catalog.cpp          \
                   src/sql/semantic/ast_builder.cpp       \
                   src/sql/semantic/ast_printer.cpp       \
                   src/sql/semantic/semantic_analyzer.cpp
SQL_SOURCES     := $(SQL_LEXER_SRC) $(SQL_PARSER_SRC) $(SQL_SEM_SRC)
SQL_HEADERS     := $(wildcard src/sql/lexer/*.h)      \
                   $(wildcard src/sql/parser/*.h)     \
                   $(wildcard src/sql/semantic/*.h)   \
                   $(wildcard src/sql/executor/*.hpp)

# ── Flags de include ────────────────────────────────────────────
INCLUDES := -Isrc -Isrc/sql/lexer -Isrc/sql/parser -Isrc/sql/semantic -Isrc/sql/executor

# ───────────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRC) $(SQL_SOURCES) \
           $(INDEX_HEADERS) $(STORAGE_HEADERS) \
           $(BENCHMARK_HEADERS) $(DEMO_HEADERS) $(SQL_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SRC) $(SQL_SOURCES)

clean:
	rm -rf $(TARGET) build

# ── Tests (suite original, sin SQL por ahora) ───────────────────
$(TEST_TARGET): $(TEST_SOURCES) tests/test_framework.hpp \
                $(INDEX_HEADERS) $(STORAGE_HEADERS) $(BENCHMARK_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc -Itests $(TEST_SOURCES) -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

# ── Benchmark ──────────────────────────────────────────────────
$(BENCHMARK_TARGET): src/benchmark_main.cpp \
                     $(BENCHMARK_HEADERS) $(INDEX_HEADERS) $(STORAGE_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc src/benchmark_main.cpp -o $@

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET) $(ARGS)

.PHONY: all clean test benchmark
