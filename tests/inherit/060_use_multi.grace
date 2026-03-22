def mixin1 = object {
    method m1 { print "mixin1" }
    method asString { "m1 object" }
}

def mixin3 = object {
    def m3 = "mixin3"
    method asString { "m3 object" }
}

def mixin2 = object {
    use mixin3
    method changedVal { 99 }
    method mult {
        5 * changedVal
    }
    method asString { "m2 object" }
    method stringify { "{self}" }
    def stringified = "{self.asString}"
}

def obj = object {
    use mixin1
    use mixin2
    method changedVal { 7 }
    method asString { "bottom object" }
}

obj.m1 //: mixin1
print(obj.m3) //: mixin3
print(obj.mult) //: 35
print(obj.stringify) //: bottom object
print(obj.stringified) //: m2 object
