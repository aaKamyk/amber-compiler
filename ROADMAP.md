### Compiler goals

- Start lowering into llvm
- A proper logging / error system
- Add function signatures to symbol table
- Fix lexer issue with long identifiers

### Language features

- Modules

1. `mod` keyword
2. Enforce declaration only at the top of the file
3. Implicitly generate a module if no declaration is provided \
   If declared generate / append to a specified module
4. `pub` keyword for member visibility - private by default
5. Declaring: `mod example_module;` / `mod outer::inner;` \
   Importing: `use example_module::{*};` / `use example_module::{foo};`

- Custom types:

1. Structs
    1. Declaration: `struct A { int field_a; int field_b; };`
    2. Methods `impl` keyword
    3. Usage:

```
impl A { fn foo() -> void {}
static fn bar() -> void {} }
```

2. Enums
    1. Tagged by default, option to make untagged
    2. Declaration: `enum A { X(int), Y(float), Z }`
    3. Implicitly generate a custom type that allows for interacting \
       interacting with the field names

- Generics:

1. Usage: `fn foo<T>(T obj) {}` / `struct bar<T> { T field; };`
2. Enforce properties: `constraint` keyword, \
   usage: `constraint foo { fn bar() -> void; }` \
   enforcing:`fn process<T>(T obj) where T: processable {}`

- Namespaces:

1. `namespace` keyword
2. Usage:

```
namespace A { fn foo() -> void {} }
namespace B { fn foo() -> void { A::foo(); }
```

- Compile time code

1. `comptime` keyword
2. Usage:

```
struct is_float<T> { pub comptime bool value = same_as<T, float>; };
struct is_int<T> { 
    pub comptime { 
        bool a = same_as<T, int>;
        bool b = size_of<T> == 16;
    }
};

fn process<T>(T val) 
    where is_float<T>::value {
}
```