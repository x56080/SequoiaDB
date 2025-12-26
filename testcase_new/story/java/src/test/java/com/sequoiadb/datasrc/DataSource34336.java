package com.sequoiadb.datasrc;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Iterator;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-34336:挂载同一数据源的多个子表后进行lob操作
 * @author Suqiang Lin
 * @Date 2025.10.26
 * @version 1.0
 */
public class DataSource34336 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource34336";
    private String csName = "cs_34336";
    private String srcCSName = "cssrc_34336";
    private String srcCLName = "clsrc_34336";
    private String clName = "cl_34336";

    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );

        // prepare datasource cs cl
        CollectionSpace srcCS = srcdb.createCollectionSpace( srcCSName ) ;
        BasicBSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        DBCollection srcCL = srcCS.createCollection( srcCLName, options ) ;

        srcCS.createCollection( srcCLName + "_2" ) ;
        BasicBSONObject attachOptions = new BasicBSONObject();
        attachOptions.put( "LowBound", new BasicBSONObject( "date", "20251002" ) );
        attachOptions.put( "UpBound", new BasicBSONObject( "date", "20251003" ) );
        srcCL.attachCollection( srcCSName + "." + srcCLName + "_2", attachOptions );

        srcCS.createCollection( srcCLName + "_3" ) ;
        attachOptions = new BasicBSONObject();
        attachOptions.put( "LowBound", new BasicBSONObject( "date", "20251003" ) );
        attachOptions.put( "UpBound", new BasicBSONObject( "date", "20251004" ) );
        srcCL.attachCollection( srcCSName + "." + srcCLName + "_3", attachOptions );



        // prepare local cs cl
        cs = sdb.createCollectionSpace( csName );
        options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        cl = cs.createCollection( clName, options );

        cs.createCollection( clName + "_1" );
        attachOptions = new BasicBSONObject();
        attachOptions.put( "LowBound", new BasicBSONObject( "date", "20251001" ) );
        attachOptions.put( "UpBound", new BasicBSONObject( "date", "20251002" ) );
        cl.attachCollection( csName + "." + clName + "_1", attachOptions );

        options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + srcCLName + "_2" );
        cs.createCollection( clName + "_2", options );
        attachOptions = new BasicBSONObject();
        attachOptions.put( "LowBound", new BasicBSONObject( "date", "20251002" ) );
        attachOptions.put( "UpBound", new BasicBSONObject( "date", "20251003" ) );
        cl.attachCollection( csName + "." + clName + "_2", attachOptions );

        options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + srcCLName + "_3" );
        cs.createCollection( clName + "_3", options );
        attachOptions = new BasicBSONObject();
        attachOptions.put( "LowBound", new BasicBSONObject( "date", "20251003" ) );
        attachOptions.put( "UpBound", new BasicBSONObject( "date", "20251004" ) );
        cl.attachCollection( csName + "." + clName + "_3", attachOptions );
    }

    @Test
    public void test() throws Exception {
        // createLobID
        SimpleDateFormat sdf = new SimpleDateFormat( "yyyy-MM-dd" );
        Date d1 = sdf.parse( "2025-10-01" );
        ObjectId id1 = cl.createLobID(d1);
        Date d2 = sdf.parse( "2025-10-02" );
        ObjectId id2 = cl.createLobID(d2);
        Date d3 = sdf.parse( "2025-10-03" );
        ObjectId id3 = cl.createLobID(d3);

        // write
        byte[] wbuff = DataSrcUtils.getRandomBytes( 100 );
        String md5 = DataSrcUtils.getMd5( wbuff );

        try ( DBLob wLob1 = cl.createLob( id1 ) ) {
            wLob1.write( wbuff );
        }

        try ( DBLob wLob2 = cl.createLob( id2 ) ) {
            wLob2.write( wbuff );
        }

        try ( DBLob wLob3 = cl.createLob( id3 ) ) {
            wLob3.write( wbuff );
        }

        // listLobs
        try ( DBCursor listLob = cl.listLobs() ) {
            int count = 0;
            while ( listLob.hasNext() ) {
                BSONObject obj = listLob.getNext();
                count += 1;
            }
            Assert.assertEquals( count, 3 );
        }

        // read
        try ( DBLob rLob1 = cl.openLob( id1, DBLob.SDB_LOB_READ )) {
            byte[] rbuff = new byte[ ( int ) rLob1.getSize() ];
            rLob1.read( rbuff );
            String curMd5 = DataSrcUtils.getMd5( rbuff );
            Assert.assertEquals( curMd5, md5 );
        }

        try ( DBLob rLob2 = cl.openLob( id2, DBLob.SDB_LOB_READ )) {
            byte[] rbuff = new byte[ ( int ) rLob2.getSize() ];
            rLob2.read( rbuff );
            String curMd5 = DataSrcUtils.getMd5( rbuff );
            Assert.assertEquals( curMd5, md5 );
        }

        try ( DBLob rLob3 = cl.openLob( id3, DBLob.SDB_LOB_READ )) {
            byte[] rbuff = new byte[ ( int ) rLob3.getSize() ];
            rLob3.read( rbuff );
            String curMd5 = DataSrcUtils.getMd5( rbuff );
            Assert.assertEquals( curMd5, md5 );
        }

        // update
        // lock
        // getRTDetail
        try ( DBLob mLob1 = cl.openLob( id1, DBLob.SDB_LOB_WRITE )) {
            mLob1.lock( 50, 50 );
            mLob1.seek( 50, DBLob.SDB_LOB_SEEK_SET );
            mLob1.write( wbuff, 0, 50 );
            BSONObject detail = mLob1.getRunTimeDetail();
            checkLockResult( detail );
        }

        try ( DBLob mLob2 = cl.openLob( id2, DBLob.SDB_LOB_WRITE )) {
            mLob2.lock( 50, 50 );
            mLob2.seek( 50, DBLob.SDB_LOB_SEEK_SET );
            mLob2.write( wbuff, 0, 50 );
            BSONObject detail = mLob2.getRunTimeDetail();
            checkLockResult( detail );
        }

        try ( DBLob mLob3 = cl.openLob( id3, DBLob.SDB_LOB_WRITE )) {
            mLob3.lock( 50, 50 );
            mLob3.seek( 50, DBLob.SDB_LOB_SEEK_SET );
            mLob3.write( wbuff, 0, 50 );
            BSONObject detail = mLob3.getRunTimeDetail();
            checkLockResult( detail );
        }

        // truncate
        cl.truncateLob( id1, 80 );
        cl.truncateLob( id2, 80 );
        cl.truncateLob( id3, 80 );

        try ( DBCursor listLob = cl.listLobs() ) {
            int count = 0;
            while ( listLob.hasNext() ) {
                BSONObject obj = listLob.getNext();
                Assert.assertEquals( (long) obj.get( "Size" ), 80 );
                count += 1;
            }
            Assert.assertEquals( count, 3 );
        }


        // remove
        cl.removeLob( id1 );
        cl.removeLob( id2 );

        try ( DBCursor listLob = cl.listLobs() ) {
            int count = 0;
            while ( listLob.hasNext() ) {
                BSONObject obj = listLob.getNext();
                count += 1;
            }
            Assert.assertEquals( count, 1 );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            srcdb.dropCollectionSpace( srcCSName );
            DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private void checkLockResult( BSONObject detail ) {
        BSONObject accessInfo = (BSONObject) detail.get( "AccessInfo" );
        BSONObject lockSec = (BSONObject) accessInfo.get( "LockSections" );
        BSONObject lockSec1 = (BSONObject) lockSec.get( "0" );
        Assert.assertEquals( (long) lockSec1.get( "Begin" ), 50 );
        Assert.assertEquals( (long) lockSec1.get( "End" ), 100 );
    }
}
