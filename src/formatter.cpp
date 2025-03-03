#include <parser/formatter.h>
#include <parser/parser.h>

CodeGenerator::CodeGenerator(std::ostream& out) : out_(out) {
}

void CodeGenerator::Generate(const Module& module) {
    GenerateModule(module);
    out_ << "\n";
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
}

void CodeGenerator::GenerateModule(const Module& module) {
    if (!module.name_.empty()) {
        out_ << "module " << module.name_ << " where";
        StartBlock();
    }
    GenerateImports(module.imports_);
    bool newline_after_decls = false;
    for (const auto& decl : module.declarations_) {
        if (std::holds_alternative<Module>(decl)) {
            const Module& submod = std::get<Module>(decl);
            if (!submod.declarations_.empty() || !submod.imports_.modules_map_.empty()) {
                newline_after_decls = true;
                break;
            }
        }
    }
    bool first_decl = true;
    for (const auto& decl : module.declarations_) {
        if (!first_decl) {
            if (newline_after_decls) {
                out_ << "\n";
                NewLine();
            } else {
                NewLine();
            }
        }
        GenerateDeclaration(decl);
        first_decl = false;
    }
    if (!module.name_.empty()) {
        EndBlock();
    }
}

void CodeGenerator::GenerateImports(const Imports& imports) {
    for (auto const& [name, info] : imports.modules_map_) {
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

CodeGenerator::DeclarationVisitor::DeclarationVisitor(CodeGenerator& gen) : gen_(gen) {
}

void CodeGenerator::DeclarationVisitor::operator()(const Constant& c) {
    gen_.GenerateConstant(c);
}

void CodeGenerator::DeclarationVisitor::operator()(const Function& f) {
    gen_.GenerateFunction(f);
}

void CodeGenerator::DeclarationVisitor::operator()(const Module& m) {
    gen_.GenerateModule(m);
}

void CodeGenerator::GenerateDeclaration(const Declaration& decl) {
    std::visit(DeclarationVisitor(*this), decl);
}

void CodeGenerator::GenerateConstant(const Constant& constant) {
    out_ << "let " << constant.name_ << " := ";
    GenerateExpression(constant.value_);
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
    }
}

CodeGenerator::ExpressionVisitor::ExpressionVisitor(CodeGenerator& gen, int parent_precedence, Operator parent_operator)
    : gen_(gen), parent_precedence_(parent_precedence), parent_operator_(parent_operator) {
}

void CodeGenerator::ExpressionVisitor::operator()(const BinaryOperation& binop) {
    gen_.GenerateBinaryOperation(binop, parent_precedence_, parent_operator_);
}

void CodeGenerator::ExpressionVisitor::operator()(const FunctionCall& fc) {
    gen_.GenerateFunctionCall(fc);
}

void CodeGenerator::ExpressionVisitor::operator()(const Variable& v) {
    gen_.GenerateVariable(v);
}

void CodeGenerator::ExpressionVisitor::operator()(const Number& n) {
    gen_.GenerateNumber(n);
}

void CodeGenerator::ExpressionVisitor::operator()(const Float& f) {
    gen_.GenerateFloat(f);
}

void CodeGenerator::GenerateExpression(const Expression& expr, int parent_precedence, Operator parent_operator) {
    std::visit(ExpressionVisitor(*this, parent_precedence, parent_operator), expr);
}

const std::unordered_map<Operator, std::string> kOperatorToRepr = {
    {Operator::ADD, "+"}, {Operator::SUB, "-"}, {Operator::MUL, "*"}, {Operator::DIV, "/"}, {Operator::POW, "^"}};

const std::unordered_map<Operator, int> kOperatorPrecedence = {
    {Operator::ADD, 1}, {Operator::SUB, 1}, {Operator::MUL, 2}, {Operator::DIV, 2}, {Operator::POW, 3}};

void CodeGenerator::GenerateBinaryOperation(const BinaryOperation& op, int parent_precedence,
                                            Operator parent_operator) {
    int current_precedence = kOperatorPrecedence.at(op.op_);
    bool place_brackets = false;
    if (current_precedence < parent_precedence) {
        bool associative = op.op_ == Operator::ADD || op.op_ == Operator::MUL;
        if (!(associative && parent_operator == op.op_)) {
            place_brackets = true;
        }
    }
    if (place_brackets) {
        out_ << "(";
    }
    int lhs_precedence = current_precedence, rhs_precedence = current_precedence;
    if (op.op_ == Operator::POW) {
        ++lhs_precedence;
    } else {
        ++rhs_precedence;
    }
    GenerateExpression(*op.lhs_, lhs_precedence, op.op_);
    out_ << " " << kOperatorToRepr.at(op.op_) << " ";
    GenerateExpression(*op.rhs_, rhs_precedence, op.op_);
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
