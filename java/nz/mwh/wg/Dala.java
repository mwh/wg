package nz.mwh.wg;

public class Dala {

    public static enum Flavour {
        UNSAFE, ISO, LOCAL, IMM
    }

    public static enum IsoWhen {
        ASSIGNMENT, DEREFERENCE, THREAD, DEREFERENCE_THREAD, NEVER
    }

    public static enum IsoMove {
        DESTRUCTIVE, MOVE, NEWEST, IMMOBILE
    }

    public static enum LocalWhen {
        DEREFERENCE, THREAD, DEREFERENCE_THREAD, NEVER
    }

    public static enum CapabilityWhere {
        STATIC, CONSTRUCTION, VARIABLE_SIDE
    }

    public static enum CapabilityChange {
        NEVER, DYNAMIC, DYNAMIC_ISO, DURATION
    }

    public static enum ImmWhen {
        CONSTRUCTION, MUTATION, NEVER
    }

    private static IsoWhen isoWhen = IsoWhen.ASSIGNMENT;
    private static IsoMove isoMove = IsoMove.DESTRUCTIVE;
    private static LocalWhen localWhen = LocalWhen.DEREFERENCE;
    private static CapabilityWhere capabilityWhere = CapabilityWhere.STATIC;
    private static CapabilityChange capabilityChange = CapabilityChange.NEVER;
    private static ImmWhen immWhen = ImmWhen.CONSTRUCTION;

    public static void setIsoWhen(IsoWhen isoWhen) {
        Dala.isoWhen = isoWhen;
    }

    public static void setIsoWhen(String isoWhen) {
        switch (isoWhen) {
            case "assignment":
                Dala.isoWhen = IsoWhen.ASSIGNMENT;
                break;
            case "dereference":
                Dala.isoWhen = IsoWhen.DEREFERENCE;
                break;
            case "thread":
                Dala.isoWhen = IsoWhen.THREAD;
                break;
            case "dereference-thread":
                Dala.isoWhen = IsoWhen.DEREFERENCE_THREAD;
                break;
            case "never":
                Dala.isoWhen = IsoWhen.NEVER;
                break;
            default:
                throw new IllegalArgumentException("Invalid iso-when: " + isoWhen);
        }
    }

    public static void setIsoMove(IsoMove isoMove) {
        Dala.isoMove = isoMove;
    }

    public static void setIsoMove(String isoMove) {
        switch (isoMove) {
            case "destructive-read":
                Dala.isoMove = IsoMove.DESTRUCTIVE;
                break;
            case "move":
                Dala.isoMove = IsoMove.MOVE;
                break;
            case "newest":
                Dala.isoMove = IsoMove.NEWEST;
                break;
            case "immobile":
                Dala.isoMove = IsoMove.IMMOBILE;
                break;
            default:
                throw new IllegalArgumentException("Invalid iso-move: " + isoMove);
        }
    }
    
    public static void setLocalWhen(LocalWhen localWhen) {
        Dala.localWhen = localWhen;
    }

    public static void setLocalWhen(String localWhen) {
        switch (localWhen) {
            case "dereference":
                Dala.localWhen = LocalWhen.DEREFERENCE;
                break;
            case "thread":
                Dala.localWhen = LocalWhen.THREAD;
                break;
            case "dereference-thread":
                Dala.localWhen = LocalWhen.DEREFERENCE_THREAD;
                break;
            case "never":
                Dala.localWhen = LocalWhen.NEVER;
                break;
            default:
                throw new IllegalArgumentException("Invalid local-when: " + localWhen);
        }
    }

    public static void setCapabilityWhere(CapabilityWhere capabilityWhere) {
        Dala.capabilityWhere = capabilityWhere;
    }

    public static void setCapabilityWhere(String capabilityWhere) {
        switch (capabilityWhere) {
            case "static":
                Dala.capabilityWhere = CapabilityWhere.STATIC;
                break;
            case "construction":
                Dala.capabilityWhere = CapabilityWhere.CONSTRUCTION;
                break;
            case "variable-side":
                Dala.capabilityWhere = CapabilityWhere.VARIABLE_SIDE;
                break;
            default:
                throw new IllegalArgumentException("Invalid capability-where: " + capabilityWhere);
        }
    }

    public static void setCapabilityChange(CapabilityChange capabilityChange) {
        Dala.capabilityChange = capabilityChange;
    }

    public static void setCapabilityChange(String capabilityChange) {
        switch (capabilityChange) {
            case "never":
                Dala.capabilityChange = CapabilityChange.NEVER;
                break;
            case "dynamic":
                Dala.capabilityChange = CapabilityChange.DYNAMIC;
                break;
            case "dynamic-iso":
                Dala.capabilityChange = CapabilityChange.DYNAMIC_ISO;
                break;
            case "duration":
                Dala.capabilityChange = CapabilityChange.DURATION;
                break;
            default:
                throw new IllegalArgumentException("Invalid capability-change: " + capabilityChange);
        }
    }

    public static void setImmWhen(ImmWhen immWhen) {
        Dala.immWhen = immWhen;
    }

    public static void setImmWhen(String immWhen) {
        switch (immWhen) {
            case "construction":
                Dala.immWhen = ImmWhen.CONSTRUCTION;
                break;
            case "mutation":
                Dala.immWhen = ImmWhen.MUTATION;
                break;
            case "never":
                Dala.immWhen = ImmWhen.NEVER;
                break;
            default:
                throw new IllegalArgumentException("Invalid imm-when: " + immWhen);
        }
    }

    public static IsoWhen getIsoWhen() {
        return isoWhen;
    }

    public static IsoMove getIsoMove() {
        return isoMove;
    }

    public static LocalWhen getLocalWhen() {
        return localWhen;
    }

    public static CapabilityWhere getCapabilityWhere() {
        return capabilityWhere;
    }

    public static CapabilityChange getCapabilityChange() {
        return capabilityChange;
    }

    public static ImmWhen getImmWhen() {
        return immWhen;
    }

}
