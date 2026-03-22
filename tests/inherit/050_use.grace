def mixin1 = object {
    def x = 1
}

def obj = object {
    use mixin1
}

print(obj.x) //: 1
