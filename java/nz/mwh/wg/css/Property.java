package nz.mwh.wg.css;

import java.util.List;
import java.util.Map;

public class Property {
    private String name;
    private List<Value> values;

    public Property(String name, List<Value> values) {
        this.name = name;
        this.values = values;
    }

    public String getName() {
        return name;
    }

    public List<Value> getValues() {
        return values;
    }

    public String getValue(Map<String, Object> attributes) {
        return String.join("", values.stream().map(v -> {
            if (v instanceof Keyword) {
                return ((Keyword) v).getKeyword();
            } else if (v instanceof Function) {
                return ((Function) v).compute(attributes).toString();
            } else if (v instanceof StringLiteral) {
                return ((StringLiteral) v).getStringLiteral();
            } else if (v instanceof Number) {
                return Double.toString(((Number) v).getNumber());
            } else {
                return "";
            }
        }).toArray(String[]::new));
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(name);
        sb.append(": ");
        for (Value value : values) {
            sb.append(value.toString());
        }
        return sb.toString();
    }

    static class Value {

        public String stringValue() {
            return toString();
        }
    }

    static class Keyword extends Value {
        private String keyword;
        
        public Keyword(String keyword) {
            this.keyword = keyword;
        }
        
        public String getKeyword() {
            return keyword;
        }
        
        public String toString() {
            return keyword;
        }
    }

    static class Function extends Value {
        private String name;
        private List<Value> arguments;
        
        public Function(String name, List<Value> arguments) {
            this.name = name;
            this.arguments = arguments;
        }
        
        public String getName() {
            return name;
        }

        public Value compute(Map<String, Object> attributes) {
            switch (name) {
                case "attr":
                    return new RawString(attributes.get(arguments.get(0).stringValue()).toString());
                default:
                    return new StringLiteral("");
            }
        }
        
        public List<Value> getArguments() {
            return arguments;
        }
        
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append(name);
            sb.append("(");
            sb.append(String.join(", ", arguments.stream().map(Value::toString).toArray(String[]::new)));
            sb.append(")");
            return sb.toString();
        }
    }

    static class StringLiteral extends Value {
        private String stringLiteral;
        
        public StringLiteral(Object o) {
            if (o instanceof Value)
                this.stringLiteral = ((Value) o).stringValue();
            else
                this.stringLiteral = o.toString();
        }

        public StringLiteral(String stringLiteral) {
            this.stringLiteral = stringLiteral;
        }
        
        public String getStringLiteral() {
            return stringLiteral;
        }

        public String stringValue() {
            return stringLiteral;
        }
        
        public String toString() {
            return "\"" + stringLiteral + "\"";
        }
    }

    static class Number extends Value {
        private double number;
        
        public Number(double number) {
            this.number = number;
        }
        
        public double getNumber() {
            return number;
        }
        
        public String toString() {
            return Double.toString(number);
        }
    }

    private static class RawString extends Value {
        private String rawString;
        
        public RawString(String rawString) {
            this.rawString = rawString;
        }
        
        public String toString() {
            return rawString;
        }
    }
}
