method foo {
    object {
        method init { "uppermost" }
        method top { print "top only" }
        method topmid { print "at top?" }
        def val = init
    }
}

class bar {
    inherit foo
    method init { "middle" }
    method topmid { print "in middle." }
}

def r = object {
    inherit bar
    method init { "bottommost" }
}

print(r.val) //: bottommost
r.top        //: top only
r.topmid     //: in middle.
