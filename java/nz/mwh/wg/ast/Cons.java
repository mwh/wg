package nz.mwh.wg.ast;

import java.util.List;
import java.util.ArrayList;

public class Cons<T> {
    public T head;
    public Cons<T> tail;
    public Cons(T head, Cons<T> tail) {
        this.head = head;
        this.tail = tail;
    }

    public static <T> List<T> toList(Cons<? extends T> cons) {
        List<T> list = new ArrayList<>();
        while (cons != null) {
            list.add(cons.head);
            cons = cons.tail;
        }
        return list;
    }

    public List<T> toList() {
        List<T> ret = new ArrayList<T>();
        Cons<T> current = this;
        while (current != null) {
            ret.add(current.head);
            current = current.tail;
        }
        return ret;
    }

    public String toString() {
        return "cons(" + head + ", " + (tail == null ? "nil" : tail.toString()) + ")";
    }

}

