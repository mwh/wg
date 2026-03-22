def b = reset {
    def x = shift { k ->
        k.apply(99)
    }
    try {
        if (x == 99) then {
            Exception.raise "from continuation"
        }
        x
    } catch { e ->
        "Exception: " ++ e.message
    }
}
print(b) //: Exception: from continuation
