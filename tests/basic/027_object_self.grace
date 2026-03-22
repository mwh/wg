method foo {
    print "no"
}
def obj = object {
    method foo {
        print "yes"
    }

    method bar {
        foo
        self.foo
    }
}
obj.bar
//: yes
//: yes
