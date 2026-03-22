class foo(grt) {
    def greeting = grt
    method greet(name) {
        print "{greeting}, {name}!"
    }
}

foo "hello".greet "world" //: hello, world!
