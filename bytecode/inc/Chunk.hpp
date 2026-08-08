#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <variant>

namespace bytecode {

enum class OpCode : std::uint8_t {
    Constant,
    Return,
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    Equal,
    Greater,
    Less,
    Print,
    Pop,
    True,
    False,
    Nil,
    Not,
    DefineGlobal,
    GetGlobal,
    SetGlobal,
    GetLocal,
    SetLocal,
    JumpIfFalse,
    Jump,
    Call,
    Closure,
    Class,
    Method,
    GetField,
    SetField,
    GetSuper,
    SuperInvoke
};

using ConstantValue = std::variant<std::nullptr_t, bool, double, std::string>;

class Chunk {
public:
    Chunk() = default;

    void write(uint8_t byte) {
        code_.push_back(byte);
    }

    void writeAt(std::size_t index, uint8_t byte) {
        if (index < code_.size()) {
            code_[index] = byte;
        }
    }

    void writeOp(OpCode op) {
        write(static_cast<uint8_t>(op));
    }

    void writeConstant(ConstantValue value) {
        constants_.push_back(std::move(value));
        write(static_cast<uint8_t>(constants_.size() - 1));
    }

    const std::vector<uint8_t>& code() const { return code_; }
    const std::vector<ConstantValue>& constants() const { return constants_; }

private:
    std::vector<uint8_t> code_;
    std::vector<ConstantValue> constants_;
};

}  // namespace bytecode

#endif  // CHUNK_HPP
