# Doxygen Guide & Cheat Sheet for C/C++ Projects

A comprehensive reference for documenting **functions, structs, classes, enums, and files** using Doxygen comments in `.c`, `.cpp`, and `.h` files.

---

## 1. File Header Comment

```c
/**
 * @file filename.h
 * @brief Brief description of the file
 * @author Your Name
 * @date 2025-09-07
 * @version 1.0
 *
 * Detailed description of the file if necessary.
 */
```

---

## 2. Function Documentation

```c
/**
 * @brief Brief description of what the function does
 *
 * Detailed description of the function if needed.
 * Can span multiple lines.
 *
 * @param param1 Description of the first parameter
 * @param param2 Description of the second parameter
 * @return Description of the return value
 * @note Any important notes
 * @warning Any warnings
 * @see RelatedFunction()
 */
ReturnType FunctionName(Type1 param1, Type2 param2);
```

**Example:**

```c
/**
 * @brief Adds two integers
 *
 * Performs addition of two integers and returns the result.
 *
 * @param a First integer
 * @param b Second integer
 * @return Sum of a and b
 * @note This function does not check for overflow
 */
int add(int a, int b);
```

---

## 3. Struct Documentation

```c
/**
 * @struct MyStruct
 * @brief Brief description of the struct
 *
 * Detailed description of the struct, its purpose and usage.
 */
typedef struct MyStruct {
    int member1; /**< Description of member1 */
    float member2; /**< Description of member2 */
} MyStruct;
```

---

## 4. Class Documentation (C++)

```cpp
/**
 * @class MyClass
 * @brief Brief description of the class
 *
 * Detailed description of the class, its purpose and usage.
 */
class MyClass {
public:
    /**
     * @brief Constructor for MyClass
     */
    MyClass();

    /**
     * @brief Example method
     * @param value Description of value
     * @return Description of return value
     */
    int exampleMethod(int value);

private:
    int data; /**< Description of private data member */
};
```

---

## 5. Enum Documentation

```c
/**
 * @enum MyEnum
 * @brief Brief description of the enum
 *
 * Detailed description of the enum if necessary.
 */
typedef enum {
    VALUE_ONE, /**< Description of VALUE_ONE */
    VALUE_TWO, /**< Description of VALUE_TWO */
    VALUE_THREE /**< Description of VALUE_THREE */
} MyEnum;
```

---

## 6. Macro Documentation

```c
/**
 * @def MAX_BUFFER
 * @brief Maximum buffer size
 *
 * Detailed description of the macro.
 */
#define MAX_BUFFER 1024
```

---

## 7. Full General Tags Cheat Sheet

| Tag             | Purpose / Usage |
|-----------------|----------------|
| `@brief`        | Short description of function, class, struct, file, or macro. |
| `@param`        | Description of a function parameter. Specify parameter name and details. |
| `@return`       | Description of the return value of a function. |
| `@retval`       | Specific return values (useful for multiple return codes). |
| `@file`         | File-level description. Typically placed at the top of a file. |
| `@author`       | Name of the author(s) of the code/file. |
| `@version`      | Version number of the file or function. |
| `@date`         | Date of creation or last modification. |
| `@note`         | Important notes about the function or file. |
| `@warning`      | Warnings about using the function or struct. |
| `@see`          | References to related functions, classes, or files. |
| `@class`        | Class description (C++ only). Place before class definition. |
| `@struct`       | Struct description. Place before struct definition. |
| `@enum`         | Enum description. Place before enum definition. |
| `@def`          | Document a macro or #define. |
| `@defgroup`     | Define a group for related functions, classes, files, or modules. |
| `@ingroup`      | Assign an item (function, class, file) to an existing group. |
| `@page`         | Create a new page in documentation. Useful for long descriptions or wiki-like pages. |
| `@section`      | Section within a page or file. Allows dividing documentation into subsections. |
| `@subsection`   | Subsection within a section. |
| `@addtogroup`   | Add items to an existing group defined elsewhere. |
| `@internal`     | Mark documentation as internal; excluded from user docs. |
| `@private`      | Mark private members/functions (usually used in classes). |
| `@protected`    | Mark protected members/functions in C++ classes. |
| `@deprecated`   | Mark function, class, or file as deprecated. |
| `@test`         | Document test functions or examples. |
| `@example`      | Reference an example code file. |
| `@anchor`       | Set an anchor point for hyperlinks within documentation. |
| `@htmlonly` / `@endhtmlonly` | Include raw HTML in documentation. |
| `@verbatim` / `@endverbatim` | Include code or text literally without formatting. |

---

## 8. Multi-line Examples

```cpp
/**
 * @brief Processes input data
 *
 * This function processes the input data in multiple steps:
 * 1. Validates the input
 * 2. Performs calculation
 * 3. Returns the result
 *
 * @param input Pointer to input data
 * @param length Length of the data
 * @return Processed result
 */
int processData(const char* input, size_t length);
```
