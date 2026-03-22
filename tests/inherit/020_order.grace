method foo {
    object {
        def val = "{a} {b}"
        method a { "up1" }
        method b { "up2" }
    }
}

class bar {
    inherit foo
    method b { "mid2" }
}

def obj = object {
    inherit bar
}

print(obj.val) //: up1 mid2
