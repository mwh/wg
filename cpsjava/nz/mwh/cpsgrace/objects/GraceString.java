package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GraceString implements GraceObject {
    private String value;

    public GraceString(String val) {
        this.value = val;
    }
    
    public String toString() {
        return value;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args) {
        switch (methodName) {
            case "asString":
                return new PendingStep(ctx, returnCont, this);
            case "++(1)":
                GraceObject arg0 = args.get(0);
                return arg0.requestMethod(ctx, (GraceObject obj) -> {
                    String concatValue = this.value + obj.toString();
                    return returnCont.returning(ctx, new GraceString(concatValue));
                }, "asString", List.of());
            case "size":
                int size = this.value.length();
                return returnCont.returning(ctx, new GraceNumber(size));
            case "at(1)":
                GraceNumber indexNum = GraceNumber.assertNumber(args.get(0));
                char ch = this.value.charAt(indexNum.intValue() - 1);
                return returnCont.returning(ctx, new GraceString("" + ch));
            case "firstCodepoint":
            case "firstCP":
                int codepoint = this.value.codePointAt(0);
                return returnCont.returning(ctx, new GraceNumber(codepoint));
            case "==(1)":
                GraceString eqStr = assertString(args.get(0));
                boolean eqResult = this.value.equals(eqStr.value);
                return returnCont.returning(ctx, GraceBoolean.of(eqResult));
            case "!=(1)":
                GraceString neqStr = assertString(args.get(0));
                boolean neqResult = this.value.equals(neqStr.value);
                return returnCont.returning(ctx, GraceBoolean.of(!neqResult));
            case "<(1)":
                GraceString ltStr = assertString(args.get(0));
                boolean ltResult = this.value.compareTo(ltStr.value) < 0;
                return returnCont.returning(ctx, GraceBoolean.of(ltResult));
            case ">(1)":
                GraceString gtStr = assertString(args.get(0));
                boolean gtResult = this.value.compareTo(gtStr.value) > 0;
                return returnCont.returning(ctx, GraceBoolean.of(gtResult));
            case "replace(1)with(1)":
                GraceString oldStr = assertString(args.get(0));
                GraceString replacementStr = assertString(args.get(1));
                String replaced = this.value.replace(oldStr.value, replacementStr.value);
                return returnCont.returning(ctx, new GraceString(replaced));
            case "substringFrom(1)to(1)":
                GraceNumber startNum = GraceNumber.assertNumber(args.get(0));
                GraceNumber endNum = GraceNumber.assertNumber(args.get(1));
                String substring = this.value.substring(startNum.intValue() - 1, endNum.intValue());
                return returnCont.returning(ctx, new GraceString(substring));
            case "concise":
                return returnCont.returning(ctx, this);
            default:
                System.out.println("no such method " + methodName + " on String");
                return new PendingStep(ctx, returnCont, null);
        }
    }

    public static GraceString assertString(GraceObject obj) {
        if (obj instanceof GraceString str) {
            return str;
        } else {
            throw new RuntimeException("Expected a String, got: " + obj);
        }
    }
}

/*
 * } else if (name.equals("==")) {
                return new GraceBoolean(value.equals(((GraceString) parts.get(0).getArgs().get(0)).value));
            } else if (name.equals("!=")) {
                return new GraceBoolean(!value.equals(((GraceString) parts.get(0).getArgs().get(0)).value));
            } else if (name.equals("<")) {
                return new GraceBoolean(value.compareTo(((GraceString) parts.get(0).getArgs().get(0)).value) < 0);
            } else if (name.equals(">")) {
                return new GraceBoolean(value.compareTo(((GraceString) parts.get(0).getArgs().get(0)).value) > 0);
            } else if (name.equals("at")) {
                int index = (int) ((GraceNumber) parts.get(0).getArgs().get(0)).value;
                return new GraceString("" + value.charAt(index - 1));
            } else if (name.equals("firstCodepoint") || name.equals("firstCP")) {
                return new GraceNumber(value.codePointAt(0));
            } else if (name.equals("|")) {
                return new GracePatternOr(this, parts.get(0).getArgs().get(0));
            } else if (name.equals("match")) {
                GraceObject target = parts.get(0).getArgs().get(0);
                if (target instanceof GraceString str && value.equals(str.value)) {
                    return new GraceMatchResult(true, this);
                }
                return new GraceMatchResult(false, target);
            }
        } else if (parts.size() == 2) {
            String name = request.getName();
            if (name.equals("replace(1)with(1)")) {
                String old = ((GraceString) parts.get(0).getArgs().get(0)).value;
                String replacement = ((GraceString) parts.get(1).getArgs().get(0)).value;
                return new GraceString(value.replace(old, replacement));
            } else if (name.equals("substringFrom(1)to(1)")) {
                int start = (int) ((GraceNumber) parts.get(0).getArgs().get(0)).value;
                int end = (int) ((GraceNumber) parts.get(1).getArgs().get(0)).value;
                return new GraceString(value.substring(start - 1, end));
            }
 * 
 */
