#ifndef VM_HPP
#define VM_HPP

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <variant>

#include "Chunk.hpp"

namespace bytecode {

enum class InterpretStatus {
    Ok,
    CompileError,
    RuntimeError
};

using Value = std::variant<std::nullptr_t, bool, double, std::string>;

class VirtualMachine {
public:
    VirtualMachine() = default;

    InterpretStatus interpret(const Chunk& chunk, std::ostream& out) {
        ip_ = 0;
        stack_.clear();
        for (; ip_ < chunk.code().size(); ) {
            const auto op = static_cast<OpCode>(chunk.code()[ip_++]);
            switch (op) {
                case OpCode::Constant: {
                    const auto index = static_cast<std::size_t>(chunk.code()[ip_++]);
                    stack_.push_back(chunk.constants()[index]);
                    break;
                }
                case OpCode::Nil:
                    stack_.push_back(nullptr);
                    break;
                case OpCode::True:
                    stack_.push_back(true);
                    break;
                case OpCode::False:
                    stack_.push_back(false);
                    break;
                case OpCode::Negate: {
                    Value value = stack_.back();
                    stack_.pop_back();
                    if (!std::holds_alternative<double>(value)) {
                        return InterpretStatus::RuntimeError;
                    }
                    stack_.push_back(-std::get<double>(value));
                    break;
                }
                case OpCode::Add: {
                    auto rhs = stack_.back();
                    stack_.pop_back();
                    auto lhs = stack_.back();
                    stack_.pop_back();
                    if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
                        stack_.push_back(std::get<double>(lhs) + std::get<double>(rhs));
                    } else if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs)) {
                        stack_.push_back(std::get<std::string>(lhs) + std::get<std::string>(rhs));
                    } else {
                        return InterpretStatus::RuntimeError;
                    }
                    break;
                }
                case OpCode::Subtract: {
                    auto rhs = pop();
                    auto lhs = pop();
                    if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                        return InterpretStatus::RuntimeError;
                    }
                    push(std::get<double>(lhs) - std::get<double>(rhs));
                    break;
                }
                case OpCode::Multiply: {
                    auto rhs = pop();
                    auto lhs = pop();
                    if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                        return InterpretStatus::RuntimeError;
                    }
                    push(std::get<double>(lhs) * std::get<double>(rhs));
                    break;
                }
                case OpCode::Divide: {
                    auto rhs = pop();
                    auto lhs = pop();
                    if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                        return InterpretStatus::RuntimeError;
                    }
                    push(std::get<double>(lhs) / std::get<double>(rhs));
                    break;
                }
                case OpCode::Equal: {
                    auto rhs = pop();
                    auto lhs = pop();
                    stack_.push_back(isEqual(lhs, rhs));
                    break;
                }
                case OpCode::Greater: {
                    auto rhs = pop();
                    auto lhs = pop();
                    if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                        return InterpretStatus::RuntimeError;
                    }
                    push(std::get<double>(lhs) > std::get<double>(rhs));
                    break;
                }
                case OpCode::Less: {
                    auto rhs = pop();
                    auto lhs = pop();
                    if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                        return InterpretStatus::RuntimeError;
                    }
                    push(std::get<double>(lhs) < std::get<double>(rhs));
                    break;
                }
                case OpCode::Print: {
                    Value value = pop();
                    out << stringify(value) << '\n';
                    break;
                }
                case OpCode::Pop:
                    stack_.pop_back();
                    break;
                case OpCode::Not: {
                    Value value = pop();
                    stack_.push_back(!isTruthy(value));
                    break;
                }
                case OpCode::JumpIfFalse: {
                    const auto target = static_cast<std::size_t>(chunk.code()[ip_++]);
                    if (!isTruthy(pop())) {
                        ip_ = target;
                    }
                    break;
                }
                case OpCode::Jump: {
                    const auto target = static_cast<std::size_t>(chunk.code()[ip_++]);
                    ip_ = target;
                    break;
                }
                case OpCode::DefineGlobal: {
                    const auto index = static_cast<std::size_t>(chunk.code()[ip_++]);
                    const auto& name = std::get<std::string>(chunk.constants()[index]);
                    Value value = pop();
                    globals_[name] = value;
                    stack_.push_back(value);
                    break;
                }
                case OpCode::GetGlobal: {
                    const auto index = static_cast<std::size_t>(chunk.code()[ip_++]);
                    const auto& name = std::get<std::string>(chunk.constants()[index]);
                    auto it = globals_.find(name);
                    if (it == globals_.end()) {
                        return InterpretStatus::RuntimeError;
                    }
                    stack_.push_back(it->second);
                    break;
                }
                case OpCode::SetGlobal: {
                    const auto index = static_cast<std::size_t>(chunk.code()[ip_++]);
                    const auto& name = std::get<std::string>(chunk.constants()[index]);
                    Value value = pop();
                    auto it = globals_.find(name);
                    if (it == globals_.end()) {
                        return InterpretStatus::RuntimeError;
                    }
                    it->second = value;
                    stack_.push_back(value);
                    break;
                }
                case OpCode::GetLocal: {
                    const auto slot = static_cast<std::size_t>(chunk.code()[ip_++]);
                    if (slot >= locals_.size()) {
                        return InterpretStatus::RuntimeError;
                    }
                    stack_.push_back(locals_[slot]);
                    break;
                }
                case OpCode::SetLocal: {
                    const auto slot = static_cast<std::size_t>(chunk.code()[ip_++]);
                    if (slot >= locals_.size()) {
                        locals_.resize(slot + 1);
                    }
                    Value value = pop();
                    locals_[slot] = value;
                    stack_.push_back(value);
                    break;
                }
                case OpCode::Return:
                    if (!stack_.empty()) {
                        Value value = pop();
                        out << stringify(value) << '\n';
                    }
                    return InterpretStatus::Ok;
                default:
                    return InterpretStatus::RuntimeError;
            }
        }

        if (!stack_.empty()) {
            Value value = pop();
            out << stringify(value) << '\n';
        }

        return InterpretStatus::Ok;
    }

private:
    std::vector<Value> stack_;
    std::vector<Value> locals_;
    std::unordered_map<std::string, Value> globals_;
    std::size_t ip_ = 0;

    void push(const Value& value) {
        stack_.push_back(value);
    }

    Value peek() const {
        return stack_.back();
    }

    Value pop() {
        Value value = stack_.back();
        stack_.pop_back();
        return value;
    }

    static bool isTruthy(const Value& value) {
        if (std::holds_alternative<std::nullptr_t>(value)) return false;
        if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
        return true;
    }

    static bool isEqual(const Value& lhs, const Value& rhs) {
        if (lhs.index() != rhs.index()) return false;
        if (std::holds_alternative<std::nullptr_t>(lhs)) return true;
        if (std::holds_alternative<bool>(lhs)) return std::get<bool>(lhs) == std::get<bool>(rhs);
        if (std::holds_alternative<double>(lhs)) return std::get<double>(lhs) == std::get<double>(rhs);
        return std::get<std::string>(lhs) == std::get<std::string>(rhs);
    }

    static std::string stringify(const Value& value) {
        if (std::holds_alternative<std::nullptr_t>(value)) return "nil";
        if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "true" : "false";
        if (std::holds_alternative<double>(value)) {
            std::ostringstream out;
            out << std::get<double>(value);
            return out.str();
        }
        return std::get<std::string>(value);
    }
};

}  // namespace bytecode

#endif  // VM_HPP
