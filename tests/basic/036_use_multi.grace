def trt1 = object {
    method foo { print "hello" }
}

def trt2 = object {
    method bar { print "world" }
}

def obj = object {
    use trt1
    use trt2
}
obj.foo //: hello
obj.bar //: world
