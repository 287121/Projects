#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <string>

class Calculator {
public:
    Calculator();
    void clear();
    void backspace();
    void append(const char* str);
    void selectOperator(char op);
    void toggleSign();
    bool calculate();
    std::string getExpression() const;
    std::string getResult() const;
    void memoryStore();
    void memoryRecall();
    float getMemory() const;
    void setMemory(float val);
    static std::string convertNumber(const std::string& input, int fromBase, int toBase);

private:
    static float toFloat(const std::string& s);
    std::string leftOperand;
    std::string rightOperand;
    char currentOperator;
    std::string resultStr;
    float memoryValue;
};

#endif
