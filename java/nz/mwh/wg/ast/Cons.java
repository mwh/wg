package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.runtime.GraceString;

import java.util.ArrayList;

public class Cons<T> {
    public T head;
    public Cons<T> tail;
    protected boolean isNil = false;

    public Cons(T head, Cons<T> tail) {
        this.head = head;
        this.tail = tail;
    }

    public Cons() {
        isNil = true;
    }

    public static <T> List<T> toList(Cons<? extends T> cons) {
        List<T> list = new ArrayList<>();
        while (cons != null && !cons.isNil) {
            list.add(cons.head);
            cons = cons.tail;
        }
        return list;
    }

    public List<T> toList() {
        List<T> ret = new ArrayList<T>();
        Cons<T> current = this;
        while (!current.isNil) {
            ret.add(current.head);
            current = current.tail;
        }
        return ret;
    }

    public String toString() {
        if (isNil)
            return "nil";
        return "cons(" + (head instanceof String ? "\"" + head + "\"" : head instanceof GraceString ? "\"" + head.toString() + "\"" : head) + ", " + (tail == null ? "nil" : tail.toString()) + ")";
    }

    public boolean isNil() {
        return isNil;
    }

    public T getHead() {
        return head;
    }

    public static <T> Cons<T> nil() {
        return new Cons<T>();
    }

}

