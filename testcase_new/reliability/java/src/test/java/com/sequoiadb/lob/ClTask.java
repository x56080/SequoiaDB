package com.sequoiadb.lob;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.OperateTask;
import org.bson.BasicBSONObject;
import org.testng.Assert;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 */
public abstract class ClTask extends OperateTask {
    /**
     * 在
     *
     * @param num
     * @return
     */
    public static ClTask getClTask( final int num ) {
        return new ClTask() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = MyUtil.getSdb();
                    String csName = LobUtil.csName;
                    String clName = LobUtil.clName;
                    db.beginTransaction();
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    for ( int i = 0; i < num; i++ ) {
                        cl.insert( new BasicBSONObject( "a", i ) );
                        cl.delete( new BasicBSONObject() );
                    }
                    for ( int i = 0; i < num; i++ ) {
                        cl.insert( new BasicBSONObject( "a", i ) );
                    }
                    db.commit();
                } catch ( BaseException e ) {
                    Assert.fail( e.getMessage() );
                } finally {
                    if ( db != null ) {
                        db.disconnect();
                    }
                }
            }
        };
    }
}
