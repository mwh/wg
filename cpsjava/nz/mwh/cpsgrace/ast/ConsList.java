package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

public class ConsList<T> extends ASTNode {
    private List<T> items = new ArrayList<>();

    @SuppressWarnings("rawtypes")
    private static ConsList NIL_SINGLETON = new ConsList();

    @SuppressWarnings("unchecked")
    public static <T> ConsList<T> nil() {
        return NIL_SINGLETON;
    }

    public T single() {
        if (items.size() == 0) {
            return null;
        }
        return items.get(items.size() - 1);
    }

    public static <T> ConsList<T> one(T value) {
        ConsList<T> ret = new ConsList<>();
        ret.items.add(value);
        return ret;
    }

    public static <T> ConsList<T> cons(T value, ConsList<T> list) {
        if (list == NIL_SINGLETON) list = new ConsList<T>();
        list.items.add(value);
        return list;
    }

    public List<T> asList() {
        return items.reversed();
    }
}
