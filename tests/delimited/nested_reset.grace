// Inner reset is independent of outer reset
def a = reset {
    def x = reset {
        shift { k -> k.apply(10) }
    }
    x + 1
}
print(a) //: 11

// Shift in outer reset, no shift in inner reset
def b = reset {
    def x = shift { k -> k.apply(10) }
    def y = reset { x + 1 }
    y * 2
}
print(b) //: 22

// Nested shifts, each capturing to its own reset
def c = reset {
    def x = shift { k -> k.apply(3) }
    reset {
        def y = shift { inner_k -> inner_k.apply(x * 10) }
        y + 3
    }
}
print(c) //: 33
