package com.sequoiadb.test.schema;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBSchema;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import org.bson.BasicBSONObject;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class DBSchemaTest {
    private static Sequoiadb sdb;
    private static DBSchema dbSchema;
    private static String schemaName = "schema_02";

    @BeforeClass
    public static void init() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        BasicBSONObject col = new BasicBSONObject();
        dbSchema = sdb.createSchema(schemaName, col);
    }

    @AfterClass
    public static void destroy() {
        DBCursor dbCursor = sdb.listSchema(null, null, null, null);
        while (dbCursor.hasNext()) {
            System.out.println(dbCursor.getNext());
        }
        sdb.dropSchema(schemaName);
    }

    @Test
    public void test() {
        addColumn();
        alterColumn();
        renameColumn();
        dropColumnDefault();
        dropColumn();
        alter();
    }

    public void addColumn() {
        BasicBSONObject colDefine = new BasicBSONObject();
        colDefine.put("ReadDefault", "read");
        colDefine.put("WriteDefault", "write");
        colDefine.put("Type", "string");
        dbSchema.addColumn("filed",  colDefine);
    }

    public void alterColumn() {
        BasicBSONObject alterCol = new BasicBSONObject();
        alterCol.put("WriteDefault", "alterWrite");
        dbSchema.alterColumn("filed", alterCol);
    }

    public void renameColumn() {
        dbSchema.renameColumn("filed", "filedRename");
    }

    public void dropColumnDefault() {
        dbSchema.dropColumnDefault("filedRename");
    }

    public void dropColumn() {
        dbSchema.dropColumn("filedRename");
    }

    public void alter() {
        BasicBSONObject alter = new BasicBSONObject();
        alter.put("StrictMode", true);
        dbSchema.alter(alter);
    }
}
