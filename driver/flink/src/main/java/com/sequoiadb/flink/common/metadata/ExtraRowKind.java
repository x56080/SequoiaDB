package com.sequoiadb.flink.common.metadata;

import com.sequoiadb.flink.common.exception.SDBException;

public enum ExtraRowKind {
    INSERT(0),
    UPDATE_AFT(1),
    DELETE(2),
    UPDATE_PK_BEF(3),
    UPDATE_PK_AFT(4),

    ;

    private final int code;

    ExtraRowKind(int code) {
        this.code = code;
    }

    public int getCode() {
        return code;
    }

    public static ExtraRowKind from(Integer code) {
        ExtraRowKind kind = null;
        switch (code) {
            case 0:
                kind = INSERT;
                break;
            case 1:
                kind = UPDATE_AFT;
                break;
            case 2:
                kind = DELETE;
                break;
            case 3:
                kind = UPDATE_PK_BEF;
                break;
            case 4:
                kind = UPDATE_PK_AFT;
                break;

            default:
                throw new SDBException(String.format(
                        "unsupported extra row kind, code: %s",
                        code));
        }

        return kind;
    }
}
