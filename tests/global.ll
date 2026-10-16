define void @global_init() {
global_init:
    %__v0 = sub i32 0, 10
    %__v1 = mul i32 %__v0, 2
    %x.0 = alloca i32
    %__t0 = add i32 %__v1, 5
    store i32 %__t0, ptr %x.0
    %x.0.load0 = load i32, ptr %x.0
    %__v2 = mul i32 2, %x.0.load0
    %y.0 = alloca i32
    %__t1 = add i32 %__v2, 12
    store i32 %__t1, ptr %y.0
    %x.0.load1 = load i32, ptr %x.0
    %__v3 = sub i32 0, %x.0.load1
    %y.0.load0 = load i32, ptr %y.0
    %__v4 = sub i32 0, %y.0.load0
    %z.0 = alloca i32
    %__t2 = add i32 %__v3, %__v4
    store i32 %__t2, ptr %z.0
    ret void
}

