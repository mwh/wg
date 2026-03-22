method foo {
    def x = object { }
    x
}

object {
    inherit foo
}
// Expected result: failure
