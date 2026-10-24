#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build_fresh"
BIN="${BUILD_DIR}/litecode"

pass() {
  echo "[pass] $1"
}

fail() {
  echo "[fail] $1" >&2
  exit 1
}

assert_contains() {
  local haystack="$1"
  local needle="$2"
  local name="$3"
  if [[ "${haystack}" != *"${needle}"* ]]; then
    fail "${name}: expected output to contain '${needle}'"
  fi
}

assert_not_contains() {
  local haystack="$1"
  local needle="$2"
  local name="$3"
  if [[ "${haystack}" == *"${needle}"* ]]; then
    fail "${name}: expected output to NOT contain '${needle}'"
  fi
}

echo "[info] configuring and building..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" >/dev/null
cmake --build "${BUILD_DIR}" -j >/dev/null

tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

# 1) Valid parse path
cat > "${tmpdir}/ok.lox" <<'EOF'
print 1 + 2;
EOF
ok_output="$("${BIN}" "${tmpdir}/ok.lox" 2>&1)"
assert_contains "${ok_output}" "3" "valid execution"
assert_not_contains "${ok_output}" "[PRINT]" "valid parse default token dump off"
pass "valid parse + default no token dump"

# 2) Lex error -> exit 65
cat > "${tmpdir}/lex_err.lox" <<'EOF'
print @;
EOF
set +e
lex_output="$("${BIN}" "${tmpdir}/lex_err.lox" 2>&1)"
lex_code=$?
set -e
[[ ${lex_code} -eq 65 ]] || fail "lex error exit code: expected 65, got ${lex_code}"
assert_contains "${lex_output}" "Unexpected character: @" "lex error message"
pass "lex error exit code + message"

# 3) Parse error -> exit 65
cat > "${tmpdir}/parse_err.lox" <<'EOF'
(1 + 2 * 3
EOF
set +e
parse_output="$("${BIN}" "${tmpdir}/parse_err.lox" 2>&1)"
parse_code=$?
set -e
[[ ${parse_code} -eq 65 ]] || fail "parse error exit code: expected 65, got ${parse_code}"
assert_contains "${parse_output}" "Expect ')' after expression." "parse error message"
pass "parse error exit code + message"

# 4) Missing file -> exit 66
set +e
missing_output="$("${BIN}" "${tmpdir}/does_not_exist.lox" 2>&1)"
missing_code=$?
set -e
[[ ${missing_code} -eq 66 ]] || fail "missing file exit code: expected 66, got ${missing_code}"
assert_contains "${missing_output}" "Unable to open file" "missing file message"
pass "missing file exit code + message"

# 5) Token dump env toggle
dump_output="$(LITECODE_DUMP_TOKENS=1 "${BIN}" "${tmpdir}/ok.lox" 2>&1)"
assert_contains "${dump_output}" "[PRINT]" "token dump enabled"
assert_contains "${dump_output}" "3" "token dump keeps runtime output"
pass "token dump toggle"

# 6) AST dump toggle
ast_output="$(LITECODE_DUMP_AST=1 "${BIN}" "${tmpdir}/ok.lox" 2>&1)"
assert_contains "${ast_output}" "(print (+ 1 2))" "ast dump enabled"
assert_contains "${ast_output}" "3" "ast dump keeps runtime output"
pass "ast dump toggle"

# 7) Function call + closure runtime
cat > "${tmpdir}/fn_ok.lox" <<'EOF'
fun add(a, b) {
  return a + b;
}
fun makeAdder(x) {
  fun inner(y) {
    return x + y;
  }
  return inner;
}
print add(2, 3);
var add10 = makeAdder(10);
print add10(5);
EOF
fn_output="$("${BIN}" "${tmpdir}/fn_ok.lox" 2>&1)"
assert_contains "${fn_output}" "5" "function call output"
assert_contains "${fn_output}" "15" "closure output"
pass "function + closure runtime"

# 8) Function arity mismatch
cat > "${tmpdir}/fn_arity.lox" <<'EOF'
fun one(a) { return a; }
print one();
EOF
set +e
arity_output="$("${BIN}" "${tmpdir}/fn_arity.lox" 2>&1)"
arity_code=$?
set -e
[[ ${arity_code} -eq 65 ]] || fail "arity error exit code: expected 65, got ${arity_code}"
assert_contains "${arity_output}" "Expected 1 arguments but got 0." "arity error message"
pass "function arity error"

# 9) Native clock() is callable
cat > "${tmpdir}/native_clock.lox" <<'EOF'
print clock();
EOF
clock_output="$("${BIN}" "${tmpdir}/native_clock.lox" 2>&1)"
assert_not_contains "${clock_output}" "Undefined variable 'clock'." "native clock availability"
pass "native clock callable"

# 10) Native clock arity check
cat > "${tmpdir}/native_clock_arity.lox" <<'EOF'
print clock(1);
EOF
set +e
clock_arity_output="$("${BIN}" "${tmpdir}/native_clock_arity.lox" 2>&1)"
clock_arity_code=$?
set -e
[[ ${clock_arity_code} -eq 65 ]] || fail "clock arity exit code: expected 65, got ${clock_arity_code}"
assert_contains "${clock_arity_output}" "Expected 0 arguments but got 1." "clock arity message"
pass "native clock arity"

# 11) Resolver: return outside function
cat > "${tmpdir}/resolver_return_top.lox" <<'EOF'
return 1;
EOF
set +e
resolver_return_output="$("${BIN}" "${tmpdir}/resolver_return_top.lox" 2>&1)"
resolver_return_code=$?
set -e
[[ ${resolver_return_code} -eq 65 ]] || fail "resolver top-level return exit code: expected 65, got ${resolver_return_code}"
assert_contains "${resolver_return_output}" "Can't return from top-level code." "resolver top-level return message"
pass "resolver top-level return"

# 12) Resolver: local variable in its own initializer
cat > "${tmpdir}/resolver_self_init.lox" <<'EOF'
{
  var a = a;
}
EOF
set +e
resolver_self_init_output="$("${BIN}" "${tmpdir}/resolver_self_init.lox" 2>&1)"
resolver_self_init_code=$?
set -e
[[ ${resolver_self_init_code} -eq 65 ]] || fail "resolver self-init exit code: expected 65, got ${resolver_self_init_code}"
assert_contains "${resolver_self_init_output}" "Can't read local variable in its own initializer." "resolver self-init message"
pass "resolver self-init"

# 13) Class fields and methods with this
cat > "${tmpdir}/class_basic.lox" <<'EOF'
class Bag {
  init(v) {
    this.value = v;
  }
  get() {
    return this.value;
  }
}
var b = Bag(41);
print b.get();
b.value = 99;
print b.value;
EOF
class_basic_output="$("${BIN}" "${tmpdir}/class_basic.lox" 2>&1)"
assert_contains "${class_basic_output}" "41" "class method output"
assert_contains "${class_basic_output}" "99" "class field set/get output"
pass "class basic runtime"

# 14) this outside class
cat > "${tmpdir}/class_this_outside.lox" <<'EOF'
print this;
EOF
set +e
class_this_outside_output="$("${BIN}" "${tmpdir}/class_this_outside.lox" 2>&1)"
class_this_outside_code=$?
set -e
[[ ${class_this_outside_code} -eq 65 ]] || fail "this outside class exit code: expected 65, got ${class_this_outside_code}"
assert_contains "${class_this_outside_output}" "Can't use 'this' outside of a class." "this outside class message"
pass "this outside class"

# 15) initializer cannot return value
cat > "${tmpdir}/class_init_return.lox" <<'EOF'
class C {
  init() {
    return 1;
  }
}
EOF
set +e
class_init_return_output="$("${BIN}" "${tmpdir}/class_init_return.lox" 2>&1)"
class_init_return_code=$?
set -e
[[ ${class_init_return_code} -eq 65 ]] || fail "initializer return exit code: expected 65, got ${class_init_return_code}"
assert_contains "${class_init_return_output}" "Can't return a value from an initializer." "initializer return message"
pass "initializer return restriction"

# 16) Inheritance and super dispatch
cat > "${tmpdir}/class_inheritance.lox" <<'EOF'
class A {
  speak() { return "A"; }
}
class B < A {
  speak() { return super.speak() + "B"; }
}
print B().speak();
EOF
inheritance_output="$("${BIN}" "${tmpdir}/class_inheritance.lox" 2>&1)"
assert_contains "${inheritance_output}" "AB" "inheritance super dispatch"
pass "inheritance + super dispatch"

# 17) super outside class
cat > "${tmpdir}/super_outside.lox" <<'EOF'
super.foo();
EOF
set +e
super_outside_output="$("${BIN}" "${tmpdir}/super_outside.lox" 2>&1)"
super_outside_code=$?
set -e
[[ ${super_outside_code} -eq 65 ]] || fail "super outside class exit code: expected 65, got ${super_outside_code}"
assert_contains "${super_outside_output}" "Can't use 'super' outside of a class." "super outside class message"
pass "super outside class"

# 18) super in class without superclass
cat > "${tmpdir}/super_no_superclass.lox" <<'EOF'
class C {
  method() { return super.method(); }
}
EOF
set +e
super_no_superclass_output="$("${BIN}" "${tmpdir}/super_no_superclass.lox" 2>&1)"
super_no_superclass_code=$?
set -e
[[ ${super_no_superclass_code} -eq 65 ]] || fail "super without superclass exit code: expected 65, got ${super_no_superclass_code}"
assert_contains "${super_no_superclass_output}" "Can't use 'super' in a class with no superclass." "super without superclass message"
pass "super without superclass"

# 19) class cannot inherit from itself
cat > "${tmpdir}/self_inherit.lox" <<'EOF'
class Oops < Oops {}
EOF
set +e
self_inherit_output="$("${BIN}" "${tmpdir}/self_inherit.lox" 2>&1)"
self_inherit_code=$?
set -e
[[ ${self_inherit_code} -eq 65 ]] || fail "self inheritance exit code: expected 65, got ${self_inherit_code}"
assert_contains "${self_inherit_output}" "A class can't inherit from itself." "self inheritance message"
pass "self inheritance check"

# 20) REPL .reset clears retained definitions
repl_reset_output="$(printf 'var a = 1;\n.reset\nprint a;\n' | "${BIN}" 2>&1)"
assert_contains "${repl_reset_output}" "State reset." "repl reset acknowledgement"
assert_contains "${repl_reset_output}" "Undefined variable 'a'." "repl reset clears previous state"
pass "repl reset command"

echo "[info] all smoke checks passed"
