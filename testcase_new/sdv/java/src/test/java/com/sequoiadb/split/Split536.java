package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:SEQDB-536 目标分区组不存在:1.在cl下指定范围条件进行数据切分 2、执行split操作，其中设置的目标分区组不存在
 *                     3、查看数据切分是否成功
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split536 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection currentCL;
    private String clName = "testcaseCL536";

    @BeforeClass(enabled = true)
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( coordUrl, "", "" );
            // 跳过 standAlone
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }

            this.commCS = sdb.getCollectionSpace( csName );
            this.currentCL = commCS.createCollection( clName,
                    ( BSONObject ) JSON.parse( "{ShardingKey:{\"a\":1}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    // 执行split操作，其中设置的目标分区组不存在 ,查看数据切分是否成功
    @Test(enabled = true)
    public void test() {
        DBCursor dbc = null;
        try {
            dbc = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + currentCL.getFullName() + "\"}", null, null );
            BasicBSONList list = ( BasicBSONList ) dbc.getNext()
                    .get( "CataInfo" );
            String srcGroupName = ( String ) ( ( BSONObject ) list.get( 0 ) )
                    .get( "GroupName" );// 获取源数据组名
            String destGroupName = "thisDataGroupShouldNotExists_536";// 定义一个不存在组名
            Sequoiadb db = new Sequoiadb( coordUrl, "", "" );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );

            // 准备切分所需数据
            for ( int i = 0; i < 10; i++ ) {
                cl.insert( ( BSONObject ) JSON.parse( "{a:" + i + "}" ) );
            }
            // 切分
            cl.split( srcGroupName, destGroupName,
                    ( BSONObject ) JSON.parse( "{a:1}" ),
                    ( BSONObject ) JSON.parse( "{a:5}" ) );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -154, e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
            return;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
        Assert.fail();
    }

    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            if ( commCS != null ) {
                if ( commCS.isCollectionExist( clName ) )

                    commCS.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "messqge:" + e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }
        if ( sdb != null ) {
            sdb.disconnect();
        }
    }

}
