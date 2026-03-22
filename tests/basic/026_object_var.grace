def obj = object {
    var x is public := 123
}
obj.x := obj.x + 111
print(obj.x) //: 234
