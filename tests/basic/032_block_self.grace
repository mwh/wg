method foo {
    print "no"
}
var blk
object {
    blk := {
        foo
        self.foo
    }
    method foo {
        print "yes"
    }
}

blk.apply
//: yes
//: yes
