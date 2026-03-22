var next := done
reset {
    shift { k -> next := k }
    print(1)
    shift { k -> next := k }
    print(2)
    shift { k -> next := k }
    print(3)
}

print "A"         //: A
next.apply(done)  //: 1
print "B"         //: B
next.apply(done)  //: 2
print "C"         //: C
next.apply(done)  //: 3
