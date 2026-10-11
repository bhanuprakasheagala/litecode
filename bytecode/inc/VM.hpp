#ifndef VM_HPP
#define VM_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <variant>

#include "Chunk.hpp"

namespace bytecode {

/**
 * @brief Status returned by the experimental bytecode execution path.
 * @details CompileError is reserved for the combined host pipeline; the VM itself returns Ok or RuntimeError.
 */
enum class InterpretStatus {
    Ok,
    CompileError,
    RuntimeError
};

/**
 * @brief Runtime value used by the bytecode VM.
 *
 * This is the VM-side equivalent of the tree-walk runtime value. It is smaller
 * than the tree-walk model but covers the same essential categories: numbers,
 * strings, booleans, nil, and callable function objects.
 */
using Value = std::variant<std::nullptr_t, bool, double, std::string, std::shared_ptr<Function>>;

/**
 * @brief Stack-oriented execution engine paired with Litecode's bytecode compiler.
 *
 * This is the `--vm` alternative to the default tree-walk interpreter: it executes
 * compiler-produced chunks rather than traversing AST nodes. It exists to explore
 * bytecode execution while the tree-walk interpreter remains the complete,
 * authoritative runtime.
 *
 * The VM uses an operand stack, local slots, and globals. Function calls use fresh
 * VM frames with copied globals; lexical closures and the parsed class/property
 * opcodes are not implemented by this execution loop.
 */
class VirtualMachine {
public:
    VirtualMachine() = default;

    /**
     * @brief Executes one compiled chunk and writes output through the supplied stream.
     * @param chunk Bytecode chunk to interpret.
     * @param out Output stream for `print` statements and diagnostics.
     * @return Status describing whether execution succeeded or failed.
     */
    InterpretStatus interpret(const Chunk& chunk, std::ostream& out);

private:
    std::vector<Value> stack_;
    std::vector<Value> locals_;
    std::unordered_map<std::string, Value> globals_;
    Value returnValue_ = nullptr;
    bool hasReturnValue_ = false;
    bool suppressResultOutput_ = false;
    std::size_t ip_ = 0;

    void push(const Value& value);
    /** @brief Reads the operand-stack top; caller must ensure the stack is nonempty. */
    Value peek() const;
    /** @brief Removes the operand-stack top; caller must ensure the stack is nonempty. */
    Value pop();
    /**
     * @brief Runs a function in a fresh VM frame initialized from the arguments.
     *
     * Globals are copied into the callee VM, and local slots are initialized from
     * parameters. This is not closure capture; a failed nested execution currently
     * returns nil to the caller rather than propagating its status.
     */
    Value interpretFunction(const Function& function, const std::vector<Value>& args, std::ostream& out);

    static bool isTruthy(const Value& value);
    static bool isEqual(const Value& lhs, const Value& rhs);
    static std::string stringify(const Value& value);
};

}  // namespace bytecode

#endif  // VM_HPP
