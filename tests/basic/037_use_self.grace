def trt = object {
    method greet { print "hello {self.name}" }
}
class foo(n) {
    use trt
    def name = n
}

foo "world".greet //: hello world
