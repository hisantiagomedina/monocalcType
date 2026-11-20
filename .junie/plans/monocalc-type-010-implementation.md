---
sessionId: session-261120-010133-1tbb
---

# Requirements

### Overview & Goals
Implementar monocalcType v0.1.0: una biblioteca Arduino C++ autónoma que interpreta expresiones matemáticas textuales, las convierte en funciones reutilizables y permite `compute`, `derive`/`d`, `integrate`/`i` y `lim`.

**Arquitectura seleccionada:** Parser Pratt + AST compacto + reglas simbólicas conservadoras + gestión explícita de memoria y dominios.

### Scope

**In Scope:**
- Lexer que reconoce tokens: variable `x`, constantes numéricas (incl. notación científica), operadores `+`, `-`, `*`, `/`, `^`, paréntesis, funciones `sin`, `cos`, `exp`, `ln`, `abs`.
- Parser Pratt con precedencia correcta: paréntesis/funciones > potencia `^` (asociativo a la derecha) > unarios > `*`/`/` > `+`/`-`.
- AST compacto con arena de memoria controlada.
- Evaluador numérico: `compute(f, x)`.
- Derivación simbólica: `derive(f)` con reglas de constante, potencia, linealidad, producto, cociente y cadena.
- Integración simbólica: `integrate(f)` para constantes, polinomios, `sin`, `cos`, `exp`, `1/x` y composiciones afines.
- Gestión de dominios: preservar restricciones como `x != 0`.
- Estimación de límites: `lim(f, x0)` numérica.
- Errores distinguibles: sintaxis inválida, operación no soportada, dominio inválido, falta de recursos.
- Compilación nativa con GCC y Clang (C++11, Ninja, CMake).
- Pruebas con CTest (sin frameworks externos).
- Compilación para Arduino AVR (Uno).

**Out of Scope:**
- Multiplicación implícita (`2x` rechazado).
- Integración por partes, sustitución no lineal general, fracciones parciales generales.
- Integración de `ln` ni potencias no enteras.
- Bytecode o RPN como representación principal.
- GitHub Actions.
- Hardware real (ESP32, ARM).

### User Stories
1. Como usuario, quiero escribir `(x^2+5)/(x+2)` y evaluarla en `x=1` para obtener `2`.
2. Como usuario, quiero derivar `x^2*sin(x)` y obtener una función reutilizable.
3. Como usuario, quiero integrar `1/x` y obtener `ln(abs(x))` con dominio correcto.
4. Como usuario, quiero que `exp(x^2)` se acepte para evaluar pero se rechace para integrar.
5. Como usuario, quiero que `-x^2` en `x=3` dé `-9` (no `9`).

### Functional Requirements
- **TXT-001 a TXT-009:** Entrada textual, subconjunto sintáctico, multiplicación explícita, precedencia, potencias, validación, diagnósticos, restricciones de recursos, ciclo de vida.
- **MAT-T-001 a MAT-T-008:** Evaluación, derivación, integración, logaritmo recíproco, integrales definidas, límites, dominios, equivalencia.
- **TY-CA-001 a TY-CA-037:** 37 casos de aceptación específicos.

### Non-Functional Requirements
- C++11 estricto, sin extensiones no estándar.
- Portable: GCC, Clang, Arduino AVR.
- Sin dependencias externas.
- Memoria controlada: arena de nodos, límites configurables.
- Errores sin corrupción de memoria ni fallback silencioso.

# Technical Design

### Current Implementation
Repositorio vacío con `specifications.md` y `monocalcType.workspace`. Un commit inicial (9198cef). Sin código fuente, sin CMake, sin pruebas.

### Key Decisions

**1. Estructura include/src clásica.** Cabeceras en `include/monocalc/`, fuentes en `src/`, pruebas en `tests/`, ejemplos en `examples/`. Simple, portable, compatible con Arduino.

**2. Arena de memoria para AST.** Nodos compactos almacenados en un búfer contiguo, referenciados por índice (no punteros). Esto evita fragmentación en AVR y permite copiar funciones sin punteros colgantes.

**3. Error codes en lugar de excepciones.** C++11 en Arduino no garantiza excepciones. Se usarán códigos de error y resultados opcionales (`struct Result { bool ok; T value; ErrorCode err; }`).

**4. `double` como tipo numérico en PC; `float` configurable para AVR.** Un macro `MONOCALC_REAL` permite cambiar el tipo sin recompilar toda la lógica.

**5. Parser Pratt con binding powers.** Precedencia implementada mediante potencias de enlace: `^` es asociativo a la derecha, unarios tienen mayor binding que `^` en posición de operador pero menor en operandos, lo que garantiza `-x^2 = -(x^2)`.

**6. Simplificación conservadora.** Solo normalización polinómica básica (coeficientes cero eliminados, `1*x` → `x`). Sin cancelación racional general ni identidades trigonométricas.

### Proposed Changes

#### File Structure
```
monocalcType/
├── CMakeLists.txt              # Configuración principal: C++11, warnings, sanitizers opcionales
├── include/
│   └── monocalc/
│       ├── type.h              # API pública unificada
│       ├── token.h             # Enum Token, struct Token con posición
│       ├── lexer.h             # Lexer: tokenize(input) → tokens[]
│       ├── parser.h            # Parser Pratt: parse(tokens[]) → AST root
│       ├── ast.h               # NodeType enum, Node struct, Arena, Function
│       ├── evaluator.h         # evaluate(node, x) → double
│       ├── differentiator.h    # differentiate(node) → nuevo AST
│       ├── integrator.h        # integrate(node) → nuevo AST + dominio
│       ├── domain.h            # DomainConstraint, validar dominios
│       ├── limiter.h           # limit(f, x0, direction) → resultado
│       └── error.h             # ErrorCode enum, ErrorMessage
├── src/
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── ast.cpp
│   ├── evaluator.cpp
│   ├── differentiator.cpp
│   ├── integrator.cpp
│   ├── domain.cpp
│   └── limiter.cpp
├── tests/
│   ├── CMakeLists.txt          # add_executable + add_test por prueba
│   ├── test_lexer.cpp          # TY-CA-015 a TY-CA-018, TY-CA-019
│   ├── test_parser.cpp         # TY-CA-002 a TY-CA-006, TY-CA-026, TY-CA-027
│   ├── test_evaluator.cpp      # TY-CA-001, TY-CA-003 a TY-CA-008, TY-CA-021, TY-CA-028
│   ├── test_differentiator.cpp # TY-CA-012, TY-CA-029, TY-CA-034
│   ├── test_integrator.cpp     # TY-CA-009 a TY-CA-014, TY-CA-030, TY-CA-035
│   ├── test_domain.cpp         # TY-CA-013, TY-CA-031, TY-CA-032
│   ├── test_limiter.cpp        # TY-CA-023
│   ├── test_error.cpp          # TY-CA-015 a TY-CA-018, TY-CA-020
│   └── test_memory.cpp         # TY-CA-020, TY-CA-033, TY-CA-037
├── examples/
│   └── Basic/
│       ├── Basic.ino           # Sketch Arduino: parse, compute, derive, integrate
│       └── library.properties  # Metadata para Arduino CLI
├── .gitignore
└── specifications.md
```

### Architecture Diagram

``` mermaid
graph LR
    T[Texto de entrada] --> L[Lexer]
    L --> TK[Tokens[]]
    TK --> P[Parser Pratt]
    P --> A[AST + Arena]
    A --> E[Evaluator: compute]
    A --> D[Differentiator: derive]
    A --> I[Integrator: integrate]
    A --> LM[Limiter: lim]
    D --> A2[Nuevo AST]
    I --> A3[Nuevo AST]
    A2 --> E
    A3 --> E
    E --> R[Resultado]
    D --> R
    I --> R
    LM --> R
    A --> DOM[Domain Validator]
    DOM --> R
```

### Data Models / Contracts

**API pública (`type.h`):**
```cpp
namespace monocalc {

// Resultado con error opcional
struct Result {
    bool ok;
    double value;
    ErrorCode err;
    size_t pos; // posición del error
};

// Función parseada, reutilizable
struct Function {
    Arena arena;        // memoria de nodos
    NodeId root;        // nodo raíz del AST
    Domain domain;      // restricciones de dominio
};

// API principal
ErrorCode parse(const char* input, size_t len, Function& out);
Result compute(const Function& f, double x);
ErrorCode derive(const Function& f, Function& out);
ErrorCode integrate(const Function& f, Function& out);
Result limit(const Function& f, double x0, LimitDirection dir);

} // namespace monocalc
```

**AST (`ast.h`):**
```cpp
enum class NodeType { Const, Var, UnaryOp, BinaryOp, FuncCall };
enum class OpType { Add, Sub, Mul, Div, Pow, Neg, Pos };
enum class FuncType { Sin, Cos, Exp, Ln, Abs };

struct Node {
    NodeType type;
    union {
        double constant;
        OpType op;
        FuncType func;
    };
    NodeId left;   // índice en arena (INVALID_ID si no aplica)
    NodeId right;  // índice en arena (INVALID_ID si no aplica)
};

struct Arena {
    Node* nodes;
    size_t count;
    size_t capacity;
    NodeId alloc(); // devuelve índice o INVALID_ID si lleno
};
```

**Tokens (`token.h`):**
```cpp
enum class TokenType {
    EOF, Number, Ident, Plus, Minus, Star, Slash, Caret,
    LParen, RParen, Comma, Unknown
};

struct Token {
    TokenType type;
    double number;      // si es Number
    char ident[16];     // si es Ident ("x", "sin", etc.)
    size_t pos;         // posición en la entrada
};
```

**Error codes (`error.h`):**
```cpp
enum class ErrorCode {
    OK,
    SyntaxError,
    UnsupportedOperation,
    DomainError,
    ResourceExhausted,
    NotConverged,
    Diverged
};
```

# Testing

### Validation Approach
Pruebas unitarias en C++11 puro, sin frameworks externos. Cada archivo de prueba es un `main()` que ejecuta casos y retorna `0` si todos pasan, `1` si alguno falla. CTest registra cada prueba.

### Key Scenarios

**Lexer (test_lexer.cpp):**
- Tokenizar `(x^2+5)/(x+2)` correctamente.
- Rechazar `2x` (multiplicación implícita).
- Rechazar `sin(x` (paréntesis incompleto).
- Rechazar `sin(x) basura` (contenido sobrante).
- Aceptar `1e-3*x`.

**Parser (test_parser.cpp):**
- `-x^2` en `x=3` → `-9` (precedencia correcta).
- `(-x)^2` en `x=3` → `9`.
- `x^2^3` en `x=2` → `256` (asociatividad derecha).
- `x^-2` en `x=2` → `0.25`.
- Profundidad excesiva → error de recursos.

**Evaluator (test_evaluator.cpp):**
- `(x^2+5)/(x+2)` en `x=1` → `2`.
- `3*x^2+2` en `x=2` → `14`.
- `ln(abs(x))` en `x=-1` → `0`.
- `ln(abs(x))` en `x=0` → dominio inválido.
- 100 evaluaciones sin reparsear.

**Differentiator (test_differentiator.cpp):**
- Derivar `x^2` → `2*x`.
- Derivar `x*x*sin(x)` → válida por evaluación.
- Derivar `exp(x^2)` → válida.
- Reutilizar derivada tras destruir contexto temporal.

**Integrator (test_integrator.cpp):**
- Integrar `sin(3*x+2)` → equivalente a `-cos(3*x+2)/3`.
- Integrar `1/x` → `ln(abs(x))`, derivar resultado → `1/x`.
- Integrar `exp(x^2)` → no soportada.
- Integral definida `1/x` en `[-1,1]` → rechazada.
- Integrar `1/(2*x+3)` → `ln(abs(2*x+3))/2`.

**Domain (test_domain.cpp):**
- `(x^2-1)/(x-1)` en `x=1` → dominio inválido.
- `x/x` simplificado en `x=0` → mantener inválido.
- `0*(1/x)` en `x=0` → mantener restricción.

**Limiter (test_limiter.cpp):**
- Batería de 24 límites con resultados esperados.

**Error (test_error.cpp):**
- `x+` → error sintáctico.
- `sin(x` → error sintáctico.
- `2x` → error sintáctico.
- Entrada excesiva → error de recursos.

**Memory (test_memory.cpp):**
- Exceder memoria durante `derive()` → error, función original preservada.
- Límites de arena documentados.

### Edge Cases
- Espacios en expresiones: ` 3 * x ^ 2 + 2 `.
- Notación científica: `1e-3`, `2.5E2`.
- Signos unarios consecutivos: `--x`.
- Composiciones anidadas: `sin(exp(ln(x)))`.

### Test Commands
```bash

# Compilar y ejecutar pruebas

C:\msys64\ucrt64\bin\cmake.exe -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
C:\msys64\ucrt64\bin\cmake.exe --build build
C:\msys64\ucrt64\bin\ctest.exe --test-dir build --output-on-failure

# Compilar para Arduino

C:\msys64\home\hoover\bin\arduino-cli.exe compile --fqbn arduino:avr:uno --library . examples/Basic
```

# Delivery Steps

### ✓ Step 1: Stage 1: CMake, estructura y .gitignore
Configurar el sistema de construcción y la estructura de archivos base.
- Crear `CMakeLists.txt` raíz con C++11, warnings (`-Wall -Wextra -Wpedantic`), opción de sanitizers desactivada por defecto.
- Crear `tests/CMakeLists.txt` para registrar pruebas con CTest.
- Crear `.gitignore` para excluir `build/`, `*.xml`, artefactos de IDE.
- Crear los directorios `include/monocalc/`, `src/`, `tests/`, `examples/Basic/`.
- Crear las cabeceras vacías: `error.h`, `token.h`, `ast.h`, `lexer.h`, `parser.h`, `evaluator.h`, `differentiator.h`, `integrator.h`, `domain.h`, `limiter.h`, `type.h`.
- Verificar que `cmake -S . -B build -G Ninja` y `cmake --build build` compilan sin errores.

### ✓ Step 2: Stage 2: Lexer y Parser Pratt
Implementar el analizador léxico y el parser Pratt con precedencia correcta.
- `token.h` / `lexer.cpp`: reconocer números (con notación científica), `x`, operadores, paréntesis, funciones (`sin`, `cos`, `exp`, `ln`, `abs`). Validar literales y rechazar caracteres inválidos.
- `ast.h` / `ast.cpp`: definir `Node`, `Arena` con capacidad configurable, `NodeId` como índice.
- `parser.h` / `parser.cpp`: implementar Parser Pratt con binding powers. Precedencia: paréntesis/funciones > `^` (asociativo derecha) > unarios > `*`/`/` > `+`/`-`. Garantizar `-x^2 = -(x^2)`, `x^2^3 = x^(2^3)`, `x^-2 = x^(-2)`.
- `test_lexer.cpp`: validar tokenización y rechazo de entradas inválidas (TY-CA-015 a TY-CA-018, TY-CA-019).
- `test_parser.cpp`: validar precedencia y asociatividad (TY-CA-002 a TY-CA-006, TY-CA-026, TY-CA-027).
- Compilar y ejecutar pruebas; corregir errores.

### ✓ Step 3: Stage 3: Evaluador numérico y API compute()
Implementar evaluación del AST y la función compute().
- `evaluator.h` / `evaluator.cpp`: evaluar recursivamente el AST. Soportar constantes, variable, operadores binarios/unarios, funciones elementales (`sin`, `cos`, `exp`, `ln`, `abs`).
- `type.h` / `type.cpp`: exponer `parse()`, `compute()` y `Function` como API pública.
- `test_evaluator.cpp`: TY-CA-001, TY-CA-003 a TY-CA-008, TY-CA-021, TY-CA-028 (100 evaluaciones sin reparsear).
- Compilar y ejecutar pruebas; corregir errores.

### ✓ Step 4: Stage 4: Derivación simbólica
Implementar reglas de derivación que transforman el AST en un nuevo AST.
- `differentiator.h` / `differentiator.cpp`: reglas de constante, potencia entera, linealidad, producto, cociente, cadena. Funciones elementales: `sin` → `cos`, `cos` → `-sin`, `exp` → `exp`, `ln` → `1/x`, `abs` → regla por signo.
- `type.h`: exponer `derive()`.
- `test_differentiator.cpp`: TY-CA-012, TY-CA-029, TY-CA-034. Validar por evaluación en puntos representativos.
- Compilar y ejecutar pruebas; corregir errores.

###   Step 5: Stage 5: Integración simbólica y dominios
Implementar integración simbólica conservadora y gestión de dominios.
- `integrator.h` / `integrator.cpp`: reglas para constantes, polinomios, `sin`, `cos`, `exp`, `1/x` → `ln(abs(x))`, composiciones afines `a*x+b`. Rechazar explícitamente patrones no soportados (ej. `exp(x^2)`).
- `domain.h` / `domain.cpp`: representar restricciones (`x != 0`, `x > 0`, etc.). Validar dominios en evaluación y en integrales definidas.
- `type.h`: exponer `integrate()`.
- `test_integrator.cpp`: TY-CA-009 a TY-CA-014, TY-CA-030, TY-CA-035.
- `test_domain.cpp`: TY-CA-013, TY-CA-031, TY-CA-032.
- Compilar y ejecutar pruebas; corregir errores.

###   Step 6: Stage 6: Límites, errores y memoria
Implementar estimación de límites, categorías de error y control de memoria.
- `limiter.h` / `limiter.cpp`: estimación numérica de límites laterales y bilaterales. Distinguir convergencia, divergencia y no concluyente.
- `test_limiter.cpp`: TY-CA-023 (batería de 24 límites).
- `test_error.cpp`: TY-CA-015 a TY-CA-018, TY-CA-020 (errores distinguibles).
- `test_memory.cpp`: TY-CA-033, TY-CA-037 (exceso de memoria, preservación de función original).
- Compilar y ejecutar pruebas; corregir errores.

###   Step 7: Stage 7: Ejemplo Arduino, validación final y reporte
Preparar el sketch Arduino, validar compilación para AVR y generar informe final.
- `examples/Basic/Basic.ino`: sketch mínimo que demuestra parse, compute, derive, integrate y lim.
- `examples/Basic/library.properties`: metadata para Arduino CLI.
- Compilar para Arduino: `arduino-cli compile --fqbn arduino:avr:uno --library . examples/Basic`.
- Ejecutar batería completa con GCC: `ctest --test-dir build-gcc --output-on-failure`.
- Ejecutar batería completa con Clang: `ctest --test-dir build-clang --output-on-failure`.
- Generar informe JUnit XML y resumen Markdown con resultados reales.
- Commit final con hash registrado.