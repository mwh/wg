method foo {
    bar { x -> return x * 10 }
    123
}

method bar(blk) {
    blk.apply(-123)
}

print(foo) //: -1230
