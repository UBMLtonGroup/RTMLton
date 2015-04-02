# Object Manipulation in MLton
## Object Allocation
2 types of object allocation:

- Weak object, Array are allocated in C calls
- Normal objects are allocated by increasing the frontier (c-codegen)

## Object Read

Objects are read from Select operation. Here "Select" refers to only
tuples in SSA but becomes an overloaded term in SSA2 and below.

## Object Write
2 types of MLton native object writes:

- Arrays and refs are converted to "Update" operation in SSA2.

# Potential Implementation Strategy
- Due to the overloadded nature of "Select" operation, we need to
  pattern matching over its type to determine which type object it is
  writing. We for now first focus on normal objects and let arrays and
  refs fall through to packed-representation. We can refer to the
  "getSelects" function defined in packed-representation. If we are going
  to implement a new object type "chunked" object, we can directly determine
  the object size in ssa-to-rssa and map it to object selection in C code. And
  we let other cases fall through.
- Object writes on normal objects can be tackled by adding additional
  calculation on the offsets of objects before the move operation. But in this way
  it means we need to understand the SSA2 IR. Not sure if we can map this directly
  to C primitive calls atm...
