package com.sequoiadb.lzwtransactionsync;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.lzwtransaction.LzwTransUtils;

/**
 * 1、日志满导致事务回滚的场景，比如：日志文件大小配置小点，开启事务，大批量灌数据
 * 
 * @author chensiqin
 * @Date 2016-12-16
 */
public class TestSeqDB6670 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl6670";
    private String rgName = "rg6670";
    private int port1;
    private int port2;
    private int port3;

    @BeforeClass()
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // 跳过 standAlone 和数据组不足的环境
            LzwTransUtils util = new LzwTransUtils();
            if ( util.isStandAlone( this.sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( LzwTransUtils.getDataRgNames( this.sdb ).size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            this.port1 = SdbTestBase.reservedPortBegin + 670;
            this.port2 = SdbTestBase.reservedPortBegin + 680;
            this.port3 = SdbTestBase.reservedPortBegin + 690;
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 1、CL压缩类型为lzw，开启事务，对CL做增删改查数据 2、日志爆满，自动回滚 3、检查返回结果，并检查数据压缩情况
     */
    @Test()
    public void test() {
        try {
            createDataGroup();
            createCL();
            // 插入数据，使cl存在已被压缩的记录
            LzwTransUtils util = new LzwTransUtils();
            util.insertData( this.cl, 0, 99, 1024 * 1024 );
            util.insertData( this.cl, 99, 109, 1024 * 1024 );
            BSONObject bObject = getSnapshotDetail();
            // wait for creating dictionary
            while ( !"true"
                    .equals( bObject.get( "DictionaryCreated" ).toString() ) ) {
                try {
                    Thread.sleep( 10 * 1000 );
                    bObject = getSnapshotDetail();
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
            }

            util.insertData( this.cl, 109, 115, 1024 );
            // 做事务操作之前的压缩信息
            BSONObject before = getSnapshotDetail();
            if ( ( double ) before.get( "CurrentCompressionRatio" ) >= 1 ) {
                Assert.fail( "CurrentCompressionRatio >= 1 !" );
            }
            Assert.assertEquals( before.get( "DictionaryCreated" ).toString(),
                    "true" );
            // 开启事务
            this.sdb.beginTransaction();
            // 对cl做增删改查操作,将日志写满
            try {
                this.cl.delete( "{_id:{$et:114}}", "{'':'_id'}" );
                for ( int i = 1; i <= 40000; i++ ) {
                    util.insertData( this.cl, 115, 116, 1024 );
                    this.cl.delete( "{_id:115}" );
                }
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -203 );
            }

            checkData();

            // 回滚后记录跟开启事务前数据一致且记录压缩状态一致
            BSONObject after = getSnapshotDetail();
            Assert.assertEquals( before.get( "CompressionType" ).toString(),
                    after.get( "CompressionType" ).toString() );
            Assert.assertEquals( before.get( "DictionaryCreated" ).toString(),
                    after.get( "DictionaryCreated" ).toString() );
            if ( ( double ) after.get( "CurrentCompressionRatio" ) >= 1 ) {
                Assert.fail( "CurrentCompressionRatio >= 1 !" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void checkData() {
        try {
            // 协调节点检查数据正确性
            Assert.assertEquals( this.cl.getCount(
                    "{$and:[{_id:{$gte:0}},{_id:{$lt:115}}]}" ), 115 );
            // 连接主节点数据正确性
            checkDataInMaster();
            // 连接从节点检查数据正确性
            checkDataInSlave();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

    }

    private void checkDataInSlave() {
        Sequoiadb dataDb = null;
        try {
            // 连接源组data验证数据
            List< String > rgNames = LzwTransUtils.getDataRgNames( this.sdb );
            ReplicaGroup replicaGroup = sdb.getReplicaGroup( this.rgName );
            Node slave = replicaGroup.getSlave();
            String url = slave.getNodeName();
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = cs.getCollection( this.clName );
            while ( true ) {
                if ( dbcl.getCount(
                        "{$and:[{_id:{$gte:0}},{_id:{$lt:115}}]}" ) == 115 ) {
                    break;
                }
            }
            // Assert.assertEquals(dbcl.getCount("{$and:[{_id:{$gte:0}},{_id:{$lt:115}}]}"),
            // 115);
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }
    }

    private void checkDataInMaster() {
        Sequoiadb dataDb = null;
        try {
            // 连接源组data验证数据
            List< String > rgNames = LzwTransUtils.getDataRgNames( this.sdb );
            String url = LzwTransUtils.getGroupIPByGroupName( this.sdb,
                    this.rgName );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = cs.getCollection( this.clName );
            Assert.assertEquals(
                    dbcl.getCount( "{$and:[{_id:{$gte:0}},{_id:{$lt:115}}]}" ),
                    115 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }
    }

    public BSONObject getSnapshotDetail() {
        BSONObject detail = null;
        Sequoiadb dataDB = null;
        try {
            detail = new BasicBSONObject();
            List< String > rgNames = LzwTransUtils.getDataRgNames( this.sdb );
            String url = LzwTransUtils.getGroupIPByGroupName( this.sdb,
                    this.rgName );
            dataDB = new Sequoiadb( url, "", "" );
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", csName + "." + clName );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            detail = ( BSONObject ) details.get( 0 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( dataDB != null ) {
                dataDB.disconnect();

            }
        }
        return detail;
    }

    public void createDataGroup() {
        try {

            BSONObject configure = new BasicBSONObject();
            configure.put( "logfilenum", 5 );
            configure.put( "transactionon", true );
            configure.put( "diaglevel", 5 );
            String hostName = sdb.getReplicaGroup( "SYSCatalogGroup" )
                    .getMaster().getHostName();
            try {
                sdb.removeReplicaGroup( this.rgName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -154 ) { // SDB_CLS_GRP_NOT_EXIST
                    throw e;
                }
            }
            ReplicaGroup rg = this.sdb.createReplicaGroup( this.rgName );
            System.out.println(
                    this.port1 + ":" + this.port2 + ":" + this.port3 );
            Node node1 = rg.createNode( hostName, this.port1,
                    SdbTestBase.workDir + port1 + "/", configure );
            Node node2 = rg.createNode( hostName, this.port2,
                    SdbTestBase.workDir + port2 + "/", configure );
            Node node3 = rg.createNode( hostName, this.port3,
                    SdbTestBase.workDir + port3 + "/", configure );
            this.sdb.activateReplicaGroup( this.rgName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void createCL() {
        try {
            List< String > rgNames = LzwTransUtils.getDataRgNames( this.sdb );
            BSONObject option = new BasicBSONObject();
            // 自己创建组
            option.put( "Group", this.rgName );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            this.cl = LzwTransUtils.createCL( this.cs, this.clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( this.clName ) ) {
                this.cs.dropCollection( this.clName );
            }
            ReplicaGroup rg = sdb.getReplicaGroup( this.rgName );
            String hostName = sdb.getReplicaGroup( "SYSCatalogGroup" )
                    .getMaster().getHostName();
            BSONObject configure = new BasicBSONObject();
            configure.put( "logfilenum", 5 );
            configure.put( "transactionon", true );
            if ( rg != null ) {
                sdb.removeReplicaGroup( this.rgName );
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }

}
