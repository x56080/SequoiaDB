package com.sequoiadb.split;

import java.util.ArrayList;
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
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;

import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-513 数据切分过程中插入大量数据，包括普通记录数据和lob数据:1.在cl下指定分区键进行数据切分
 *                     2、切分过程中向cl中插入大量数据，其中包括普通记录和lob对象，如插入1百万条记录 3、查看数据切分结果
 *                     4、再次插入数据，查看写数据情况
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split513 extends SdbTestBase {
    private String clName = "testcaseCL513";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;

    @BeforeClass(enabled = true)
    public void setUp() {

        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( commlib.getDataGroupNames( commSdb ).size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }

            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            commCS.createCollection( clName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"a\":1},ShardingType:\"hash\"}" ) );
            ArrayList< String > tmp = SplitUtils.getGroupName( commSdb, csName,
                    clName );
            srcGroupName = tmp.get( 0 );
            destGroupName = tmp.get( 1 );
            prepareData( commSdb );// 写入待切分的记录（1000）
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    // 切分时，插入LOB和普通记录，校验结果
    @Test(enabled = true)
    public void insertLobAndDoc() {
        Sequoiadb sdb = null;
        Split split = new Split();
        InsertData insertDataThread = new InsertData();
        try {
            split.start();
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
            insertDataThread.start();
            if ( !split.isSuccess() ) {
                Assert.fail( split.getErrorMsg() );
            }
            if ( !insertDataThread.isSuccess() ) {
                Assert.fail( insertDataThread.getErrorMsg() );
            }
            sdb = new Sequoiadb( coordUrl, "", "" );
            checkCatalog( sdb );// 检查编目信息
            checkData( sdb ); // 检查目标组数据量，重新插入数据，检查落入情况
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }

    }

    public void prepareData( Sequoiadb db ) {
        try {
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            ArrayList< BSONObject > arr = new ArrayList< BSONObject >();
            for ( int i = 0; i < 1000; i++ ) {
                arr.add( ( BSONObject ) JSON.parse( "{a:" + i + "}" ) );
            }
            cl.bulkInsert( arr, SplitUtils.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            throw e;
        }
    }

    private void checkData( Sequoiadb sdb ) {
        Sequoiadb destDataNode = null;
        Sequoiadb srcdataNode = null;
        try {
            // 获取目标组主节点链接
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();
            // 获取源组主节点链接
            srcdataNode = sdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();

            checkDestGroupDataCount( destDataNode ); // 检查目标组数据正确性
            insertAndCheck( sdb, destDataNode, srcdataNode ); // 重新插入数据，检查落入情况
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( srcdataNode != null ) {
                srcdataNode.disconnect();
            }
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
        }

    }

    public void insertAndCheck( Sequoiadb sdb, Sequoiadb destDataNode,
            Sequoiadb srcdataNode ) {
        DBCursor dbc2 = null;
        DBCursor dbc3 = null;
        try {
            // 插入数据，检查落入情况
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            dbc2 = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName )
                    .query( "", null, null, null, 0, -1 );

            // 获取一个目标组的记录，添加一个b字段，去除_id后重新插入,期望此数据落入目标组)
            BSONObject bobj = null;
            if ( dbc2.hasNext() ) {
                bobj = dbc2.getNext();
            } else {
                Assert.fail( "query error" );
            }
            bobj.put( "b", -10 );
            bobj.removeField( "_id" );
            cl.insert( bobj );

            // 检查是否落入目标组
            DBCollection destGroupCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( destGroupCL,
                    bobj.toString() ) ) {
                Assert.fail( "check query data not pass" );
            }

            // 获取一个源组的记录，添加一个c字段，去除_id后重新插入,期望此数据落入源组
            dbc3 = srcdataNode.getCollectionSpace( csName )
                    .getCollection( clName )
                    .query( "", null, null, null, 0, -1 );
            BSONObject bobj2 = null;
            if ( dbc3.hasNext() ) {
                bobj2 = dbc3.getNext();
            } else {
                Assert.fail( "query error" );
            }
            bobj2.put( "c", -10 );
            bobj2.removeField( "_id" );
            cl.insert( bobj2 );

            // 检查是否落入源组
            DBCollection srcGroupCL = srcdataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( srcGroupCL,
                    bobj2.toString() ) ) {
                Assert.fail( "check query data not pass" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc2 != null ) {
                dbc2.close();
            }
            if ( dbc3 != null ) {
                dbc3.close();
            }
        }
    }

    private void checkDestGroupDataCount( Sequoiadb destDataNode ) {
        DBCursor dbc1 = null;
        try {
            // 统计目标组普通记录数目
            long destDataCount = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName ).getCount();

            // 统计目标组Lob数目
            dbc1 = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName ).listLobs();
            int destLobCount = 0;
            while ( dbc1.hasNext() ) {
                destLobCount++;
                dbc1.getNext();
            }

            if ( destDataCount < 550 - ( 550 * 0.3 )
                    || destDataCount > 550 + ( 550 * 0.3 ) ) {
                Assert.fail( "split count unexpeted" );// 对目标组的普通记录数量做校验
            }
            if ( destLobCount < 50 - ( 50 * 0.3 )
                    || destLobCount > 50 + ( 50 * 0.3 ) ) {
                Assert.fail( "split lob count unexpeted:" + destDataCount );// 对目标组的lob记录数量做校验
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc1 != null ) {
                dbc1.close();
            }
        }
    }

    // 检查编目信息的切分范围是否正确
    private void checkCatalog( Sequoiadb sdb ) {
        DBCursor dbc = null;
        try {
            dbc = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            } else {
                Assert.fail( clName + " collection catalog not found" );
            }
            BSONObject expectLowBound = ( BSONObject ) JSON
                    .parse( "{\"\":2048}" );
            BSONObject expectUpBound = ( BSONObject ) JSON
                    .parse( "{\"\":4096}" );
            for ( int i = 0; i < list.size(); i++ ) {
                String groupName = ( String ) ( ( BSONObject ) list.get( i ) )
                        .get( "GroupName" );
                if ( groupName.equals( destGroupName ) ) {
                    BSONObject actualLowBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "LowBound" );
                    BSONObject actualUpBound = ( BSONObject ) ( ( BSONObject ) list
                            .get( i ) ).get( "UpBound" );
                    if ( actualLowBound.equals( expectLowBound )
                            && actualUpBound.equals( expectUpBound ) ) {
                        break;
                    } else {
                        Assert.fail( "check catalog fail" );
                    }
                }
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }

    }

    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            CollectionSpace commCS = commSdb.getCollectionSpace( csName );
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    class InsertData extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 100; i++ ) {
                    DBLob blob = cl.createLob();
                    blob.write( clName.getBytes() );
                    blob.close();
                }
                for ( int j = 0; j < 100; j++ ) {
                    cl.insert( "{a:" + j + "}" );
                }
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

    class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName, 50 );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }
}
