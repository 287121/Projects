#include "Calculator.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>

Calculator::Calculator()
    : leftOperand(""), rightOperand(""), currentOperator('\0'), resultStr("Result: N/A"), memoryValue(0.0f) {}

void Calculator::clear() {
    leftOperand.clear();
    rightOperand.clear();
    currentOperator = '\0';
    resultStr = "Result: N/A";
}

void Calculator::backspace() {
    if (currentOperator != '\0' && !rightOperand.empty()) {
        rightOperand.pop_back();
    }
    else if (currentOperator != '\0') {
        currentOperator = '\0';
    }
    else if (!leftOperand.empty()) {
        leftOperand.pop_back();
    }
}

void Calculator::append(const char* str) {
    std::string& operand = (currentOperator == '\0') ? leftOperand : rightOperand;
    if (str[0] == '.') {
        if (operand.find('.') != std::string::npos) return;
        if (operand.empty()) operand = "0";
    }
    operand.append(str);
}

void Calculator::selectOperator(char op) {
    if (!leftOperand.empty())
        currentOperator = op;
}

void Calculator::toggleSign() {
    std::string& operand = (currentOperator == '\0') ? leftOperand : rightOperand;
    if (!operand.empty() && operand != "0") {
        if (operand[0] == '-')
            operand.erase(0, 1);
        else
            operand.insert(0, "-");
    }
}

bool Calculator::calculate() {
    if (leftOperand.empty() || currentOperator == '\0' || rightOperand.empty())
        return true;
    float a = toFloat(leftOperand);
    float b = toFloat(rightOperand);
    if ((currentOperator == '/' || currentOperator == '%') && b == 0.0f) {
        resultStr = (currentOperator == '/') ? "Error: Division by zero!" : "Error: Modulo by zero!";
        return false;
    }
    float calc = 0.0f;
    switch (currentOperator) {
    case '+': calc = a + b; break;
    case '-': calc = a - b; break;
    case '*': calc = a * b; break;
    case '/': calc = a / b; break;
    case '%': calc = std::fmod(a, b); break;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.4f", calc);
    std::string s(buf);
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    resultStr = "Result: " + s;
    leftOperand = s;
    currentOperator = '\0';
    rightOperand.clear();
    return true;
}

std::string Calculator::getExpression() const {
    std::ostringstream oss;
    oss << (leftOperand.empty() ? "0" : leftOperand);
    if (currentOperator != '\0')
        oss << " " << currentOperator << " ";
    oss << rightOperand;
    return oss.str();
}

std::string Calculator::getResult() const {
    return resultStr;
}

void Calculator::memoryStore() {
    if (!leftOperand.empty())
        memoryValue = toFloat(leftOperand);
}

void Calculator::memoryRecall() {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.4f", memoryValue);
    std::string mem(buf);
    if (mem.find('.') != std::string::npos) {
        while (!mem.empty() && mem.back() == '0') mem.pop_back();
        if (!mem.empty() && mem.back() == '.') mem.pop_back();
    }
    if (currentOperator == '\0')
        leftOperand = mem;
    else
        rightOperand = mem;
}

float Calculator::getMemory() const {
    return memoryValue;
}

void Calculator::setMemory(float val) {
    memoryValue = val;
}

float Calculator::toFloat(const std::string& s) {
    try { return std::stof(s); }
    catch (...) { return 0.0f; }
}

std::string Calculator::convertNumber(const std::string& input, int fromBase, int toBase) {
    char* endptr = nullptr;
    long long value = std::strtoll(input.c_str(), &endptr, fromBase);
    if (endptr == input.c_str())
        return "Invalid input";
    std::ostringstream oss;
    if (toBase == 10) {
        oss << value;
    }
    else if (toBase == 16) {
        oss << std::uppercase << std::hex << value;
    }
    else if (toBase == 8) {
        oss << std::oct << value;
    }
    else if (toBase == 2) {
        if (value == 0) return "0";
        bool negative = value < 0;
        unsigned long long uval = negative ? -value : value;
        std::string bin;
        while (uval) {
            bin.push_back((uval & 1) ? '1' : '0');
            uval >>= 1;
        }
        if (negative) bin.push_back('-');
        std::reverse(bin.begin(), bin.end());
        return bin;
    }
    else {
        return "Unsupported base";
    }
    return oss.str();
}
