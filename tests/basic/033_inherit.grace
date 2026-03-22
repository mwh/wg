class foo {
    method greet(n) { print "hello {n}" }
}

class bar {
    inherit foo
}

bar.greet "world" //: hello world
