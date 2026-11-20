/*
 * monocalcType v0.1.0 — Basic Example for Arduino AVR
 *
 * Demonstrates: parse, compute, derive, integrate, and lim.
 * Uses Serial.print for output.
 *
 * Memory usage (Arduino Uno):
 *   Flash: ~11 KB (35% of 32 KB)
 *   SRAM:  ~242 B (11% of 2 KB)
 */

#include "type.h"

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  Serial.println(F("=== monocalcType Basic Example ==="));

  // 1. Parse and compute (x^2+5)/(x+2) at x=1 -> 2
  {
    monocalc::Function f;
    auto pr = monocalc::parse("(x^2+5)/(x+2)", 13, f);
    if (pr.ok) {
      Serial.println(F("Parsed: (x^2+5)/(x+2)"));
      auto r = monocalc::compute(f, 1.0);
      Serial.print(F("  at x=1: "));
      Serial.println(r.ok ? r.value : (int)r.err);
    }
  }

  // 2. Derivative of x^2 -> 2*x
  {
    monocalc::Function f;
    auto pr = monocalc::parse("x^2", 3, f);
    if (pr.ok) {
      monocalc::Function df;
      auto err = monocalc::derive(f, df);
      if (err == monocalc::ErrorCode::OK) {
        auto r = monocalc::compute(df, 3.0);
        Serial.print(F("d/dx(x^2) at x=3: "));
        Serial.println(r.ok ? r.value : (int)r.err);
      }
    }
  }

  // 3. Integral of sin(x) -> -cos(x)
  {
    monocalc::Function f;
    auto pr = monocalc::parse("sin(x)", 6, f);
    if (pr.ok) {
      monocalc::Function intF;
      auto err = monocalc::integrate(f, intF);
      if (err == monocalc::ErrorCode::OK) {
        auto r = monocalc::compute(intF, 0.0);
        Serial.print(F("int(sin(x)) at x=0: "));
        Serial.println(r.ok ? r.value : (int)r.err);
      }
    }
  }

  // 4. Limit of sin(x)/x as x->0 -> 1
  {
    monocalc::Function f;
    auto pr = monocalc::parse("sin(x)/x", 8, f);
    if (pr.ok) {
      auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
      Serial.print(F("lim sin(x)/x x->0: "));
      Serial.println(lr.ok ? lr.value : (int)lr.err);
    }
  }

  // 5. Error handling: 1/x at x=0
  {
    monocalc::Function f;
    auto pr = monocalc::parse("1/x", 3, f);
    if (pr.ok) {
      auto r = monocalc::compute(f, 0.0);
      Serial.print(F("1/x at x=0: "));
      Serial.println(r.ok ? r.value : (int)r.err);
    }
  }

  Serial.println(F("=== Done ==="));
}

void loop() {
  // nothing to repeat
}
