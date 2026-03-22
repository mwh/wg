class foo {
    print(name)
    def sign = "!"
}

class bar {
    inherit foo
    print "world"
    print(sign)
    method name { "hello" }
}

bar
//: hello
//: world
//: !
