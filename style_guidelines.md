

# ESP-IDF Style Guide

This document outlines the coding standards and formatting rules for the Espressif IoT Development Framework (ESP-IDF). Following these conventions helps create readable, maintainable, and robust code.

---

## Purpose

The purpose of this style guide is to encourage use of common coding practices within the ESP-IDF.

- Style guidelines help create readable, maintainable, and robust code
- Writing code that looks the same across the codebase helps others read and comprehend it
- Using the same conventions for spaces and newlines reduces chances that future changes will produce huge unreadable diffs
- Following common patterns for module structure and using language features consistently helps others understand code behavior

> **Note:** Rules are kept simple and cannot cover all potential cases. In some cases, you must bend these simple rules to achieve readability, maintainability, or robustness.

> **Third-Party Code:** When modifying third-party code used in ESP-IDF, follow the way that particular project is written to help propose useful changes for merging into the upstream project.

---

## C Code Formatting

### Naming Conventions

| Rule | Example |
|------|---------|
| Single-file variables/functions → declare `static` | `static bool s_invert;` |
| Public names → use per-component/unit prefix | `esp_vfs_register()`, `esp_console_run()` |
| Static variables → prefix with `s_` | `static bool s_invert` |
| Avoid unnecessary abbreviations | Use `data` not `dat` (unless name becomes too long) |

### Indentation

- **Use four spaces** for each indentation level
- **Do not use tabs** for indentation
- Configure your editor to emit four spaces when pressing Tab

### Vertical Space

- Place **one empty line between functions**
- Do **not** begin or end a function with an empty line

```c
void function1()
{
    do_one_thing();
    do_another_thing();
}

void function2()
{
    int var = 0;
    while (var < SOME_CONSTANT) {
        do_stuff(&var);
    }
}
```

**Maximum line length:** 120 characters (as long as it does not seriously affect readability)

### Horizontal Space

| Context | Rule | Example |
|---------|------|---------|
| After `if`, `for`, `while`, `switch` keywords | Single space required | `if (condition) {` |
| Binary operators | Single space around | `y = x + z;` |
| Unary operators | No space necessary | `int y = -x;` |
| `*` and `/` operators | Space optional | `y = a*b/c;` or `y = a * b / c;` |
| `.`, `->` operators | No space necessary | `obj.field` |
| Trailing whitespace | Never add at end of line | ❌ |

**Alignment Note:** Horizontal alignment can help readability but may cause re-alignment issues if new lines are added. Use sparingly, especially for lists expected to grow.

```c
// Good use of alignment
esp_rom_gpio_connect_in_signal(PIN_CAM_D6,   I2S0I_DATA_IN14_IDX, false);
esp_rom_gpio_connect_in_signal(PIN_CAM_D7,   I2S0I_DATA_IN15_IDX, false);

// Never use TAB characters for alignment
// Never add trailing whitespace
```

### Braces

**Function definitions:** Opening brace on separate line

```c
// CORRECT
void function(int arg)
{
}

// INCORRECT
void function(int arg) {
}
```

**Conditionals and loops:** Opening brace on same line as statement

```c
if (condition) {
    do_one();
} else if (other_condition) {
    do_two();
}
```

### Comments

- Single-line comments: Use `//`
- Multi-line comments: Use `//` on each line OR `/* */` block

#### Comment Best Practices

| Do ✅ | Don't ❌ |
|-------|--------|
| Remove code completely if no longer required | Leave commented-out code without explanation |
| Add `TODO` with explanation if temporarily disabling | Store code snippets in `#if 0 ... #endif` for future use |
| Look up authorship via git | Add trivial comments about authorship/change date |

```c
// GOOD - explains why temporarily disabled
void init_something()
{
    setup_dma();
    // TODO: we should load resources here, but loader is not fully integrated yet.
    // load_resources();
    start_timer();
}

// BAD - unclear why commented
void init_something()
{
    setup_dma();
    // load_resources();                // WHY is this thing commented?
    start_timer();
}
```

### Line Endings

- Commits should only contain files with **LF (Unix style)** endings
- Windows users can configure git with `core.autocrlf`

**Converting LF to Unix endings:**
```bash
cd IDF_WORKING_DIRECTORY
git rebase --exec 'git diff-tree --no-commit-id --name-only -r HEAD | xargs dos2unix && git commit -a --amend --no-edit --allow-empty' master
```

**Single commit conversion:**
```bash
dos2unix FILENAME
git commit --amend
```

### Formatting Your Code

ESP-IDF uses **Astyle** to format source code. Configuration stored in `tools/ci/astyle-rules.yml`.

**Enable formatting for a component** by removing it from `components_not_formatted_temporary` list.

**Run formatting:**
```bash
pre-commit run --files <path_to_files> astyle_py
```

**Manual installation:**
```bash
pip install astyle_py==VERSION
astyle_py --rules=$IDF_PATH/tools/ci/astyle-rules.yml <path-to-file>
```

### Type Definitions

- Should be `snake_case` with `_t` suffix

```c
typedef int signed_32_bit_t;
```

### Enums

- Should be defined through `typedef` and namespaced

```c
typedef enum
{
    MODULE_FOO_ONE,
    MODULE_FOO_TWO,
    MODULE_FOO_THREE
} module_foo_t;
```

### Assertions

- Use standard `assert()` from `<assert.h>` for conditions that should always be true
- Assert for **unrecoverable errors** (internal logic bugs, corruption)
- Return error values for **recoverable errors** (invalid external input)

> When asserting that `esp_err_t` equals `ESP_OK`, use the **Macro For Unrecoverable Errors** instead of `assert()`.

**Important considerations:**

1. Functions called in `assert()` should not have side-effects (assertions can be disabled)
2. Avoid "variable set but not used" warnings when assertions are optimized out

```c
// Strategy 1: Cast to void
int res = do_something();
assert(res == 0);
(void)res;

// Strategy 2: Use __attribute__((unused))
int res __attribute__((unused));
res = do_something();
assert(res == 0);
res = do_something_else();
assert(res != 0);
```

### Header File Guards

**Preferred:** `#pragma once`

```c
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* declarations go here */

#ifdef __cplusplus
}
#endif
```

**Alternative pattern:**
```c
#ifndef FILE_NAME_H
#define FILE_NAME_H
...
#endif // FILE_NAME_H
```

**Order:** `#pragma once` → `#include` statements → `extern "C"` guards

### Include Statement Order

1. C standard library headers
2. Other POSIX standard headers (`sys/queue.h`)
3. Common IDF headers (`esp_log.h`, `esp_system.h`, `esp_timer.h`, `esp_sleep.h`, etc.)
4. Headers of other components (FreeRTOS, etc.)
5. Public headers of the current component
6. Private headers

**Syntax:**
- Angle brackets `<>` for C standard library and POSIX headers
- Double quotes `""` for all other headers

---

## C++ Code Formatting

Same rules as C apply. Additional rules below.

### File Naming

| File Type | Extension |
|-----------|-----------|
| Header files | `.hpp` |
| Source files | `.cpp` |

### Naming Conventions

| Element | Convention | Exception |
|---------|------------|-----------|
| Class/struct names | `CamelCase` with capital letter | `GPIO_Output` (underscore allowed for readability) |
| Member variables/methods | `snake_case` | |
| Namespaces | `lower_snake_case` | |
| Templates | Specified on line above function declaration | |
| Interfaces | Without `...Interface` suffix | |

### Member Order in Classes

**Precedence order:**

1. **Visibility:** `public` → `protected` → `private`
2. **Within each section:** Constructors/destructors → Member functions → Member variables

```cpp
class ForExample {
public:
    // first constructors, then default constructor, then destructor
    ForExample(double example_factor_arg);
    ForExample();
    ~ForExample();

    // then remaining public methods
    set_example_factor(double example_factor_arg);

    // then public member variables
    uint32_t public_data_member;

private:
    // first private methods
    void internal_method();

    // then private member variables
    double example_factor;
};
```

### Spacing

- **Do not indent** inside namespaces
- Put `public`, `protected`, `private` labels at same indentation level as `class` label

### Complete Example

```cpp
// file spaceship.h
#ifndef SPACESHIP_H_
#define SPACESHIP_H_
#include <cstdlib>

namespace spaceships {

class SpaceShip {
public:
    SpaceShip(size_t crew);
    size_t get_crew_size() const;

private:
    const size_t crew;
};

class SpaceShuttle : public SpaceShip {
public:
    SpaceShuttle();
};

class Sojuz : public SpaceShip {
public:
    Sojuz();
};

template <typename T>
class CargoShip {
public:
    CargoShip(const T &cargo);

private:
    T cargo;
};

} // namespace spaceships

#endif // SPACESHIP_H_
```

```cpp
// file spaceship.cpp
#include "spaceship.h"

namespace spaceships {

// Curly braces on same line OK for constructors with initializer list only
SpaceShip::SpaceShip(size_t crew) : crew(crew) { }

size_t SpaceShip::get_crew_size() const
{
    return crew;
}

SpaceShuttle::SpaceShuttle() : SpaceShip(7)
{
    // doing further initialization
}

Sojuz::Sojuz() : SpaceShip(3)
{
    // doing further initialization
}

template <typename T>
CargoShip<T>::CargoShip(const T &cargo) : cargo(cargo) { }

} // namespace spaceships
```

---
