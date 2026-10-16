define i32 @foo(i32 %n.0) {
foo:
    %n.0.addr = alloca i32
    store i32 %n.0, ptr %n.0.addr
    %sum.0 = alloca i32
    store i32 0, ptr %sum.0
    br label %__while_cond0

__while_body0:
    %sum.0.load0 = load i32, ptr %sum.0
    %n.0.load0 = load i32, ptr %n.0.addr
    %__t0 = add i32 %sum.0.load0, %n.0.load0
    store i32 %__t0, ptr %sum.0
    %n.0.load1 = load i32, ptr %n.0.addr
    %__t1 = sub i32 %n.0.load1, 1
    store i32 %__t1, ptr %n.0.addr
    br label %__while_cond0

__while_cond0:
    %n.0.load2 = load i32, ptr %n.0.addr
    %__v0 = add i32 %n.0.load2, 2
    %__t2 = icmp sgt i32 %__v0, 0
    br i1 %__t2, label %__while_body0, label %__while_end0

__while_end0:
    %sum.0.load1 = load i32, ptr %sum.0
    ret i32 %sum.0.load1
}

define i32 @main() {
main:
    %x.0 = alloca i32
    store i32 5, ptr %x.0
    %x.0.load0 = load i32, ptr %x.0
    %__v1 = call i32 @foo(i32 %x.0.load0)
    ret i32 %__v1
}

define void @global_init() {
global_init:
    ret void
}

