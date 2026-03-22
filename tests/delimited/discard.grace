def result = reset {
    def x = 10
    def y = shift { k -> "aborted" }
    // This code is never reached:
    print("unreachable")
    x + y
}
print(result) //: aborted
