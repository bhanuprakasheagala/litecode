#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <variant>

namespace bytecode {

struct Function;

using ConstantValue = std::variant<std::nullptr_t, bool, double, std::string, std::shared_ptr<Function>>;

/**
 * @brief Compiled function object used by the VM.
 *
 * This is intentionally a lightweight representation of a callable chunk: its
 * name, parameter list, bytecode, and constant pool. The VM reads this structure
 * as a runtime code object during nested calls.
 */
struct Function {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::uint8_t> code;
    std::vector<ConstantValue> constants;
};

/**
 * @brief Bytecode instruction set for the stack-based VM bootstrap path.
 *
 * The opcode list is deliberately compact and close to the project’s learning-oriented
 * design. Each instruction encodes a small execution primitive such as loading a
 * constant, mutating the stack, branching, or invoking a function.
 */
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

/**
 * @brief Linear bytecode container plus a constant pool.
 *
 * A chunk is the VM equivalent of a code object: a flat instruction stream plus
 * the literals and nested function objects referenced by that stream. The current
 * VM supports block-local slots and function calls, but function objects do not
 * capture lexical environments, so this representation is not yet closure-capable.
 */
class Chunk {
public:
    Chunk() = default;
    Chunk(const std::vector<uint8_t>& code, const std::vector<ConstantValue>& constants)
        : code_(code), constants_(constants) {}

    /** @brief Appends one encoded opcode or operand byte to the instruction stream. */
    void write(uint8_t byte) {
        code_.push_back(byte);
    }

    /** @brief Replaces an already-emitted byte; an out-of-range index is ignored. */
    void writeAt(std::size_t index, uint8_t byte) {
        if (index < code_.size()) {
            code_[index] = byte;
        }
    }

    /** @brief Encodes an opcode as one byte. */
    void writeOp(OpCode op) {
        write(static_cast<uint8_t>(op));
    }

    /**
     * @brief Adds a constant and emits its pool index as a one-byte operand.
     *
     * Callers must keep the pool within the operand's representable range; this
     * low-level container does not diagnose index overflow.
     */
    void writeConstant(ConstantValue value) {
        constants_.push_back(std::move(value));
        write(static_cast<uint8_t>(constants_.size() - 1));
    }

    void writeConstantValue(std::shared_ptr<Function> value) {
        writeConstant(ConstantValue{std::move(value)});
    }

    const std::vector<uint8_t>& code() const { return code_; }
    const std::vector<ConstantValue>& constants() const { return constants_; }

private:
    std::vector<uint8_t> code_;
    std::vector<ConstantValue> constants_;
};

}  // namespace bytecode

#endif  // CHUNK_HPP
