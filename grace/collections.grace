
def endList = object {
    method asString {
        "endList"
    }

}

class listNode(p, n, v) {
    var next is public := n
    var prev is public := p
    var value is public := v

    method asString {
        "listNode with value: " ++ value.asString
    }
}

class list {
    var size is public := 0
    var firstItem := endList
    var lastItem := firstItem

    method append(item) {
        if (size == 0) then {
            firstItem := listNode(endList, endList, item)
            lastItem := firstItem
        } else {
            lastItem.next := listNode(lastItem, endList, item)
            lastItem := lastItem.next
        }
        size := size + 1
    }

    method prepend(item) {
        if (size == 0) then {
            firstItem := listNode(endList, endList, item)
            lastItem := firstItem
        } else {
            firstItem.prev := listNode(endList, firstItem.prev, item)
            firstItem.prev.next := firstItem
            firstItem := firstItem.prev
        }
        size := size + 1
    }

    method at(index) {
        if ((index < 1) || (index > size)) then {
            Exception.raise("Index out of bounds: " ++ index)
        }
        var current := firstItem
        var i := 1
        while {i < index} do {
            current := current.next
            i := i + 1
        }
        current.value
    }

    method each(block) {
        var current := firstItem
        while {current != endList} do {
            block.apply(current.value)
            current := current.next
        }
    }

    method map(transform) {
        var result := list
        var current := firstItem
        while {current != endList} do {
            result.append(transform.apply(current.value))
            current := current.next
        }
        result
    }

    method map(transform) combine(combiner) {
        var result := transform.apply(firstItem.value)
        var current := firstItem.next
        while {current != endList} do {
            result := combiner.apply(result, transform.apply(current.value))
            current := current.next
        }
        result
    }

    method flatMap(transform) {
        var result := list
        var current := firstItem
        while {current != endList} do {
            var transformed := transform.apply(current.value)
            transformed.each { item ->
                result.append(item)
            }
            current := current.next
        }
        result
    }

    method zip(otherList) each(block) {
        if (size != otherList.size) then {
            Exception.raise("Cannot zip lists of different sizes: " ++ size ++ " and " ++ otherList.size)
        }
        var current1 := firstItem
        var current2 := otherList.firstItem
        while {current1 != endList} do {
            block.apply(current1.value, current2.value)
            current1 := current1.next
            current2 := current2.next
        }
    }
}

class keyValuePair(k, v) {
    var key := k
    var value := v

    method asString {
        "keyValuePair(" ++ key ++ ", " ++ value ++ ")"
    }
}

class dictionary {
    var size is public := 0
    var items := list

    method at(key) {
        items.each { item ->
            if (item.key == key) then {
                return item.value
            }
        }
        Exception.raise("Key not found: " ++ key)
    }

    method at(key) put(value) {
        items.each { item ->
            if (item.key == key) then {
                item.value := value
                return done
            }
        }
        items.append(keyValuePair(key, value))
        size := size + 1
    }

    method has(key) {
        items.each { item ->
            if (item.key == key) then {
                return true
            }
        }
        return false
    }

}
