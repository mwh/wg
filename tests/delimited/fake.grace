// Implements fake shift-reset using blocks that return from the enclosing
// method after it's finished ("time travel"). Probably shouldn't work!
// Contains code from basic, coroutine, multishot, nested, and nondeterminism
// tests adjusted to use these imitations.
var resetPoint := { x -> x }
method rs(blk) {
    def origReset = resetPoint
    resetPoint := { val ->
        resetPoint := origReset
        return val
    }
    def v = blk.apply
    resetPoint.apply(v)
}

method sh(blk) {
    def cont = { val -> return val }
    def origReset = resetPoint
    def v = blk.apply(object {
        method apply(v) {
            resetPoint := { val -> return val }
            cont.apply(v)
        }
    })
    resetPoint := origReset
    origReset.apply(v)
}

// Reset with no shift
def a = rs { 42 }
print(a) //: 42

// Shift that immediately returns without using k
def b = rs { 1 + sh { k -> 100 } }
print(b) //: 100

// Shift that uses k once
def c = rs { 1 + sh { k -> k.apply(10) } }
print(c) //: 11

// Shift in a larger expression
def d = rs {
    def x = sh { k -> k.apply(5) }
    x * 3
}
print(d) //: 15


var next := done
rs {
    sh { k -> next := k }
    print(1)
    sh { k -> next := k }
    print(2)
    sh { k -> next := k }
    print(3)
}

print "A"         //: A
next.apply(done)  //: 1
print "B"         //: B
next.apply(done)  //: 2
print "C"         //: C
next.apply(done)  //: 3

// amb takes a lineup, range, or other iterable, and "returns" each value
method amb(many) {
    sh { k ->
        many.do(k)
    }
}

method pythag {
    rs {
        def a = amb [2, 3, 4, 5]
        def b = amb (2..5)
        def c = amb [3, 5, 7]
        if (((a * a) + (b * b)) == (c * c)) then {
            return object { // This return should terminate amb
                def l1 = a
                def l2 = b
                def l3 = c
            }
        }
    }
}
def sol = pythag
print "Solution is {sol.l1}^2 + {sol.l2}^2 = {sol.l3}^2"
//: Solution is 3^2 + 4^2 = 5^2

def result = rs {
    def x = sh { k ->
        k.apply(1) + k.apply(10) + k.apply(100)
    }
    x * 2
}
print(result) //: 222
// k.apply(1)   = 1*2   =   2
// k.apply(10)  = 10*2  =  20
// k.apply(100) = 100*2 = 200
// Total: 2 + 20 + 200  = 222


// Inner reset is independent of outer reset
def a = rs {
    def x = rs {
        sh { k -> k.apply(10) }
    }
    x + 1
}
print(a) //: 11

// Shift in outer reset, no shift in inner reset
def b = rs {
    def x = sh { k -> k.apply(10) }
    def y = rs { x + 1 }
    y * 2
}
print(b) //: 22

// Nested shifts, each capturing to its own reset
def c = rs {
    def x = sh { k -> k.apply(3) }
    rs {
        def y = sh { inner_k -> inner_k.apply(x * 10) }
        y + 3
    }
}
print(c) //: 33