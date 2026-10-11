#include "VM.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace bytecode {

/**
 * @brief Executes a compiled chunk and writes any output to the provided stream.
 *
 * The VM loops over compiler-emitted instructions, mutates a value stack, and
 * returns RuntimeError when an operation encounters an invalid value or name.
 * It assumes bytecode operands and constant indices are valid; malformed chunks
 * are not verified before execution. This remains a bootstrap path, not a hardened
 * bytecode engine.
 */
InterpretStatus VirtualMachine::interpret(const Chunk& chunk, std::ostream& out) {
    returnValue_ = nullptr;
    hasReturnValue_ = false;
    ip_ = 0;
    stack_.clear();

    while (ip_ < chunk.code().size()) {
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
                const Value value = pop();
                if (!std::holds_alternative<double>(value)) {
                    return InterpretStatus::RuntimeError;
                }
                push(-std::get<double>(value));
                break;
            }
            case OpCode::Add: {
                const Value rhs = pop();
                const Value lhs = pop();
                if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
                    push(std::get<double>(lhs) + std::get<double>(rhs));
                } else if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs)) {
                    push(std::get<std::string>(lhs) + std::get<std::string>(rhs));
                } else {
                    return InterpretStatus::RuntimeError;
                }
                break;
            }
            case OpCode::Subtract: {
                const Value rhs = pop();
                const Value lhs = pop();
                if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                    return InterpretStatus::RuntimeError;
                }
                push(std::get<double>(lhs) - std::get<double>(rhs));
                break;
            }
            case OpCode::Multiply: {
                const Value rhs = pop();
                const Value lhs = pop();
                if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                    return InterpretStatus::RuntimeError;
                }
                push(std::get<double>(lhs) * std::get<double>(rhs));
                break;
            }
            case OpCode::Divide: {
                const Value rhs = pop();
                const Value lhs = pop();
                if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                    return InterpretStatus::RuntimeError;
                }
                push(std::get<double>(lhs) / std::get<double>(rhs));
                break;
            }
            case OpCode::Equal: {
                const Value rhs = pop();
                const Value lhs = pop();
                push(isEqual(lhs, rhs));
                break;
            }
            case OpCode::Greater: {
                const Value rhs = pop();
                const Value lhs = pop();
                if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                    return InterpretStatus::RuntimeError;
                }
                push(std::get<double>(lhs) > std::get<double>(rhs));
                break;
            }
            case OpCode::Less: {
                const Value rhs = pop();
                const Value lhs = pop();
                if (!std::holds_alternative<double>(lhs) || !std::holds_alternative<double>(rhs)) {
                    return InterpretStatus::RuntimeError;
                }
                push(std::get<double>(lhs) < std::get<double>(rhs));
                break;
            }
            case OpCode::Print: {
                const Value value = pop();
                out << stringify(value) << '\n';
                break;
            }
            case OpCode::Pop:
                pop();
                break;
            case OpCode::Not: {
                const Value value = pop();
                push(!isTruthy(value));
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
                const std::string& name = std::get<std::string>(chunk.constants()[index]);
                const Value value = pop();
                globals_[name] = value;
                push(value);
                break;
            }
            case OpCode::GetGlobal: {
                const auto index = static_cast<std::size_t>(chunk.code()[ip_++]);
                const std::string& name = std::get<std::string>(chunk.constants()[index]);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    return InterpretStatus::RuntimeError;
                }
                push(it->second);
                break;
            }
            case OpCode::SetGlobal: {
                const auto index = static_cast<std::size_t>(chunk.code()[ip_++]);
                const std::string& name = std::get<std::string>(chunk.constants()[index]);
                auto it = globals_.find(name);
                if (it == globals_.end()) {
                    return InterpretStatus::RuntimeError;
                }
                const Value value = pop();
                it->second = value;
                push(value);
                break;
            }
            case OpCode::GetLocal: {
                const auto slot = static_cast<std::size_t>(chunk.code()[ip_++]);
                if (slot >= locals_.size()) {
                    return InterpretStatus::RuntimeError;
                }
                push(locals_[slot]);
                break;
            }
            case OpCode::SetLocal: {
                const auto slot = static_cast<std::size_t>(chunk.code()[ip_++]);
                if (slot >= locals_.size()) {
                    locals_.resize(slot + 1);
                }
                const Value value = pop();
                locals_[slot] = value;
                push(value);
                break;
            }
            case OpCode::Call: {
                const auto argCount = static_cast<std::size_t>(chunk.code()[ip_++]);
                std::vector<Value> args;
                args.reserve(argCount);
                for (std::size_t i = 0; i < argCount; ++i) {
                    args.push_back(pop());
                }
                std::reverse(args.begin(), args.end());

                const auto callee = pop();
                if (!std::holds_alternative<std::shared_ptr<Function>>(callee)) {
                    return InterpretStatus::RuntimeError;
                }

                const auto function = std::get<std::shared_ptr<Function>>(callee);
                if (!function) {
                    return InterpretStatus::RuntimeError;
                }

                const auto result = interpretFunction(*function, args, out);
                push(result);
                break;
            }
            case OpCode::Return: {
                if (!stack_.empty()) {
                    returnValue_ = pop();
                } else {
                    returnValue_ = nullptr;
                }
                hasReturnValue_ = true;
                break;
            }
            default:
                return InterpretStatus::RuntimeError;
        }
    }

    if (!suppressResultOutput_) {
        if (hasReturnValue_) {
            out << stringify(returnValue_) << '\n';
        } else if (!stack_.empty()) {
            const Value value = pop();
            out << stringify(value) << '\n';
        }
    }

    return InterpretStatus::Ok;
}

/**
 * @brief Runs a function with a new VM instance and returns its stored return value.
 *
 * The callee receives a copy of the caller's globals and parameter-initialized
 * locals. Since the function object has no closure environment, it cannot capture
 * the lexical locals of its declaration site.
 */
Value VirtualMachine::interpretFunction(const Function& function, const std::vector<Value>& args, std::ostream& out) {
    VirtualMachine callee;
    callee.globals_ = globals_;
    callee.locals_.clear();
    callee.locals_.resize(function.params.size());
    for (std::size_t i = 0; i < args.size() && i < function.params.size(); ++i) {
        callee.locals_[i] = args[i];
    }
    callee.suppressResultOutput_ = true;

    const Chunk functionChunk(function.code, function.constants);
    const auto status = callee.interpret(functionChunk, out);
    if (status != InterpretStatus::Ok) {
        return nullptr;
    }
    return callee.returnValue_;
}

/** @brief Adds one value to the operand stack. */
void VirtualMachine::push(const Value& value) {
    stack_.push_back(value);
}

/** @brief Reads the top operand without changing stack depth; requires a nonempty stack. */
Value VirtualMachine::peek() const {
    return stack_.back();
}

/** @brief Removes and returns the top operand; requires a nonempty stack. */
Value VirtualMachine::pop() {
    Value value = stack_.back();
    stack_.pop_back();
    return value;
}

/** @brief Applies Lox truthiness in the VM: nil and false are false, all else true. */
bool VirtualMachine::isTruthy(const Value& value) {
    if (std::holds_alternative<std::nullptr_t>(value)) {
        return false;
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    return true;
}

/** @brief Formats VM values for print instructions and the optional final result. */
std::string VirtualMachine::stringify(const Value& value) {
    if (std::holds_alternative<std::nullptr_t>(value)) {
        return "nil";
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<double>(value)) {
        std::ostringstream out;
        out << std::get<double>(value);
        return out.str();
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<std::shared_ptr<Function>>(value)) {
        const auto function = std::get<std::shared_ptr<Function>>(value);
        return function ? "<fn " + function->name + ">" : "<fn>";
    }
    return "<unknown>";
}

/** @brief Compares VM scalar values; function values currently compare by function name. */
bool VirtualMachine::isEqual(const Value& lhs, const Value& rhs) {
    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (std::holds_alternative<std::nullptr_t>(lhs)) {
        return true;
    }
    if (std::holds_alternative<bool>(lhs)) {
        return std::get<bool>(lhs) == std::get<bool>(rhs);
    }
    if (std::holds_alternative<double>(lhs)) {
        return std::get<double>(lhs) == std::get<double>(rhs);
    }
    if (std::holds_alternative<std::string>(lhs)) {
        return std::get<std::string>(lhs) == std::get<std::string>(rhs);
    }
    if (std::holds_alternative<std::shared_ptr<Function>>(lhs)) {
        const auto lhsFn = std::get<std::shared_ptr<Function>>(lhs);
        const auto rhsFn = std::get<std::shared_ptr<Function>>(rhs);
        return lhsFn && rhsFn && lhsFn->name == rhsFn->name;
    }
    return false;
}

}  // namespace bytecode
