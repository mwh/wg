// amb takes a lineup, range, or other iterable, and "returns" each value
method amb(many) {
    shift { k ->
        many.do(k)
    }
}

method pythag {
    reset {
        def a = amb [2, 3, 4, 5]
        def b = amb (2..5)
        def c = amb [3, 5, 7]
        print "Trying a={a}, b={b}, c={c}"
        if (((a * a) + (b * b)) == (c * c)) then {
            print "Found solution, will not evaluate remaining candidates."
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
//: Trying a=2, b=2, c=3
//: Trying a=2, b=2, c=5
//: Trying a=2, b=2, c=7
//: Trying a=2, b=3, c=3
//: Trying a=2, b=3, c=5
//: Trying a=2, b=3, c=7
//: Trying a=2, b=4, c=3
//: Trying a=2, b=4, c=5
//: Trying a=2, b=4, c=7
//: Trying a=2, b=5, c=3
//: Trying a=2, b=5, c=5
//: Trying a=2, b=5, c=7
//: Trying a=3, b=2, c=3
//: Trying a=3, b=2, c=5
//: Trying a=3, b=2, c=7
//: Trying a=3, b=3, c=3
//: Trying a=3, b=3, c=5
//: Trying a=3, b=3, c=7
//: Trying a=3, b=4, c=3
//: Trying a=3, b=4, c=5
//: Found solution, will not evaluate remaining candidates.
//: Solution is 3^2 + 4^2 = 5^2
