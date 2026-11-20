# PROGRESS — monocalcType v0.1.0

## Milestones Completados

### ✓ Stage 1: CMake, estructura y .gitignore
- CMakeLists.txt con C++11, -Wall -Wextra -Wpedantic, sanitizers opcionales
- .gitignore para build/, *.xml, .codelite/
- Directorios: include/monocalc/, src/, tests/, examples/Basic/
- Cabeceras: error.h, token.h, ast.h, lexer.h, parser.h, evaluator.h, differentiator.h, integrator.h, domain.h, limiter.h, type.h

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

## Requisitos Implementados
- TY-CA-001: (x^2+5)/(x+2) en x=1 → 2 ✓
- TY-CA-002: 3*x^2+2 en x=2 → 14 ✓
- TY-CA-003: -x^2 en x=3 → -9 ✓
- TY-CA-004: (-x)^2 en x=3 → 9 ✓
- TY-CA-005: x^2^3 en x=2 → 256 ✓
- TY-CA-006: x^-2 en x=2 → 0.25 ✓
- TY-CA-007: ln(abs(x)) en x=-1 → 0 ✓
- TY-CA-008: ln(abs(x)) en x=0 → DomainError ✓
- TY-CA-015: x+ → SyntaxError ✓
- TY-CA-016: sin(x → SyntaxError ✓
- TY-CA-017: 2x → SyntaxError ✓
- TY-CA-018: sin(x) basura → SyntaxError ✓
- TY-CA-019: 1e-3*x en x=1000 → 1 ✓
- TY-CA-021: Múltiples evaluaciones ✓
- TY-CA-028: 100 evaluaciones sin reparsear ✓
- TY-CA-012: exp(x^2) evaluar y derivar ✓
- TY-CA-029: Derivación de x*x*sin(x) ✓
- TY-CA-034: Reutilizar derivada tras destruir contexto ✓

## Resultados de Pruebas
- Compilación GCC 16.2.0: ✓ exitosa
- Pruebas test_compile: 20/20 PASSED (14 parse/compute + 6 derive)
- Clang: pendiente
- Arduino AVR: pendiente

## Decisiones Técnicas
- Arena sin constructor (C++11 no soporta aggregate assignment con constructor)
- Parser con parseUnary/parseUnaryOperand para -x^2 = -(x^2)
- Potencia derecha-asociativa en parseUnaryOperand para x^2^3 = x^(2^3)
- Notación científica en lexer con verificación de dígito después de e/E
- Detección de multiplicación implícita en type.cpp después del lexer

## Próximo Milestone
**Stage 5: Integración simbólica y dominios** — Implementar integrator.cpp con reglas para constantes, polinomios, sin, cos, exp, 1/x y composiciones afines. Gestionar dominios con domain.h.
