package com.sequoiadb.sdbschedule;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.sdbschedule.utils.BsonUtils;
import com.sequoiadb.sdbschedule.utils.BusinessApiFactory;
import com.sequoiadb.sdbschedule.utils.DataSourceUtils;
import com.sequoiadb.sdbschedule.utils.ScheduleUserEntity;
import com.sequoiadb.sdbschedule.utils.TestUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import io.restassured.response.Response;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Dscription Test of transfer collection only have LOBs
 */
public class SdbSchedule_34382 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34382_schedule";
    private String csName = "SdbSchedule_34382";
    private String clName = "SdbSchedule_34382";
    private String sourceSite = "rootsite";
    private String targetSite;
    private DBCollection sCL;
    private int lobCount = 10;

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tDB = DataSourceUtils.getDsConnect(sDB, getScheduleDataSourceName());
        sCL = sDB.createCollectionSpace( csName ).createCollection( clName );
        for ( int i = 0; i < lobCount; i++ ) {
            DBLob lob = sCL.createLob();
            String content = "This is a test lob content for SdbSchedule_34382 "
                    + i;
            lob.write( content.getBytes() );
            lob.close();
        }
    }

    @Test
    public void test() throws Exception {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( "transfer" );
        scheduleUserEntity.setCron( "* * * * * ?" );

        BSONObject content = new BasicBSONObject();
        content.put( "source_site", sourceSite );
        content.put( "target_site", targetSite );
        BasicBSONList clList = new BasicBSONList();
        clList.add( csName + "." + clName );
        content.put( "cl_list", clList );
        content.put( "cs_list", new BasicBSONList() );
        content.put( "no_write_time_threshold", 0 );
        content.put( "delete_more_lob_in_target", false );
        content.put( "partition_interruption", true );
        content.put( "cl_create_time_threshold", 0 );
        content.put( "max_exec_time", 3600000 );
        scheduleUserEntity.setContent( content );

        Response resp = BusinessApiFactory.Schedule
                .create( scheduleUserEntity.toBSONObject() );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Create schedule response code is not 200" );
        String id = ( String ) BsonUtils.fromResponse( resp ).get( "id" );
        TestUtils.waitFinish( id, 2 );
        BusinessApiFactory.Schedule.switchEnable( id, false );

        Assert.assertTrue( tDB.isCollectionSpaceExist( csName ),
                "CollectionSpace should exist in datasource site" );
        Assert.assertTrue(
                tDB.getCollectionSpace( csName ).isCollectionExist( clName ),
                "Collection should exist in datasource site" );

        checkProgress( id );

        DBCollection tCL = tDB.getCollectionSpace( csName )
                .getCollection( clName );
        if ( !TestUtils.compareLobData( sCL, tCL ) ) {
            Assert.fail(
                    "LOB data in source and target collection are not the same" );
        }

        BusinessApiFactory.Schedule.delete( id );
    }

    @AfterClass()
    public void tearDown() {
        if ( sDB != null ) {
            if ( sDB.isCollectionSpaceExist( csName ) ) {
                sDB.dropCollectionSpace( csName );
            }
            sDB.close();
        }
        if ( tDB != null ) {
            if ( tDB.isCollectionSpaceExist( csName ) ) {
                tDB.dropCollectionSpace( csName );
            }
            tDB.close();
        }
    }

    private void checkProgress( String scheduleId ) {
        Response resp = BusinessApiFactory.Schedule.listTasks( scheduleId, 0,
                -1, null, null );
        BasicBSONList taskList = ( BasicBSONList ) BsonUtils
                .fromResponse( resp );
        int successLobNum = 0;
        for ( Object obj : taskList ) {
            BSONObject task = ( BSONObject ) obj;
            String taskId = ( String ) task.get( "id" );
            Response taskProgressResp = BusinessApiFactory.Task
                    .progress( taskId );
            BasicBSONList progressList = ( BasicBSONList ) BsonUtils
                    .fromResponse( taskProgressResp );
            if ( progressList == null ) {
                continue;
            }

            for ( Object o : progressList ) {
                BSONObject progress = ( BSONObject ) o;
                Number number = ( Number ) progress.get( "success_lob_num" );
                successLobNum += number.longValue();
            }
        }

        Assert.assertEquals( successLobNum, lobCount,
                "Transferred LOB number is not correct" );
    }
}
