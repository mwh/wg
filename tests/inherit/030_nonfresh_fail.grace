def obj = object {
    method foo { "hello" }
}

def obj2 = object {
    inherit obj
}
print(obj2.foo)
