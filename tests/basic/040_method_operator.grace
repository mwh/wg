def obj = object {
    method +&(x) {
        "hello {x}"
    }
}

print(obj +& "world") //: hello world
