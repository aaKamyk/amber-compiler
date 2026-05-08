#include "node.hpp"

#include <iostream>

#include "utils/operator.hpp"
#include "utils/type.hpp"

namespace ast {

class NodePrinter final : NodeVisitor, ExpressionVisitor {
public:
    void print_program(Program& program) {
        std::cout << "Program\n";
        const size_t size = program.children.size();
        for (size_t i = 0; i < size; i++) {
            const bool is_last = i == size - 1;
            print_node(program.children[i], is_last);
        }
    }

private:
    std::string prefix = "";

    template <typename Node>
    void print_node(std::unique_ptr<Node>& node, const bool is_last) {
        std::cout << this->prefix << (is_last ? "\\- " : "|- ");
        auto old_prefix = std::move(this->prefix);
        this->prefix = old_prefix + (is_last ? "   " : "|  ");
        node.get()->accept(this);
        this->prefix = std::move(old_prefix);
    }

    void print_node(const Type type, const bool is_last) const {
        std::cout << this->prefix << (is_last ? "\\- " : "|- ");
        std::cout << "Type: " << type << '\n';
    }

    void print_node(const BinaryOperator bin_op, const bool is_last) const {
        std::cout << this->prefix << (is_last ? "\\- " : "|- ");
        std::cout << "Binary operator: " << bin_op << '\n';
    }

    void print_node(const UnaryOperator unary_op, const bool is_last) const {
        std::cout << this->prefix << (is_last ? "\\- " : "|- ");
        std::cout << "Unary operator: " << unary_op << '\n';
    }

    void print_node(AtomIdentifier* ident, const bool is_last) const {
        std::cout << this->prefix << (is_last ? "\\- " : "|- ");
        std::cout << "Identifier: " << ident->name << '\n';
    }

    void print_node(AtomIntegerLiteral* int_literal, const bool is_last) const {
        std::cout << this->prefix << (is_last ? "\\- " : "|- ");
        std::cout << "Integer literal: " << int_literal->value << '\n';
    }

    void print_node(Atom* atom, const bool is_last) const {
        if (auto* ident = dynamic_cast<AtomIdentifier*>(atom)) {
            this->print_node(ident, is_last);
        } else if (auto* lit = dynamic_cast<AtomIntegerLiteral*>(atom)) {
            this->print_node(lit, is_last);
        }
    }

    void visit(AtomExpression* atom) override {
        std::cout << "Atom expression\n";

        print_node(atom->atom.get(), true);
    }

    void visit(ParenExpression* expr) override {
        std::cout << "Paren expression\n";

        print_node(expr->expression, true);
    }

    void visit(BinaryExpression* bin_expression) override {
        std::cout << "Binary expression\n";

        print_node(bin_expression->op, false);
        print_node(bin_expression->left, false);
        print_node(bin_expression->right, true);
    }

    void visit(UnaryExpression* unary_expression) override {
        std::cout << "Unary expression\n";

        print_node(unary_expression->op, false);
        print_node(unary_expression->expression, true);
    }

    void visit(VariableDeclaration* var) override {
        std::cout << "Variable\n";

        print_node(var->type, false);
        print_node(var->name.get(), var->value == nullptr);
        if (var->value) { print_node(var->value, true); }
    }

    void visit(FunctionArgument* arg) override {
        std::cout << "Function arg\n";

        print_node(arg->type, false);
        print_node(arg->name.get(), true);
    }

    void visit(FunctionDeclaration* func) override {
        std::cout << "Function\n";

        print_node(func->name.get(), false);
        print_node(func->return_type, false);
        for (auto& arg : func->arguments) { print_node(arg, false); }
        print_node(func->body, true);
    }

    void visit(BlockStatement* statement) override {
        std::cout << "Block\n";
        const size_t size = statement->entries.size();
        for (size_t i = 0; i < size; i++) {
            const bool is_last = i == size - 1;
            print_node(statement->entries[i], is_last);
        }
    }

    void visit(AssignmentStatement* assignment) override {
        std::cout << "Assignment\n";

        print_node(assignment->identifier.get(), false);
        print_node(assignment->expression, true);
    }

    void visit(IfStatement* if_statement) override {
        std::cout << "If\n";

        print_node(if_statement->condition, false);
        print_node(if_statement->true_case, !if_statement->has_false_case());
        if (!if_statement->has_false_case()) { return; }
        print_node(if_statement->false_case, true);
    }

    void visit(WhileStatement* while_statement) override {
        std::cout << "While\n";

        print_node(while_statement->condition, false);
        print_node(while_statement->block, true);
    }

    void visit(ReturnStatement* return_statement) override {
        std::cout << "Return\n";

        print_node(return_statement->expression, true);
    }
};

void BlockStatement::accept(NodeVisitor* v) {
    v->visit(this);
}

void AssignmentStatement::accept(NodeVisitor* v) {
    v->visit(this);
}

void IfStatement::accept(NodeVisitor* v) {
    v->visit(this);
}

void WhileStatement::accept(NodeVisitor* v) {
    v->visit(this);
}

void ReturnStatement::accept(NodeVisitor* v) {
    v->visit(this);
}

void VariableDeclaration::accept(NodeVisitor* v) {
    v->visit(this);
}

void FunctionArgument::accept(NodeVisitor* v) {
    v->visit(this);
}

void FunctionDeclaration::accept(NodeVisitor* v) {
    v->visit(this);
}

void print_program(Program& program) {
    NodePrinter printer{};
    printer.print_program(program);
}

}  // namespace ast
