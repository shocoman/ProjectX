#ifndef TAC_PARSER_QUADRUPLE_HPP
#define TAC_PARSER_QUADRUPLE_HPP

#include <string>
#include <ostream>
#include <utility>
#include <vector>
#include <optional>
#include <iostream>


struct Dest {
    enum class Type {
        None, Var, ArraySet, Deref, JumpLabel,
    };

    Type dest_type{};
    std::string dest_name;
    std::optional<std::string> element_name;

    Dest(std::string dest_name, std::optional<std::string> element_name, Type dest_type)
            : dest_name(std::move(dest_name)), element_name(std::move(element_name)), dest_type(dest_type) {}

    Dest() = default;

    friend std::ostream &operator<<(std::ostream &os, const Dest &destination) {
        const char *type_names[] = {"None", "Var", "ArraySet", "Deref", "JumpLabel"};
        os << "dest_type: " << type_names[static_cast<int>(destination.dest_type)]
           << "; dest_name: " << destination.dest_name;
        if (destination.element_name.has_value()) os << "; element_name: " << destination.element_name.value();
        return os;
    }

    std::optional<std::string> fmt() const {
        switch (dest_type) {
            case Type::Var:
                return dest_name;
            case Type::ArraySet:
                return dest_name + "[" + element_name.value() + "]";
            case Type::Deref:
                return "*" + dest_name;
            case Type::JumpLabel:
                return dest_name;
            default:
                return {};
        }
    }
};


struct Operand {
    enum class Type {
        None, Var, LInt, LDouble, LBool, LChar
    };

    std::string value;
    Type type;

    Operand() : type(Type::None) {}

    Operand(std::string s, Type t) : value(std::move(s)), type(t) {}

    Operand(const std::string &s) {
        char *end = nullptr;
        if (strtol(s.c_str(), &end, 10); end != s.c_str() && *end == '\0') {
            type = Type::LInt;
        } else if (strtod(s.c_str(), &end); end != s.c_str() && *end == '\0') {
            type = Type::LDouble;
        } else if (!s.empty()) {
            type = Type::Var;
        } else {
            type = Type::None;
        }
        value = s;
    }

    std::string get_string() const {
        return value;
    }

    int get_int() const {
        char *end = nullptr;
        return strtol(value.c_str(), &end, 10);
    }

    double get_double() const {
        char *end = nullptr;
        return strtod(value.c_str(), &end);
    }

    bool is_var() const { return type == Type::Var; }

    bool is_int() const { return type == Type::LInt; }

    bool is_double() const { return type == Type::LDouble; }

    bool is_number() const {
        return type == Type::LInt || type == Type::LDouble;
    }

    void clear() {
        value.clear();
        type = Type::None;
    }

    bool operator==(const Operand &rhs) const {
        if (is_number() && rhs.is_number())
            return get_double() == rhs.get_double();
        else
            return value == rhs.value;
    }

};

struct Quad {
    enum class Type {
        Nop, Add, Sub, Mult, Div, UMinus,
        Lt, Gt, Eq, Neq,
        Assign, Deref, Ref, ArrayGet,
        IfTrue, IfFalse, Goto, Halt, Call, Param, Return,
    };

    std::optional<Dest> dest{};
    Operand op1{};
    Operand op2{};
    Type type{};


    Quad(std::string op1, std::string op2, Type op_type, const Dest &dest = {})
            : op1(std::move(op1)), op2(std::move(op2)), type(op_type), dest(dest) {}

    Quad() = default;

    static bool is_commutative(Type t) {
        return t == Type::Add || t == Type::Mult || t == Type::Assign
               || t == Type::Eq || t == Type::Neq;
    }


    std::vector<std::string> get_used_vars() {
        auto used_vars = get_rhs();
        if (auto l = get_lhs(); l.has_value()) {
            used_vars.push_back(l.value());
        }
        return used_vars;
    }

    std::optional<std::string> get_lhs() {
        if (dest && !is_jump()) {
            return (dest.value().dest_name);
        } else {
            return {};
        }
    }

    std::vector<std::string> get_rhs() {
        std::vector<std::string> rhs_vars;
        if (dest) {
            if (dest.value().dest_type == Dest::Type::ArraySet)
                rhs_vars.push_back(dest.value().element_name.value());
        }
        if (!op1.value.empty()) rhs_vars.push_back(op1.get_string());
        if (!op2.value.empty()) rhs_vars.push_back(op2.get_string());
        return rhs_vars;
    }

    bool is_jump() const {
        return type == Type::Goto
               || type == Type::IfTrue
               || type == Type::IfFalse;
    }


    friend std::ostream &operator<<(std::ostream &os, const Quad &quad) {
        const char *type_names[] = {"Nop", "Add", "Sub", "Mult", "Div", "UMinus", "Lt", "Gt", "Eq", "Neq", "Assign",
                                    "Deref", "Ref", "ArrayGet", "IfTrue", "IfFalse", "Goto", "Halt", "Call", "Param",
                                    "Return"};
        if (quad.dest.has_value()) os << "dest: { " << quad.dest.value() << "}" << "; ";
        if (quad.op1.type != Operand::Type::None)
            os << "op1: " << quad.op1.get_string() << "; ";
        if (quad.op2.type != Operand::Type::None)
            os << "op2: " << quad.op2.get_string() << "; ";
        os << "type: " << type_names[static_cast<int>(quad.type)];
        return os;
    }


    std::string fmt() const {
        std::optional<std::string> destination = dest.has_value() ? dest.value().fmt() : std::nullopt;

        std::string command;
        if (destination.has_value()) {
            command += destination.value() + " = ";
        }

        std::optional<std::string> op;
        std::optional<std::string> unary_op;
        switch (type) {
            case Type::Add:
                op = "+";
                break;
            case Type::Sub:
                op = "-";
                break;
            case Type::Mult:
                op = "*";
                break;
            case Type::Div:
                op = "/";
                break;
            case Type::UMinus:
                unary_op = "-";
                break;
            case Type::Lt:
                op = "<";
                break;
            case Type::Gt:
                op = ">";
                break;
            case Type::Eq:
                op = "==";
                break;
            case Type::Neq:
                op = "!=";
                break;
            case Type::Assign:
                unary_op = "";
                break;
            case Type::Deref:
                unary_op = "*";
                break;
            case Type::Ref:
                unary_op = "&";
                break;
            case Type::ArrayGet:
                return destination.value() + " = " + op1.get_string() + "[" + op2.get_string() + "]";
            case Type::IfTrue:
                return "if " + op1.get_string() + " goto " + destination.value();
            case Type::IfFalse:
                return "ifFalse " + op1.get_string() + " goto " + destination.value();
            case Type::Goto:
                return "goto " + destination.value();
            case Type::Halt:
                return "halt";
            case Type::Call:
                return "call " + op1.get_string() + " " + op2.get_string();
            case Type::Param:
                return "param " + op1.get_string();
            case Type::Nop:
                return "nop";
            case Type::Return:
                return "return";

        }

        if (unary_op.has_value()) {
            return destination.value() + " = " + unary_op.value() + op1.get_string();
        } else {
            return destination.value() + " = " + op1.get_string() + op.value() + op2.get_string();
        }
    }
};


#endif //TAC_PARSER_QUADRUPLE_HPP


