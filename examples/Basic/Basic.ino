/*
 * monocalcType v0.1.0 — Basic Example for Arduino AVR
 *
 * Demonstrates: parse, compute, derive, integrate, and lim.
 * Uses Serial.print for output.
 */

#include "type.h"

void printResult(const char* label, monocalc::EvalResult r) {
  Serial.print(label);
  if (r.ok) {
    Serial.print(" = ");
    Serial.println(r.value, 6);
  } else {
    Serial.print(" error: ");
    Serial.println((int)r.err);
  }
}

void printError(const char* label, monocalc::ErrorCode err) {
  Serial.print(label);
  if (err == monocalc::ErrorCode::OK) {
    Serial.println(" OK");
  } else {
    Serial.print(" error: ");
    Serial.println((int)err);
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // wait for Serial to be available (required for native USB boards)
  }

  Serial.println(F("=== monocalcType Basic Example ==="));

  // 1. Parse and compute (x^2+5)/(x+2) at x=1 -> 2
  {
    monocalc::Function f;
    auto pr = monocalc::parse("(x^2+5)/(x+2)", 13, f);
    if (pr.ok) {
      Serial.println(F("Parsed: (x^2+5)/(x+2)"));
      printResult(F("  at x=1"), monocalc::compute(f, 1.0));
      printResult(F("  at x=3"), monocalc::compute(f, 3.0));
    }
  }

  // 2. Derivative of x^2 -> 2*x
  {
    monocalc::Function f;
    auto pr = monocalc::parse("x^2", 3, f);
    if (pr.ok) {
      monocalc::Function df;
      printError(F("derive(x^2)"), monocalc::derive(f, df));
      if (df.root != monocalc::INVALID_ID) {
        printResult(F("  d/dx at x=3"), monocalc::compute(df, 3.0));
      }
    }
  }

  // 3. Integral of sin(x) -> -cos(x)
  {
    monocalc::Function f;
    auto pr = monocalc::parse("sin(x)", 6, f);
    if (pr.ok) {
      monocalc::Function intF;
      printError(F("integrate(sin(x))"), monocalc::integrate(f, intF));
      if (intF.root != monocalc::INVALID_ID) {
        printResult(F("  integral at x=0"), monocalc::compute(intF, 0.0));
      }
    }
  }

  // 4. Limit of sin(x)/x as x->0 -> 1
  {
    monocalc::Function f;
    auto pr = monocalc::parse("sin(x)/x", 8, f);
    if (pr.ok) {
      auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
      Serial.print(F("lim sin(x)/x x->0"));
      if (lr.ok) {
        Serial.print(" = ");
        Serial.println(lr.value, 6);
      } else {
        Serial.print(" error: ");
        Serial.println((int)lr.err);
      }
    }
  }

  // 5. Error handling: 1/x at x=0
  {
    monocalc::Function f;
    auto pr = monocalc::parse("1/x", 3, f);
    if (pr.ok) {
      printResult(F("1/x at x=0"), monocalc::compute(f, 0.0));
    }
  }

  // 6. Syntax error: 2x (implicit multiplication)
  {
    monocalc::Function f;
    auto pr = monocalc::parse("2x", 2, f);
    Serial.print(F("parse 2x"));
    if (!pr.ok) {
      Serial.print(" -> SyntaxError (code ");
      Serial.print((int)pr.err);
      Serial.println(F(")"));
    }
  }

  Serial.println(F("=== Done ==="));
}

void loop() {
  // nothing to repeat
}
