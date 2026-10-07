#include "node.hpp"

#include <ranges>

#include "utils/operator.hpp"
#include "utils/type.hpp"

namespace ast {

namespace {
class NodePrinter final : public NodeVisitor<type::TypeID>, public ExpressionVisitor {
public:
    void print_program(Program<type::TypeID>& program) {
        fmt::println("Program");
        const size_t size = program.children.size();
        for (size_t i = 0; i < size; i++) {
            const bool is_last = i == size - 1;
            print_node(program.children[i], is_last);
        }
    }

private:
    std::string prefix;

    template <typename Node>
    void print_node(std::unique_ptr<Node>& node, const bool is_last) {
        fmt::print("{}{}", this->prefix, is_last ? "\\- " : "|- ");
        auto old_prefix = std::move(this->prefix);
        this->prefix = old_prefix + (is_last ? "   " : "|  ");
        node->accept(this);
        this->prefix = std::move(old_prefix);
    }

    void print_node(const type::BuiltinType type, const bool is_last) const {
        fmt::println("{}{}Type: {}", this->prefix, is_last ? "\\- " : "|- ", type);
    }

    void print_node(const BinaryOperator bin_op, const bool is_last) const {
        fmt::println(
            "{}{}Binary operator: {}", this->prefix, is_last ? "\\- " : "|- ", bin_op
        );
    }

    void print_node(const UnaryOperator unary_op, const bool is_last) const {
        fmt::println(
            "{}{}Unary operator: {}", this->prefix, is_last ? "\\- " : "|- ", unary_op
        );
    }

    void print_node(const AtomIdentifier* ident, const bool is_last) const {
        fmt::println(
            "{}{}Identifier: {}", this->prefix, is_last ? "\\- " : "|- ", ident->name
        );
    }

    void print_node(const AtomIntegerLiteral* int_literal, const bool is_last) const {
        fmt::println(
            "{}{}Integer literal: {}", this->prefix, is_last ? "\\- " : "|- ",
            int_literal->value
        );
    }

    void print_node(Atom* atom, const bool is_last) const {
        if (const auto* ident = dynamic_cast<AtomIdentifier*>(atom)) {
            this->print_node(ident, is_last);
        } else if (const auto* lit = dynamic_cast<AtomIntegerLiteral*>(atom)) {
            this->print_node(lit, is_last);
        }
    }

    void print_node(const type::TypeID type, const bool is_last) const {
        fmt::println("{}{} Type: {}", this->prefix, is_last ? "\\-" : "|- ", type);
    }

public:
    void visit(AtomExpression* atom) override {
        fmt::println("Atom expression");

        print_node(atom->atom.get(), true);
    }

    void visit(ParenExpression* expr) override {
        fmt::println("Paren expression");

        print_node(expr->expression, true);
    }

    void visit(BinaryExpression* bin_expression) override {
        fmt::println("Binary expression");

        print_node(bin_expression->op, false);
        print_node(bin_expression->left, false);
        print_node(bin_expression->right, true);
    }

    void visit(UnaryExpression* unary_expression) override {
        fmt::println("Unary expression");

        print_node(unary_expression->op, false);
        print_node(unary_expression->expression, true);
    }

    void visit(CallExpression* call_expression) override {
        fmt::println("Call expression");

        auto& args = call_expression->arguments;
        print_node(call_expression->function.get(), args.empty());
        if (!args.empty()) {
            for (auto& arg : args | std::views::take(args.size() - 1)) {
                print_node(arg, false);
            }
            print_node(args.back(), true);
        }
    }

    void visit(VariableDeclaration<type::TypeID>* var) override {
        fmt::println("Variable declaration");

        fmt::println("{}|- Type: auto", this->prefix);
        print_node(var->name.get(), var->initializer == nullptr);
        if (var->initializer) { print_node(var->initializer, true); }
    }

    void visit(FunctionParameter<type::TypeID>* arg) override {
        fmt::println("Function argument");

        print_node(arg->type, false);
        print_node(arg->name.get(), true);
    }

    void visit(FunctionDeclaration<type::TypeID>* func) override {
        fmt::println("Function");

        print_node(func->name.get(), false);
        print_node(func->return_type, false);
        for (auto& arg : func->arguments) { print_node(arg, false); }
        print_node(func->body, true);
    }

    void visit(StructField<type::TypeID>* struct_field) override {
        fmt::println("StructField");
        print_node(struct_field->type, false);
        print_node(struct_field->name.get(), true);
    }

    void visit(StructDeclaration<type::TypeID>* struct_decl) override {
        fmt::println("Struct");

        const size_t size = struct_decl->fields.size();
        print_node(struct_decl->name.get(), size == 0);

        for (size_t i = 0; i < size; i++) {
            const bool is_last = i == size - 1;
            print_node(struct_decl->fields[i], is_last);
        }
    }

    void visit(BlockStatement<type::TypeID>* statement) override {
        fmt::println("Block");
        const size_t size = statement->entries.size();
        for (size_t i = 0; i < size; i++) {
            const bool is_last = i == size - 1;
            print_node(statement->entries[i], is_last);
        }
    }

    void visit(AssignmentStatement<type::TypeID>* assignment) override {
        fmt::println("Assignment");

        print_node(assignment->identifier.get(), false);
        print_node(assignment->expression, true);
    }

    void visit(IfStatement<type::TypeID>* if_statement) override {
        fmt::println("If");

        print_node(if_statement->condition, false);
        print_node(if_statement->true_case, !if_statement->has_false_case());
        if (!if_statement->has_false_case()) { return; }
        print_node(if_statement->false_case, true);
    }

    void visit(WhileStatement<type::TypeID>* while_statement) override {
        fmt::println("While");

        print_node(while_statement->condition, false);
        print_node(while_statement->block, true);
    }

    void visit(ReturnStatement<type::TypeID>* return_statement) override {
        fmt::println("Return");

        if (return_statement->expression) {
            print_node(return_statement->expression, true);
        }
    }

    void visit(ExpressionStatement<type::TypeID>* expression_statement) override {
        fmt::println("Expression");

        print_node(expression_statement->expression, true);
    }
};

}  // namespace

void print_program(Program<type::TypeID>& program) {
    NodePrinter printer{};
    printer.print_program(program);
}

}  // namespace ast
