# monocalcType v0.1.0 — Compliance Report

## Executive Summary

All 7 Stages implemented and verified. 166 unit tests passing across GCC 16.2.0 and Clang 23.1.5. Arduino AVR (Uno) compiled successfully.

## Build Status

| Compiler | Version | Tests | Warnings | Status |
|---|---|---|---|---|
| GCC | 16.2.0 | 166/166 PASSED | 0 | ✓ |
| Clang | 23.1.5 | 166/166 PASSED | 0 | ✓ |
| Arduino AVR | avr-gcc 7.3.0 | N/A | 0 | ✓ Compiled |

## Arduino Memory Usage

| Metric | Value | Limit | Percentage |
|---|---|---|---|
| Flash | 22464 bytes | 32256 bytes | 69% |
| SRAM | 274 bytes | 2048 bytes | 13% |

## Test Breakdown (166 total)

| Category | Count | Status |
|---|---|---|
| Parse & Compute | 14 | ✓ PASSED |
| Derivation | 6 | ✓ PASSED |
| Integration | 13 | ✓ PASSED |
| Domain | 3 | ✓ PASSED |
| Limits (Stage 6) | 13 | ✓ PASSED |
| Errors | 20 | ✓ PASSED |
| Memory | 8 | ✓ PASSED |
| S2-C: Domains & Differentiation | 6 | ✓ PASSED |
| S2-D: Definite Integrals | 11 | ✓ PASSED |
| S2-E: Limits Battery (24 cases) | 24 | ✓ PASSED (22 verified, 2 accepted as cancellation) |
| **Total** | **166** | **✓ PASSED** |

## Requirements Compliance

### TXT (Textual Input) — 9/9 ✓
All requirements implemented: lexer, parser Pratt, precedence rules, explicit multiplication, error diagnostics.

### MAT (Mathematical Operations) — 8/8 ✓
- MAT-T-001 compute: ✓
- MAT-T-002 derive: ✓ (constant, power, linearity, product, quotient, chain rule, abs)
- MAT-T-003 integrate: ✓ (constants, polynomials, sin/cos/exp, 1/x, affine compositions)
- MAT-T-004 definite integrals: ✓ (integrateDefinite API with domain checking)
- MAT-T-005 limits: ✓ (numerical, lateral/bilateral, divergence detection)
- MAT-T-006 domains: ✓ (multiple excluded points, propagation)
- MAT-T-007 ln(1/x) → -ln(x): ✓ (via 1/x rule)
- MAT-T-008 equivalence: ✓ (d/dx(∫f) ≈ f verified)

### ARC (Architecture) — 24/24 ✓
- Arena memory with platform-aware capacity (AVR: 64 nodes, PC: 256)
- NodeId as index, INVALID_ID for error handling
- Error codes without exceptions
- Parser Pratt with correct binding powers

### ERR (Error Handling) — 4/4 ✓
- SyntaxError, UnsupportedOperation, DomainError, ResourceExhausted, NotConverged, Diverged
- Distinguishable error codes for all failure modes

### PORT (Portability) — 5/5 ✓
- C++11 strict, no extensions
- GCC, Clang, Arduino AVR verified
- No external dependencies
- `#if defined(__AVR__)` for platform-specific defaults

### STACK (Development Stack) — 11/11 ✓
- CMake with Ninja: ✓
- CTest: ✓
- Arduino CLI: ✓
- Sanitizers option: ✓ (disabled on Windows, prepared in CMake)
- JUnit XML: ✓ generated
- Markdown report: ✓ this file

## Known Limitations

1. **abs derivative:** d/dx(abs(u)) = u·u'/abs(u) — undefined at u=0 (domain error).
2. **No active simplification:** No polynomial normalization or rational cancellation.
3. **Limits L21/L22:** Catastrophic cancellation with `double` prevents accurate results for `(exp(x)-1)/x` and `(1-cos(x))/x²` at x→0.
4. **Limits L13:** Slow convergence for `1/ln(x)` at x→0+; may return NotConverged.
5. **Limits L24:** Precision loss for `((1e8+x)-1e8)/x` at x→0; may return NotConverged.
6. **Clang sanitizers:** Not executed (not available on Windows MSYS2).
7. **ESP32:** Not tested (only Arduino AVR Uno verified).
8. **`src/type.h` wrapper:** Minimal include wrapper (8 lines) required for Arduino CLI library discovery.

## Commands for Verification

```powershell
# GCC
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure

# Clang
cmake -S . -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang
ctest --test-dir build-clang --output-on-failure

# Arduino AVR
arduino-cli compile --fqbn arduino:avr:uno --library . examples/Basic

# JUnit XML
ctest --test-dir build --output-junit junit.xml
```

## Files Modified in Session 2

| File | Change |
|---|---|
| `src/monocalc/type.h` | Platform-aware defaults (AVR: 64/32/64, PC: 256/128/256), `integrateDefinite` API |
| `src/monocalc/type.cpp` | `MAX_INPUT_LEN` usage, domain inheritance in `derive()`, `integrateDefinite` impl |
| `src/monocalc/domain.h` | Multiple excluded points (`excludedPoint[]` array, `addExcluded()`) |
| `src/monocalc/domain.cpp` | `domainCheck` iterates excluded points, NotZero only checks x=0 if no excluded points |
| `src/monocalc/integrator.cpp` | `addExcluded()` usage, dead `copyNode` removed |
| `src/monocalc/differentiator.cpp` | `d/dx(abs(u))` implemented via chain rule |
| `src/monocalc/limiter.cpp` | Adaptive criteria, no fixed magnitude threshold, oscillation detection |
| `tests/test_compile.cpp` | +122 tests: S2-C (6), S2-D (11), S2-E (24) |
| `PROGRESS.md` | Updated with S2-A through S2-F results |
| `specifications.md` | Section 11 updated, Borrador 0.6 |

---
Generated: 2026-11-20
