# PROGRESS — monocalcType v0.1.0

## Milestones Completados

### ✓ Stage 1: CMake, estructura y .gitignore
- CMakeLists.txt con C++11, -Wall -Wextra -Wpedantic, sanitizers opcionales
- .gitignore para build/, *.xml, .codelite/
- Directorios: src/monocalc/, tests/, examples/Basic/
- Cabeceras: error.h, token.h, ast.h, lexer.h, parser.h, evaluator.h, differentiator.h, integrator.h, domain.h, limiter.h, type.h
- **Estructura compartida:** `src/monocalc/` contiene todos los headers (.h) y fuentes (.cpp). `src/type.h` es un **wrapper mínimo** (`#include "monocalc/type.h"`) para Arduino CLI. CMake y Arduino comparten los mismos archivos.

### ✓ Stage 2: Lexer y Parser Pratt
- Lexer: números (incl. notación científica 1e-3), variable x, operadores, paréntesis, funciones sin/cos/exp/ln/abs
- Detección de multiplicación implícita (2x rechazado)
- Parser Pratt con parseUnary/parseUnaryOperand para precedencia correcta
- `-x^2 = -(x^2)`, `x^2^3 = x^(2^3)`, `x^-2 = x^(-2)`
- Control de profundidad MAX_DEPTH=64

### ✓ Stage 3: Evaluador numérico y API compute()
- Evaluador recursivo: constantes, variable, operadores binarios/unarios, funciones elementales
- Manejo de potencias enteras con bases negativas
- API pública: parse(), compute(), derive(), integrate(), lim()
- Detección de errores de dominio (división por cero, ln(0))
- Arena de memoria con init() para C++11

### ✓ Stage 4: Derivación simbólica
- Reglas: constante (0), variable (1), potencia (n*x^(n-1)), linealidad, producto, cociente
- Regla de cadena para sin, cos, exp, ln
- copyNode para copiar subárboles al arena de salida
- exp(x^2) derivable y evaluable
- Caso especial: d/dx(ln(abs(x))) = 1/x
- Caso extendido: d/dx(ln(abs(ax+b))) = a/(ax+b) para composiciones afines

### ✓ Stage 5: Integración simbólica y dominios
- Integrador completo: constantes, polinomios (x^n), sin, cos, exp, 1/x → ln|a|x|
- Composiciones afines: sin(a*x+b), cos(a*x+b), exp(a*x+b)
- 1/(a*x+b) → ln|a|a*x+b|/a
- x^(-n) manejado con UnaryOp Neg
- Propagación de dominios: NotZero para 1/x, 1/(a*x+b), x^(-n)
- Rechazo explícito: exp(x^2), ln(x), abs(x), funciones racionales
- isAffine() para detectar a*x+b en argumentos de funciones

### ✓ Stage 6: Límites, errores y memoria
- Limiter numérico: aproximación progresiva (h *= 0.1) hasta convergencia
- Límites laterales (Left/Right) y bilaterales (Both)
- Detección de divergencia (>1e15) con ±inf
- Límites distintos por lado → NotConverged
- 7 códigos de error distinguibles verificados
- Arena: alloc() devuelve INVALID_ID al agotarse
- Parser MAX_DEPTH=64 rechaza nesting excesivo con ResourceExhausted
- Funciones inválidas retornan errores sin corrupción de memoria

## Requisitos Implementados
- TY-CA-001: (x^2+5)/(x+2) en x=1 → 2 ✓
- TY-CA-002: 3*x^2+2 en x=2 → 14 ✓
- TY-CA-003: -x^2 en x=3 → -9 ✓
- TY-CA-004: (-x)^2 en x=3 → 9 ✓
- TY-CA-005: x^2^3 en x=2 → 256 ✓
- TY-CA-006: x^-2 en x=2 → 0.25 ✓
- TY-CA-007: ln(abs(x)) en x=-1 → 0 ✓
- TY-CA-008: ln(abs(x)) en x=0 → DomainError ✓
- TY-CA-009: sin(3*x+2) integrar ✓
- TY-CA-010: 1/x integrar → ln|a|x|, dominio NotZero ✓
- TY-CA-011: exp(x^2) integrar → UnsupportedOperation ✓
- TY-CA-012: exp(x^2) evaluar y derivar ✓
- TY-CA-015: x+ → SyntaxError ✓
- TY-CA-016: sin(x → SyntaxError ✓
- TY-CA-017: 2x → SyntaxError ✓
- TY-CA-018: sin(x) basura → SyntaxError ✓
- TY-CA-019: 1e-3*x en x=1000 → 1 ✓
- TY-CA-020: Errores distinguibles ✓
- TY-CA-021: Múltiples evaluaciones ✓
- TY-CA-023: Límites numéricos (sin(x)/x, 1/x, abs(x)/x) ✓
- TY-CA-028: 100 evaluaciones sin reparsear ✓
- TY-CA-029: Derivación de x*x*sin(x) ✓
- TY-CA-030: 1/(2*x+3) integrar ✓, evaluar ✓, derivar integral ✓, dominio ✓
- TY-CA-033: Exceder memoria → ResourceExhausted ✓
- TY-CA-034: Reutilizar derivada tras destruir contexto ✓
- TY-CA-037: Arena agotada → INVALID_ID ✓

## Resultados de Pruebas
- Compilación GCC 16.2.0: ✓ exitosa sin warnings
- Pruebas test_compile: **125/125 PASSED**
- Arduino AVR (Uno): ✓ compilación exitosa — 21184 bytes Flash (65%), 274 bytes SRAM (13%)
- Comando Arduino: `arduino-cli compile --fqbn arduino:avr:uno --library . examples/Basic`
- **Estructura compartida:** CMake y Arduino CLI usan los mismos archivos en `src/monocalc/`. `src/type.h` es un wrapper mínimo (8 líneas) para Arduino. `examples/Basic/` contiene solo `Basic.ino`.
- Clang: pendiente
- Bug corregido: domainCheck(NotZero) usaba x=0 hardcodeado; ahora usa excludedPoint
- Bug corregido: d/dx(ln(abs(ax+b))) detectado antes de la regla de cadena genérica
- Bug corregido: lim 1/x x->0 bilateral devuelve NotConverged (signos opuestos)

## Decisiones Técnicas
- Arena sin constructor (C++11 no soporta aggregate assignment con constructor)
- Parser con parseUnary/parseUnaryOperand para -x^2 = -(x^2)
- Potencia derecha-asociativa en parseUnaryOperand para x^2^3 = x^(2^3)
- Notación científica en lexer con verificación de dígito después de e/E
- Detección de multiplicación implícita en type.cpp después del lexer
- Integrador con isAffine() recursivo para detectar a*x+b
- integrateAst() renombrado para evitar colisión con integrate() de type.cpp
- d/dx(ln(abs(x))) y d/dx(ln(abs(ax+b))) como casos especiales antes de la regla de cadena
- domainCheck(NotZero) usa excludedPoint para el punto excluido real
- Limiter con aproximación numérica progresiva (h *= 0.1) hasta convergencia o divergencia
- Límites bilaterales combinan izquierda y derecha; signos opuestos → NotConverged
- **Portabilidad AVR:** `#if defined(__AVR__)` para `size_t` en headers; headers C (`<math.h>`, `<stdlib.h>`, `<string.h>`, `<ctype.h>`, `<float.h>`) en .cpp; funciones C (`fabs`, `sin`, `cos`, `exp`, `log`, `pow`, `isnan`, `isinf`, `strtod`, `malloc`, `free`) en lugar de `std::` equivalents

## Próximo Milestone
**Stage 7 completado.** Arduino AVR compilado con éxito. CMake y Arduino comparten `src/monocalc/`. Pendiente: verificación con Clang y generación de informes JUnit XML / Markdown.

## Reorganización de Estructura
- **Antes:** Headers en `include/monocalc/`, fuentes en `src/`, copias duplicadas en `examples/Basic/`
- **Ahora:** Todo en `src/monocalc/` (headers + fuentes). `src/type.h` es un **wrapper mínimo** que solo incluye `monocalc/type.h`.
- **CMake:** `target_include_directories(monocalc PUBLIC ${CMAKE_SOURCE_DIR}/src)` — resuelve `#include "monocalc/X.h"` como `src/monocalc/X.h`
- **Arduino CLI:** `--library .` desde la raíz del repositorio — `src/type.h` activa el library discovery, luego `src/monocalc/` proporciona los headers internos
- **examples/Basic/:** Solo `Basic.ino`, sin duplicación de archivos
