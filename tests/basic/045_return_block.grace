method foo {
    def blk = { return 123 }
    blk.apply
    return 456
}
print(foo) //: 123
