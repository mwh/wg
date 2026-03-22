def mixin1 = object {
    var x := 1
}

def obj = object {
    use mixin1 // Expected: failure
}

print(obj.x) // Unreachable, so no output.
