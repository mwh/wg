def trt = object {
    method foo { print "yes" }
}

def obj = object {
    use trt
}

obj.foo //: yes
