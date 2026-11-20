#include "monocalc/type.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>

static int failures = 0;

static void check(const char* name, bool cond) {
    if (cond) {
        printf("  PASS: %s\n", name);
    } else {
        printf("  FAIL: %s\n", name);
        failures++;
    }
}

static void checkApprox(const char* name, double got, double expected, double tol) {
    if (std::abs(got - expected) < tol) {
        printf("  PASS: %s (got %.6f)\n", name, got);
    } else {
        printf("  FAIL: %s (expected %.6f, got %.6f)\n", name, expected, got);
        failures++;
    }
}

int main() {
    printf("=== Compilation & Basic Parse Test ===\n");

    // Test 1: Parse and compute (x^2+5)/(x+2) at x=1 -> 2
    {
        monocalc::Function f;
        auto pr = monocalc::parse("(x^2+5)/(x+2)", 13, f);
        check("parse (x^2+5)/(x+2)", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, 1.0);
            check("compute ok", r.ok);
            checkApprox("compute at x=1", r.value, 2.0, 1e-9);
        }
    }

    // Test 2: -x^2 at x=3 -> -9
    {
        monocalc::Function f;
        auto pr = monocalc::parse("-x^2", 4, f);
        check("parse -x^2", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, 3.0);
            check("compute ok", r.ok);
            checkApprox("compute -x^2 at x=3", r.value, -9.0, 1e-9);
        }
    }

    // Test 3: (-x)^2 at x=3 -> 9
    {
        monocalc::Function f;
        auto pr = monocalc::parse("(-x)^2", 6, f);
        check("parse (-x)^2", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, 3.0);
            check("compute ok", r.ok);
            checkApprox("compute (-x)^2 at x=3", r.value, 9.0, 1e-9);
        }
    }

    // Test 4: x^2^3 at x=2 -> 256
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2^3", 5, f);
        check("parse x^2^3", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, 2.0);
            check("compute ok", r.ok);
            checkApprox("compute x^2^3 at x=2", r.value, 256.0, 1e-9);
        }
    }

    // Test 5: x^-2 at x=2 -> 0.25
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^-2", 4, f);
        check("parse x^-2", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, 2.0);
            check("compute ok", r.ok);
            checkApprox("compute x^-2 at x=2", r.value, 0.25, 1e-9);
        }
    }

    // Test 6: ln(abs(x)) at x=-1 -> 0
    {
        monocalc::Function f;
        auto pr = monocalc::parse("ln(abs(x))", 10, f);
        check("parse ln(abs(x))", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, -1.0);
            check("compute ok", r.ok);
            checkApprox("compute ln(abs(x)) at x=-1", r.value, 0.0, 1e-9);
        }
    }

    // Test 7: ln(abs(x)) at x=0 -> domain error
    {
        monocalc::Function f;
        auto pr = monocalc::parse("ln(abs(x))", 10, f);
        if (pr.ok) {
            auto r = monocalc::compute(f, 0.0);
            check("ln(abs(0)) domain error", !r.ok && r.err == monocalc::ErrorCode::DomainError);
        }
    }

    // Test 8: 3*x^2+2 at x=2 -> 14
    {
        monocalc::Function f;
        auto pr = monocalc::parse(" 3 * x ^ 2 + 2 ", 15, f);
        check("parse with spaces", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, 2.0);
            check("compute ok", r.ok);
            checkApprox("compute 3*x^2+2 at x=2", r.value, 14.0, 1e-9);
        }
    }

    // Test 9: 1e-3*x at x=1000 -> ~1
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1e-3*x", 6, f);
        check("parse 1e-3*x", pr.ok);
        if (pr.ok) {
            auto r = monocalc::compute(f, 1000.0);
            check("compute ok", r.ok);
            checkApprox("compute 1e-3*x at x=1000", r.value, 1.0, 1e-9);
        }
    }

    // Test 10: x+ -> syntax error
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x+", 2, f);
        check("x+ syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 11: sin(x -> syntax error
    {
        monocalc::Function f;
        auto pr = monocalc::parse("sin(x", 5, f);
        check("sin(x syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 12: 2x -> syntax error (implicit multiplication)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("2x", 2, f);
        check("2x syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 13: sin(x) trailing garbage -> syntax error
    {
        monocalc::Function f;
        auto pr = monocalc::parse("sin(x) basura", 13, f);
        check("sin(x) basura syntax error", !pr.ok);
    }

    // Test 14: Repeated evaluations
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2", 3, f);
        if (pr.ok) {
            bool allOk = true;
            for (int i = 0; i < 100; i++) {
                auto r = monocalc::compute(f, (double)i);
                if (!r.ok || std::abs(r.value - (double)(i*i)) > 1e-9) {
                    allOk = false;
                    break;
                }
            }
            check("100 repeated evaluations", allOk);
        }
    }

    // === Derivative Tests ===
    printf("\n=== Derivative Tests ===\n");

    // Test 15: d/dx(x^2) = 2*x, evaluate at x=3 -> 6
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2", 3, f);
        if (pr.ok) {
            monocalc::Function df;
            auto err = monocalc::derive(f, df);
            check("derive x^2 ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(df, 3.0);
                check("d(x^2) eval ok", r.ok);
                checkApprox("d(x^2) at x=3", r.value, 6.0, 1e-9);
            }
        }
    }

    // Test 16: d/dx(3*x^2+2) = 6*x, evaluate at x=2 -> 12
    {
        monocalc::Function f;
        auto pr = monocalc::parse("3*x^2+2", 7, f);
        if (pr.ok) {
            monocalc::Function df;
            auto err = monocalc::derive(f, df);
            check("derive 3*x^2+2 ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(df, 2.0);
                check("d(3*x^2+2) eval ok", r.ok);
                checkApprox("d(3*x^2+2) at x=2", r.value, 12.0, 1e-9);
            }
        }
    }

    // Test 17: d/dx(sin(x)) = cos(x), evaluate at x=0 -> 1
    {
        monocalc::Function f;
        auto pr = monocalc::parse("sin(x)", 6, f);
        if (pr.ok) {
            monocalc::Function df;
            auto err = monocalc::derive(f, df);
            check("derive sin(x) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(df, 0.0);
                check("d(sin(x)) eval ok", r.ok);
                checkApprox("d(sin(x)) at x=0", r.value, 1.0, 1e-9);
            }
        }
    }

    // Test 18: d/dx(exp(x)) = exp(x), evaluate at x=0 -> 1
    {
        monocalc::Function f;
        auto pr = monocalc::parse("exp(x)", 6, f);
        if (pr.ok) {
            monocalc::Function df;
            auto err = monocalc::derive(f, df);
            check("derive exp(x) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(df, 0.0);
                check("d(exp(x)) eval ok", r.ok);
                checkApprox("d(exp(x)) at x=0", r.value, 1.0, 1e-9);
            }
        }
    }

    // Test 19: d/dx(exp(x^2)) at x=1 -> 2*exp(1)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("exp(x^2)", 9, f);
        if (pr.ok) {
            monocalc::Function df;
            auto err = monocalc::derive(f, df);
            check("derive exp(x^2) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(df, 1.0);
                check("d(exp(x^2)) eval ok", r.ok);
                checkApprox("d(exp(x^2)) at x=1", r.value, 2.0 * std::exp(1.0), 1e-6);
            }
        }
    }

    // Test 20: Original function preserved after derive
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2", 3, f);
        if (pr.ok) {
            monocalc::Function df;
            monocalc::derive(f, df);
            // f should still work
            auto r = monocalc::compute(f, 5.0);
            check("original preserved after derive", r.ok && std::abs(r.value - 25.0) < 1e-9);
        }
    }

    // === Integration Tests ===
    printf("\n=== Integration Tests ===\n");

    // Test 21: ∫5 dx = 5*x, evaluate at x=3 -> 15
    {
        monocalc::Function f;
        auto pr = monocalc::parse("5", 1, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate 5 ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 3.0);
                check("int(5) eval ok", r.ok);
                checkApprox("int(5) at x=3", r.value, 15.0, 1e-9);
            }
        }
    }

    // Test 22: ∫x^2 dx = x^3/3, evaluate at x=3 -> 9
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2", 3, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate x^2 ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 3.0);
                check("int(x^2) eval ok", r.ok);
                checkApprox("int(x^2) at x=3", r.value, 9.0, 1e-9);
            }
        }
    }

    // Test 23: ∫sin(x) dx = -cos(x), evaluate at x=0 -> -1
    {
        monocalc::Function f;
        auto pr = monocalc::parse("sin(x)", 6, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate sin(x) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 0.0);
                check("int(sin(x)) eval ok", r.ok);
                checkApprox("int(sin(x)) at x=0", r.value, -1.0, 1e-9);
            }
        }
    }

    // Test 24: ∫cos(x) dx = sin(x), evaluate at x=0 -> 0
    {
        monocalc::Function f;
        auto pr = monocalc::parse("cos(x)", 6, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate cos(x) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 0.0);
                check("int(cos(x)) eval ok", r.ok);
                checkApprox("int(cos(x)) at x=0", r.value, 0.0, 1e-9);
            }
        }
    }

    // Test 25: ∫exp(x) dx = exp(x), evaluate at x=0 -> 1
    {
        monocalc::Function f;
        auto pr = monocalc::parse("exp(x)", 6, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate exp(x) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 0.0);
                check("int(exp(x)) eval ok", r.ok);
                checkApprox("int(exp(x)) at x=0", r.value, 1.0, 1e-9);
            }
        }
    }

    // Test 26: ∫1/x dx = ln|a|x|, domain should be x != 0
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/x", 3, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate 1/x ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                // Check domain: x != 0
                check("int(1/x) domain NotZero", intF.domain.type == monocalc::DomainType::NotZero);
                // Evaluate at x=2: ln(abs(2)) ≈ 0.693
                auto r = monocalc::compute(intF, 2.0);
                check("int(1/x) eval ok", r.ok);
                checkApprox("int(1/x) at x=2", r.value, std::log(2.0), 1e-9);
            }
        }
    }

    // Test 27: ∫sin(3*x+2) dx = -cos(3*x+2)/3, evaluate at x=0
    // -cos(2)/3 ≈ -0.30117
    {
        monocalc::Function f;
        auto pr = monocalc::parse("sin(3*x+2)", 10, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate sin(3*x+2) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 0.0);
                check("int(sin(3*x+2)) eval ok", r.ok);
                double expected = -std::cos(2.0) / 3.0;
                checkApprox("int(sin(3*x+2)) at x=0", r.value, expected, 1e-9);
            }
        }
    }

    // Test 28: ∫exp(x^2) -> UnsupportedOperation
    {
        monocalc::Function f;
        auto pr = monocalc::parse("exp(x^2)", 9, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate exp(x^2) unsupported", err == monocalc::ErrorCode::UnsupportedOperation);
        }
    }

    // Test 29: ∫1/(2*x+3) dx = ln|2*x+3|/2
    // Evaluate at x=0: ln(3)/2 ≈ 0.5493
    // Derivative should be equivalent to 1/(2*x+3)
    // Domain: x != -1.5
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/(2*x+3)", 9, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate 1/(2*x+3) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                // Evaluate at x=0: ln(3)/2
                auto r = monocalc::compute(intF, 0.0);
                check("int(1/(2*x+3)) eval ok", r.ok);
                double expected = std::log(3.0) / 2.0;
                checkApprox("int(1/(2*x+3)) at x=0", r.value, expected, 1e-9);
                
                // Evaluate at x=1: ln(5)/2
                auto r2 = monocalc::compute(intF, 1.0);
                check("int(1/(2*x+3)) eval at x=1 ok", r2.ok);
                double expected2 = std::log(5.0) / 2.0;
                checkApprox("int(1/(2*x+3)) at x=1", r2.value, expected2, 1e-9);
                
                // Derivative of integral should equal original
                monocalc::Function dIntF;
                auto dErr = monocalc::derive(intF, dIntF);
                check("derive(int(1/(2*x+3))) ok", dErr == monocalc::ErrorCode::OK);
                if (dErr == monocalc::ErrorCode::OK) {
                    // d/dx(ln|2x+3|/2) at x=0 should equal 1/(2*0+3) = 1/3
                    auto dr = monocalc::compute(dIntF, 0.0);
                    check("d(int(1/(2*x+3))) eval ok", dr.ok);
                    checkApprox("d(int(1/(2*x+3))) at x=0", dr.value, 1.0/3.0, 1e-9);
                    
                    // at x=1 should equal 1/(2*1+3) = 1/5
                    auto dr2 = monocalc::compute(dIntF, 1.0);
                    check("d(int(1/(2*x+3))) eval at x=1 ok", dr2.ok);
                    checkApprox("d(int(1/(2*x+3))) at x=1", dr2.value, 1.0/5.0, 1e-9);
                }
                
                // Domain check: x = -1.5 should fail
                auto rDom = monocalc::compute(intF, -1.5);
                check("int(1/(2*x+3)) at x=-1.5 domain error", !rDom.ok && rDom.err == monocalc::ErrorCode::DomainError);
            }
        }
    }

    // Test 30: ∫(x^2+5)/(x+2) -> UnsupportedOperation (rational function not in catalog)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("(x^2+5)/(x+2)", 13, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate (x^2+5)/(x+2) unsupported", err == monocalc::ErrorCode::UnsupportedOperation);
        }
    }

    // Test 31: ∫3*x^2+2 dx = x^3+2*x, evaluate at x=2 -> 8+4=12
    {
        monocalc::Function f;
        auto pr = monocalc::parse("3*x^2+2", 7, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate 3*x^2+2 ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 2.0);
                check("int(3*x^2+2) eval ok", r.ok);
                checkApprox("int(3*x^2+2) at x=2", r.value, 12.0, 1e-9);
            }
        }
    }

    // Test 32: Original function preserved after integrate
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2", 3, f);
        if (pr.ok) {
            monocalc::Function intF;
            monocalc::integrate(f, intF);
            // f should still work
            auto r = monocalc::compute(f, 5.0);
            check("original preserved after integrate", r.ok && std::abs(r.value - 25.0) < 1e-9);
        }
    }

    // Test 33: ∫x^-2 dx = -1/x, evaluate at x=2 -> -0.5
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^-2", 4, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate x^-2 ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 2.0);
                check("int(x^-2) eval ok", r.ok);
                checkApprox("int(x^-2) at x=2", r.value, -0.5, 1e-9);
            }
        }
    }

    // Test 34: ∫exp(2*x) dx = exp(2*x)/2, evaluate at x=0 -> 0.5
    {
        monocalc::Function f;
        auto pr = monocalc::parse("exp(2*x)", 8, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate exp(2*x) ok", err == monocalc::ErrorCode::OK);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 0.0);
                check("int(exp(2*x)) eval ok", r.ok);
                checkApprox("int(exp(2*x)) at x=0", r.value, 0.5, 1e-9);
            }
        }
    }

    // Test 35: Domain check — int(1/x) at x=0 should fail
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/x", 3, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            if (err == monocalc::ErrorCode::OK) {
                auto r = monocalc::compute(intF, 0.0);
                check("int(1/x) at x=0 domain error", !r.ok && r.err == monocalc::ErrorCode::DomainError);
            }
        }
    }

    // ===================== LIMITS =====================
    printf("\n=== Limits ===\n");

    // Test 36: lim x^2 as x->3 = 9 (polynomial, continuous)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2", 3, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 3.0, monocalc::LimitDirection::Both);
            check("lim x^2 x->3 ok", lr.ok);
            checkApprox("lim x^2 x->3", lr.value, 9.0, 1e-6);
        }
    }

    // Test 37: lim sin(x)/x as x->0 = 1 (classic limit)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("sin(x)/x", 8, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
            check("lim sin(x)/x x->0 ok", lr.ok);
            checkApprox("lim sin(x)/x x->0", lr.value, 1.0, 1e-4);
        }
    }

    // Test 38: lim 1/x as x->0+ = +inf (diverges)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/x", 3, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Right);
            check("lim 1/x x->0+ diverged", !lr.ok && lr.err == monocalc::ErrorCode::Diverged);
        }
    }

    // Test 39: lim 1/x as x->0- = -inf (diverges)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/x", 3, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Left);
            check("lim 1/x x->0- diverged", !lr.ok && lr.err == monocalc::ErrorCode::Diverged);
        }
    }

    // Test 40: lim 1/x as x->0 bilateral = NotConverged (left != right)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/x", 3, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
            check("lim 1/x x->0 bilateral not converged", !lr.ok && lr.err == monocalc::ErrorCode::NotConverged);
        }
    }

    // Test 41: lim (x^2-1)/(x-1) as x->1 = 2 (removable discontinuity)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("(x^2-1)/(x-1)", 13, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 1.0, monocalc::LimitDirection::Both);
            check("lim (x^2-1)/(x-1) x->1 ok", lr.ok);
            checkApprox("lim (x^2-1)/(x-1) x->1", lr.value, 2.0, 1e-4);
        }
    }

    // Test 42: lim exp(x) as x->0 = 1
    {
        monocalc::Function f;
        auto pr = monocalc::parse("exp(x)", 6, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
            check("lim exp(x) x->0 ok", lr.ok);
            checkApprox("lim exp(x) x->0", lr.value, 1.0, 1e-6);
        }
    }

    // Test 43: lim x as x->5 = 5 (trivial)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x", 1, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 5.0, monocalc::LimitDirection::Both);
            check("lim x x->5 ok", lr.ok);
            checkApprox("lim x x->5", lr.value, 5.0, 1e-6);
        }
    }

    // Test 44: lim 1/x as x->inf approximation — use large x value
    // lim 1/x as x->1000 should be close to 0
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/x", 3, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 1000.0, monocalc::LimitDirection::Both);
            check("lim 1/x x->1000 ok", lr.ok);
            checkApprox("lim 1/x x->1000", lr.value, 0.001, 1e-6);
        }
    }

    // Test 45: lim abs(x)/x as x->0+ = 1 (right side)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("abs(x)/x", 8, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Right);
            check("lim abs(x)/x x->0+ ok", lr.ok);
            checkApprox("lim abs(x)/x x->0+", lr.value, 1.0, 1e-4);
        }
    }

    // Test 46: lim abs(x)/x as x->0- = -1 (left side)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("abs(x)/x", 8, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Left);
            check("lim abs(x)/x x->0- ok", lr.ok);
            checkApprox("lim abs(x)/x x->0-", lr.value, -1.0, 1e-4);
        }
    }

    // Test 47: lim abs(x)/x as x->0 bilateral = NotConverged (left != right)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("abs(x)/x", 8, f);
        if (pr.ok) {
            auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
            check("lim abs(x)/x x->0 bilateral not converged", !lr.ok && lr.err == monocalc::ErrorCode::NotConverged);
        }
    }

    // Test 48: lim on invalid function = SyntaxError
    {
        monocalc::Function f;
        f.root = monocalc::INVALID_ID;
        auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
        check("lim invalid function syntax error", !lr.ok && lr.err == monocalc::ErrorCode::SyntaxError);
    }

    // ===================== ERRORS =====================
    printf("\n=== Errors ===\n");

    // Test 49: Syntax error — "x+" incomplete
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x+", 2, f);
        check("parse x+ syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 50: Syntax error — "sin(x" incomplete paren
    {
        monocalc::Function f;
        auto pr = monocalc::parse("sin(x", 5, f);
        check("parse sin(x syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 51: Syntax error — "2x" implicit multiplication
    {
        monocalc::Function f;
        auto pr = monocalc::parse("2x", 2, f);
        check("parse 2x syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 52: Syntax error — empty string
    {
        monocalc::Function f;
        auto pr = monocalc::parse("", 0, f);
        check("parse empty syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 53: Syntax error — "x@y" invalid character
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x@y", 3, f);
        check("parse x@y syntax error", !pr.ok && pr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 54: UnsupportedOperation — integrate exp(x^2)
    {
        monocalc::Function f;
        auto pr = monocalc::parse("exp(x^2)", 8, f);
        if (pr.ok) {
            monocalc::Function intF;
            auto err = monocalc::integrate(f, intF);
            check("integrate exp(x^2) unsupported", err == monocalc::ErrorCode::UnsupportedOperation);
        }
    }

    // Test 55: DomainError — compute 1/x at x=0
    {
        monocalc::Function f;
        auto pr = monocalc::parse("1/x", 3, f);
        if (pr.ok) {
            auto r = monocalc::compute(f, 0.0);
            check("compute 1/x at 0 domain error", !r.ok && r.err == monocalc::ErrorCode::DomainError);
        }
    }

    // Test 56: DomainError — compute ln(x) at x=-1
    {
        monocalc::Function f;
        auto pr = monocalc::parse("ln(x)", 5, f);
        if (pr.ok) {
            auto r = monocalc::compute(f, -1.0);
            check("compute ln(x) at -1 domain error", !r.ok && r.err == monocalc::ErrorCode::DomainError);
        }
    }

    // Test 57: Different error codes are distinguishable
    {
        check("ErrorCode OK != SyntaxError",
              monocalc::ErrorCode::OK != monocalc::ErrorCode::SyntaxError);
        check("ErrorCode SyntaxError != UnsupportedOperation",
              monocalc::ErrorCode::SyntaxError != monocalc::ErrorCode::UnsupportedOperation);
        check("ErrorCode UnsupportedOperation != DomainError",
              monocalc::ErrorCode::UnsupportedOperation != monocalc::ErrorCode::DomainError);
        check("ErrorCode DomainError != ResourceExhausted",
              monocalc::ErrorCode::DomainError != monocalc::ErrorCode::ResourceExhausted);
        check("ErrorCode ResourceExhausted != NotConverged",
              monocalc::ErrorCode::ResourceExhausted != monocalc::ErrorCode::NotConverged);
        check("ErrorCode NotConverged != Diverged",
              monocalc::ErrorCode::NotConverged != monocalc::ErrorCode::Diverged);
    }

    // Test 58: errorMessage returns non-null for all codes
    {
        check("errorMessage OK", monocalc::errorMessage(monocalc::ErrorCode::OK) != nullptr);
        check("errorMessage SyntaxError", monocalc::errorMessage(monocalc::ErrorCode::SyntaxError) != nullptr);
        check("errorMessage UnsupportedOperation", monocalc::errorMessage(monocalc::ErrorCode::UnsupportedOperation) != nullptr);
        check("errorMessage DomainError", monocalc::errorMessage(monocalc::ErrorCode::DomainError) != nullptr);
        check("errorMessage ResourceExhausted", monocalc::errorMessage(monocalc::ErrorCode::ResourceExhausted) != nullptr);
        check("errorMessage NotConverged", monocalc::errorMessage(monocalc::ErrorCode::NotConverged) != nullptr);
        check("errorMessage Diverged", monocalc::errorMessage(monocalc::ErrorCode::Diverged) != nullptr);
    }

    // ===================== MEMORY =====================
    printf("\n=== Memory ===\n");

    // Test 59: Arena alloc returns INVALID_ID when full
    {
        monocalc::Node* nodes = static_cast<monocalc::Node*>(std::malloc(sizeof(monocalc::Node) * 3));
        monocalc::Arena arena;
        arena.init(nodes, 0, 3);
        monocalc::NodeId id1 = arena.alloc();
        monocalc::NodeId id2 = arena.alloc();
        monocalc::NodeId id3 = arena.alloc();
        monocalc::NodeId id4 = arena.alloc(); // should fail
        check("arena alloc 1 ok", id1 == 0);
        check("arena alloc 2 ok", id2 == 1);
        check("arena alloc 3 ok", id3 == 2);
        check("arena alloc 4 INVALID", id4 == monocalc::INVALID_ID);
        std::free(nodes);
    }

    // Test 60: ResourceExhausted on deep nesting
    // The parser has MAX_DEPTH=64; a very deep expression should fail
    {
        // Build a deeply nested expression: (((...x...)))
        // We'll create a string with many nested parens
        char deep[512];
        int pos = 0;
        for (int i = 0; i < 100 && pos < 500; i++) {
            deep[pos++] = '(';
        }
        deep[pos++] = 'x';
        for (int i = 0; i < 100 && pos < 500; i++) {
            deep[pos++] = ')';
        }
        deep[pos] = '\0';

        monocalc::Function f;
        auto pr = monocalc::parse(deep, pos, f);
        // Should fail with ResourceExhausted due to MAX_DEPTH
        check("deep nesting resource exhausted", !pr.ok && pr.err == monocalc::ErrorCode::ResourceExhausted);
    }

    // Test 61: derive() preserves original function on failure
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x^2", 3, f);
        if (pr.ok) {
            double origVal;
            {
                auto r = monocalc::compute(f, 5.0);
                origVal = r.value;
            }
            // Try to derive abs(x) which fails — but f is x^2, so derive should succeed
            // Instead, test that derive doesn't corrupt f
            monocalc::Function dF;
            auto err = monocalc::derive(f, dF);
            check("derive x^2 ok", err == monocalc::ErrorCode::OK);
            // f should still work
            auto r = monocalc::compute(f, 5.0);
            check("original preserved after derive", r.ok && std::abs(r.value - origVal) < 1e-9);
        }
    }

    // Test 62: derive() on invalid function returns error
    {
        monocalc::Function f;
        f.root = monocalc::INVALID_ID;
        monocalc::Function dF;
        auto err = monocalc::derive(f, dF);
        check("derive invalid function syntax error", err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 63: integrate() on invalid function returns error
    {
        monocalc::Function f;
        f.root = monocalc::INVALID_ID;
        monocalc::Function intF;
        auto err = monocalc::integrate(f, intF);
        check("integrate invalid function syntax error", err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 64: compute() on invalid function returns error
    {
        monocalc::Function f;
        f.root = monocalc::INVALID_ID;
        auto r = monocalc::compute(f, 1.0);
        check("compute invalid function syntax error", !r.ok && r.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 65: lim() on invalid function returns error
    {
        monocalc::Function f;
        f.root = monocalc::INVALID_ID;
        auto lr = monocalc::lim(f, 0.0, monocalc::LimitDirection::Both);
        check("lim invalid function syntax error", !lr.ok && lr.err == monocalc::ErrorCode::SyntaxError);
    }

    // Test 66: No silent fallback — invalid parse doesn't produce a valid function
    {
        monocalc::Function f;
        auto pr = monocalc::parse("x+y", 3, f); // 'y' is not recognized
        check("parse x+y fails", !pr.ok);
        // f.root should be INVALID_ID
        check("failed parse root is invalid", f.root == monocalc::INVALID_ID);
    }

    printf("\n=== Results: %d failures ===\n", failures);
    return failures > 0 ? 1 : 0;
}
