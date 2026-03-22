method foo {
    if (false) then {
        return 123
    } else {
        return 456
    }
}
print(foo) //: 456
