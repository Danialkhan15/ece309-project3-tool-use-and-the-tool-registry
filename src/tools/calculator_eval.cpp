// Evaluate tokens using a value stack and an operator stack.

#include "tools/calculator.h"

namespace {

using Kind = CalcToken::Kind;

int precedence(Kind op) {
    if (op == Kind::Times || op == Kind::Divide) {
        return 2;
    }
    if (op == Kind::Plus || op == Kind::Minus) {
        return 1;
    }
    return 0;
}

bool apply_operator(Kind op, std::vector<double>* values, std::string* error) {
    if (values->size() < 2) {
        *error = "missing operand";
        return false;
    }
    const double right = values->back();
    const double left = (*values)[values->size() - 2];
    double result = 0;
    switch (op) {
        case Kind::Plus:
            result = left + right;
            break;
        case Kind::Minus:
            result = left - right;
            break;
        case Kind::Times:
            result = left * right;
            break;
        case Kind::Divide:
            if (right == 0.0) {
                *error = "division by zero";
                return false;
            }
            result = left / right;
            break;
        default:
            *error = "invalid operator";
            return false;
    }
    values->pop_back();
    values->back() = result;
    return true;
}

}  // namespace

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out,
                   std::string* error) {
    std::vector<double> values;
    std::vector<Kind> ops;
    bool expect_operand = true;
    error->clear();

    for (const auto& token : tokens) {
        if (token.kind == Kind::Number) {
            if (!expect_operand) {
                *error = "missing operator";
                return false;
            }
            values.push_back(token.value);
            expect_operand = false;
        } else if (token.kind == Kind::LParen) {
            if (!expect_operand) {
                *error = "missing operator before '('";
                return false;
            }
            ops.push_back(token.kind);
        } else if (token.kind == Kind::RParen) {
            if (expect_operand) {
                *error = "missing operand before ')'";
                return false;
            }
            while (!ops.empty() && ops.back() != Kind::LParen) {
                if (!apply_operator(ops.back(), &values, error)) {
                    return false;
                }
                ops.pop_back();
            }
            if (ops.empty()) {
                *error = "unmatched ')'";
                return false;
            }
            ops.pop_back();
        } else {
            if (precedence(token.kind) == 0 || expect_operand) {
                *error = "invalid or misplaced operator";
                return false;
            }
            // Apply equal precedence first so subtraction and division run left to right.
            while (!ops.empty() && ops.back() != Kind::LParen &&
                   precedence(ops.back()) >= precedence(token.kind)) {
                if (!apply_operator(ops.back(), &values, error)) {
                    return false;
                }
                ops.pop_back();
            }
            ops.push_back(token.kind);
            expect_operand = true;
        }
    }
    if (expect_operand) {
        *error = "empty expression or missing operand";
        return false;
    }
    while (!ops.empty()) {
        if (ops.back() == Kind::LParen) {
            *error = "missing ')'";
            return false;
        }
        if (!apply_operator(ops.back(), &values, error)) {
            return false;
        }
        ops.pop_back();
    }
    if (values.size() != 1) {
        *error = "expected one result";
        return false;
    }
    *out = values.back();
    return true;
}
