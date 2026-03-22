def result = reset {
    def x = shift { k ->
        k.apply(1) + k.apply(10) + k.apply(100)
    }
    x * 2
}
print(result) //: 222
// k.apply(1)   = 1*2   =   2
// k.apply(10)  = 10*2  =  20
// k.apply(100) = 100*2 = 200
// Total: 2 + 20 + 200  = 222
