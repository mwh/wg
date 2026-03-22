method foo {
    object {
        method greet(n) { print "hello {n}" }
    }
}

def bar = object {
    inherit foo
}

bar.greet "world" //: hello world
