#include <ostream>
#include <variant>

#include "parser.h"
#include "tokenizer.h"

class CodeGenerator {
public:
    explicit CodeGenerator(std::ostream& out);

    void Generate(const Module& module);

private:
    std::ostream& out_;
    size_t indent_level_ = 0;

    void Indent();
    void NewLine();
    void StartBlock();
    void EndBlock();

    void GenerateModule(const Module& module);
    void GenerateImports(const Imports& imports);

    void GenerateDeclaration(const Declaration& decl);
    void GenerateConstant(const Constant& constant);
    void GenerateFunction(const Function& func);

    void GenerateExpression(const Expression& expr, int parent_precedence = 0,
                            Operator parent_operator = Operator::ROOT);
    void GenerateBinaryOperation(const BinaryOperation& op, int parent_precedence = 0,
                                 Operator parent_operator = Operator::ROOT);
    void GenerateFunctionCall(const FunctionCall& call);
    void GenerateVariable(const Variable& var);
    void GenerateNumber(const Number& n);
    void GenerateFloat(const Float& f);
};
