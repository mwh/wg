// Reset with no shift
def a = reset { 42 }
print(a) //: 42

// Shift that immediately returns without using k
def b = reset { 1 + shift { k -> 100 } }
print(b) //: 100

// Shift that uses k once
def c = reset { 1 + shift { k -> k.apply(10) } }
print(c) //: 11

// Shift in a larger expression
def d = reset {
    def x = shift { k -> k.apply(5) }
    x * 3
}
print(d) //: 15
