package com.sequoiadb.snapshot;

import java.util.*;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
/**
 * @Description: seqDB-22489:同时执行插入、切分操作，检查会话快照中IsBlocked和Doing字段信息
 * @Author Zhao Xiaoni
 * @Date 2020.7.30
 */
public class Snapshot22489 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl_22489";
    private List< String > groupNames;
    private String groupName;
    private String lobSb;
    private int times = 0;
    private int totalTimes = 60;
    private static boolean isSuccess = false;
    List< WriteLob > writeLobThds = new ArrayList<>();

    @BeforeClass
    public void setup(){
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        lobSb = LobOprUtils.getRandomString( 1024*1024*200 );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "ONE GROUP MODE" );
        }

        groupName = groupNames.get( 0 );
        sdb.getCollectionSpace( csName ).createCollection( clName, (BSONObject)JSON.parse( "{ ReplSize: 7, "
                + "ShardingKey: { 'a': 1 }, ShardingType: 'hash', Group: '" + groupName + "' }" ) );

        DBCollection cl = sdb.getCollectionSpace( csName ).getCollection( clName );
        for( int i = 0; i < 5; ++i ){
            DBLob lob = cl.createLob();
            lob.write( lobSb.getBytes() );
            lob.close();
        }
    }

    @Test
    public void test() throws Exception{
        for( int i = 0; i < 20; ++i ){
            WriteLob writeLob = new WriteLob();
            writeLob.start();
            writeLobThds.add( writeLob );
        }
        Thread.sleep( 3000 );
        Split split = new Split();
        split.start();

        DBCursor cursor = null;
        do{
            Thread.sleep( 1 );
            cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SESSIONS, "{ 'NodeSelect': 'master', 'IsBlocked': true, "
                    + "'Doing': 'Waiting for freezing window(Name:story_java_test.cl_22489)' }", null, null );
            if( cursor.hasNext() )
            {
                isSuccess = true;
                System.out.println( "success" );
                break;
            }
        }while( times < totalTimes );

        while(!split.isSuccess()){
            Thread.sleep( 5000 );
            System.out.println( "Sleep 5s, waiting for split threads to finish" );
        }

        for( WriteLob thd : writeLobThds ){
            Thread.sleep( 5000 );
            if(!thd.isSuccess()){
                System.out.println( "Sleep 5s, waiting for writeLob threads to finish" );
                continue;
            }
        }

        Assert.assertTrue( isSuccess );
    }

    public class WriteLob extends SdbThreadBase{
        DBCollection cl = null;
        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            try( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                while( !isSuccess && times < totalTimes ){
                    times++;
                    System.out.println( "Begin to createLob, isSuccess: " + isSuccess + ", times: " + times + ", totalTimes: " + totalTimes );
                    DBLob lob = cl.createLob();
                    lob.write( lobSb.getBytes() );
                    lob.close();
                    System.out.println( "End to createLob, isSuccess: " + isSuccess + ", times: " + times + ", totalTimes: " + totalTimes );
                }
            }
        }
    }

    public class Split extends SdbThreadBase{
        DBCollection cl = null;
        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            try( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                String tmpGroup = null;
                String srcGroup = groupName;
                String desGroup = groupNames.get( 1 );
                while( !isSuccess && times < totalTimes ){
                    System.out.println( "Begin to split from " + srcGroup + " to " + desGroup + ", times: " + times + ", totalTimes: " + totalTimes );
                    cl.split( srcGroup, desGroup, 100 );
                    System.out.println( "End to split from " + srcGroup + " to " + desGroup + ", times: " + times + ", totalTimes: " + totalTimes );
                    tmpGroup = srcGroup;
                    srcGroup = desGroup;
                    desGroup = tmpGroup;
                }
            }
        }
    }

    @AfterClass
    public void tearDown(){
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }
}
