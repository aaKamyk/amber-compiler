define void @main() {
main:
    %x.0 = alloca i32
    store i32 12, ptr %x.0
    %x.0.load0 = load i32, ptr %x.0
    %__t0 = icmp ne i32 %x.0.load0, 0
    br i1 %__t0, label %__case_true0, label %__case_false0

__case_true0:
    %x.0.load1 = load i32, ptr %x.0
    ret i32 %x.0.load1

__case_false0:
    %x.0.load2 = load i32, ptr %x.0
    %__v0 = mul i32 2, %x.0.load2
    ret i32 %__v0

__if_end0:
    ret i32 0
}

