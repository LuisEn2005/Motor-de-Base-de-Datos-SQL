# Motor de Base de Datos - Fase 1: Índices B-Tree

Implementación en C++17 del almacenamiento e indexación de registros mediante
un Árbol B. La fase busca comparar una búsqueda indexada `O(log n)` con un
recorrido completo `O(n)`.

## Requisitos

- GCC o Clang con soporte para C++17.
- GNU Make.
- Linux, macOS o Windows mediante MinGW/WSL.

No se usan bibliotecas externas para compilar las pruebas ni el benchmark.

## Compilación y ejecución

```bash
make
./db_engine
```

El menú ejecuta el flujo completo: carga masiva, log y visualización de splits,
y comparación entre Index Scan y Full Table Scan.

## Pruebas automatizadas

```bash
make test
```

La suite usa las implementaciones reales de `RecordManager`, `RowID` y `BTree`.
Incluye carga desordenada de 10 000 tuplas, claves duplicadas, carga vacía,
splits de raíz, varios grados mínimos, validación independiente de ocupación y
profundidad, y equivalencia entre Index Scan y Full Table Scan.

## Benchmark

La ejecución predeterminada carga 100 000 registros y realiza 5 000 consultas:

```bash
make benchmark
```

El tamaño y el grado mínimo se pueden ajustar con
`ARGS="registros consultas grado"`:

```bash
make benchmark ARGS="20000 1000 32"
```

El benchmark inserta cada tupla serializada en páginas de `RecordManager`,
asigna su RowID y registra
la pareja clave–RowID mediante `BTree::insert`. Luego compara llamadas reales a
`BTree::search` contra el recorrido de los registros devueltos por
`RecordManager::scan`. El reporte muestra altura, tiempo de carga, tiempo de
búsqueda, resultados y accesos a páginas lógicas. También compara el número
de resultados y un checksum de los RowIDs; la ejecución falla si ambos métodos
discrepan.

## Carga masiva integrada

`loadIndexedTuples` conecta los módulos de la fase en este orden:

1. Extrae una clave `int64_t` de la columna indexada.
2. Rechaza claves duplicadas antes de crear un registro.
3. Almacena la tupla y obtiene el RowID asignado por `RecordManager`.
4. Inserta la clave y el RowID en el B-Tree.
5. Ejecuta `BTree::validate` al final y, opcionalmente, cada N intentos.

El ejecutable usa `t=64` por defecto. Las pruebas también cubren `t=2`, `t=3`,
`t=8` y `t=32`, comprobando que todos los nodos no raíz tengan entre `t-1` y
`2t-1` claves y que todas las hojas terminen a la misma profundidad.

## Diseño

- Las páginas contienen 128 slots por defecto.
- Cada slot almacena la representación binaria producida por `TupleSerializer`.
- Un RowID de 64 bits codifica `PageID` en los 32 bits altos y `SlotID + 1` en
  los 32 bits bajos.
- Cada nodo del B-Tree se contabiliza como una página lógica del índice.
- El grado predeterminado `t=64` permite hasta 127 claves y 128 hijos por nodo,
  reduciendo la altura para cargas grandes.

## Limpieza

```bash
make clean
```
