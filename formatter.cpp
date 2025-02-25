#include "formatter.h"

#include "parser.h"

CodeGenerator::CodeGenerator(std::ostream& out) : out_(out) {
}

void CodeGenerator::Generate(const Module& module) {
    GenerateModule(module);
}

void CodeGenerator::Indent() {
    for (size_t i = 0; i < indent_level_; ++i) {
        out_ << "  ";
    }
}

void CodeGenerator::NewLine() {
    out_ << '\n';
    Indent();
}

void CodeGenerator::StartBlock() {
    ++indent_level_;
    NewLine();
}

void CodeGenerator::EndBlock() {
    --indent_level_;
    NewLine();
}

void CodeGenerator::GenerateModule(const Module& module) {
    if (!module.name_.empty()) {
        out_ << "module " << module.name_ << " where";
        StartBlock();
    }
    GenerateImports(module.imports_);
    for (const auto& decl : module.declarations_) {
        GenerateDeclaration(decl);
    }
    if (!module.name_.empty()) {
        EndBlock();
    }
}

void CodeGenerator::GenerateImports(const Imports& imports) {
    for (auto const [name, info] : imports.modules_map_) {
        auto [alias, funcs] = info;
        out_ << "import " << name;
        if (name != alias) {
            out_ << " as " << alias;
        }
        if (!funcs.empty()) {
            out_ << " (";
            for (auto it = funcs.begin(); it != funcs.end(); ++it) {
                if (it != funcs.begin()) {
                    out_ << ", ";
                }
                out_ << *it;
            }
            out_ << ")";
        }
        NewLine();
    }
}

void CodeGenerator::GenerateDeclaration(const Declaration& decl) {
    if (std::holds_alternative<Constant>(decl)) {
        GenerateConstant(std::get<Constant>(decl));
    } else if (std::holds_alternative<Function>(decl)) {
        GenerateFunction(std::get<Function>(decl));
    } else if (std::holds_alternative<Module>(decl)) {
        GenerateModule(std::get<Module>(decl));
    }
}

void CodeGenerator::GenerateConstant(const Constant& constant) {
    out_ << "let " << constant.name_ << " := ";
    GenerateExpression(constant.value_);
    NewLine();
}

void CodeGenerator::GenerateFunction(const Function& func) {
    out_ << "let " << func.name_;
    if (!func.parameters_.empty()) {
        out_ << "(" << func.parameters_[0];
        for (auto it = func.parameters_.begin() + 1; it != func.parameters_.end(); ++it) {
            out_ << ", " << *it;
        }
        out_ << ")";
    }
    out_ << " := ";
    GenerateExpression(func.value_);
    if (func.body_) {
        out_ << " where";
        StartBlock();
        GenerateModule(*func.body_);
        EndBlock();
    } else {
        NewLine();
    }
}

void CodeGenerator::GenerateExpression(const Expression& expr, int parent_precedence) {
    if (std::holds_alternative<BinaryOperation>(expr)) {
        GenerateBinaryOperation(std::get<BinaryOperation>(expr), parent_precedence);
    } else if (std::holds_alternative<FunctionCall>(expr)) {
        GenerateFunctionCall(std::get<FunctionCall>(expr));
    } else if (std::holds_alternative<Variable>(expr)) {
        GenerateVariable(std::get<Variable>(expr));
    } else if (std::holds_alternative<Number>(expr)) {
        GenerateNumber(std::get<Number>(expr));
    } else if (std::holds_alternative<Float>(expr)) {
        GenerateFloat(std::get<Float>(expr));
    } else {
        std::cerr << "Token type to be outputted is not yet supported.\n";
        exit(3);
    }
}

const std::unordered_map<Operator, std::string> kOperatorToRepr = {{Operator::ADD, "+"},
                                                                   {Operator::SUB, "-"},
                                                                   {Operator::MUL, "*"},
                                                                   {Operator::DIV, "/"},
                                                                   {Operator::POW, "^"}};

const std::unordered_map<Operator, int> kOperatorPrecedence = {{Operator::ADD, 1},
                                                               {Operator::SUB, 1},
                                                               {Operator::MUL, 2},
                                                               {Operator::DIV, 2},
                                                               {Operator::POW, 3}};

// TODO(helloclock): looks horrible maybe i should rewrite it
void CodeGenerator::GenerateBinaryOperation(const BinaryOperation& op, int parent_precedence) {
    int child_precedence = kOperatorPrecedence.at(op.op_);
    bool place_brackets = (child_precedence < parent_precedence) &&
                          !((op.parent_operator_ == Operator::MUL && op.op_ == Operator::MUL) ||
                            (op.parent_operator_ == Operator::ADD && op.op_ == Operator::ADD));
    if (place_brackets) {
        out_ << "(";
    }
    int lhs_precedence = child_precedence, rhs_precedence = child_precedence;
    if (op.op_ == Operator::POW) {
        ++lhs_precedence;
    } else {
        ++rhs_precedence;
    }
    GenerateExpression(*op.lhs_, lhs_precedence);
    out_ << " " << kOperatorToRepr.at(op.op_) << " ";
    GenerateExpression(*op.rhs_, rhs_precedence);
    if (place_brackets) {
        out_ << ")";
    }
}

void CodeGenerator::GenerateFunctionCall(const FunctionCall& call) {
    out_ << call.name_ << "(";
    if (!call.args_.empty()) {
        GenerateExpression(call.args_[0]);
        for (auto it = call.args_.begin() + 1; it != call.args_.end(); ++it) {
            out_ << ", ";
            GenerateExpression(*it);
        }
    }
    out_ << ")";
}

void CodeGenerator::GenerateVariable(const Variable& var) {
    out_ << var.name_;
}

void CodeGenerator::GenerateNumber(const Number& n) {
    out_ << n.value_;
}

void CodeGenerator::GenerateFloat(const Float& f) {
    out_ << f.value_;
}
