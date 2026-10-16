define i32 @foo(i32 %x.0) {
foo:
    %x.0.addr = alloca i32
    store i32 %x.0, ptr %x.0.addr
    %x.0.load0 = load i32, ptr %x.0.addr
    ret i32 %x.0.load0
}

define void @main() {
main:
    %x.1 = alloca i32
    store i32 12, ptr %x.1
    %x.1.load0 = load i32, ptr %x.1
    call i32 @foo(i32 %x.1.load0)
    ret void
}

