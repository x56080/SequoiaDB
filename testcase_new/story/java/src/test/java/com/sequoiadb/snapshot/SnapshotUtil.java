package com.sequoiadb.snapshot;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class SnapshotUtil extends SdbTestBase {

    // 插入记录数，快照用例公用此变量
    public static final int INSERT_NUMS = 1000;

    public static void insertData(DBCollection cl ) {
        insertData( cl, INSERT_NUMS );
    }

    public static void insertData(DBCollection cl, int recordNum ) {

        if ( recordNum < 1 ) {
            recordNum = 1;
        }
        for ( int i = 0; i < recordNum; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "a", i  );
            cl.insert( record );
        }
    }
}
