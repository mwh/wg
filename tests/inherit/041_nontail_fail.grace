method foo {
    if (true) then {
        return object {
            method val { "hello" }
        }
    } else {
        object {
            method val { "world" }
        }
    }
}

object {
    inherit foo
}
// Expected result: failure
