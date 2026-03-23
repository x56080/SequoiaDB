package com.sequoiadb.sdbschedule;

import com.sequoiadb.base.DBCollection;
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
 * @Dscription Test of collection auto inc change during transfer
 */
public class SdbSchedule_34378 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34378_schedule";
    private String csName = "SdbSchedule_34378";
    private String clName = "SdbSchedule_34378";
    private String sourceSite = "rootsite";
    private String targetSite;
    private DBCollection sCL;
    private BSONObject field1 = new BasicBSONObject( "Field", "autoIncField1" );
    private int recordCount = 10;

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tDB = DataSourceUtils.getDsConnect(sDB, getScheduleDataSourceName());
        sCL = TestUtils.initCS( sDB, csName ).createCollection( clName );
        sCL.createAutoIncrement( field1 );
        for ( int i = 0; i < recordCount; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "index", i );
            record.put( "message",
                    "This is a test record for SdbSchedule_34378 "
                            + i );
            sCL.insertRecord( record );
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
        TestUtils.waitFinish( id, 2, 150 );
        BusinessApiFactory.Schedule.switchEnable( id, false );

        Assert.assertTrue( tDB.isCollectionSpaceExist( csName ),
                "CollectionSpace should exist in datasource site" );
        Assert.assertTrue(
                tDB.getCollectionSpace( csName ).isCollectionExist( clName ),
                "Collection should exist in datasource site" );
        Assert.assertTrue(
                TestUtils.compareAutoInc( sDB, tDB, csName + "." + clName ),
                "AutoIncrement settings are not the same after transfer" );
        DBCollection tCL = tDB.getCollectionSpace( csName )
                .getCollection( clName );
        if ( !TestUtils.compareRecord( sCL, tCL ) ) {
            Assert.fail( "Source and target collection records are not same" );
        }

        sCL.createAutoIncrement(
                new BasicBSONObject( "Field", "autoIncField2" ) );
        Response listResp = BusinessApiFactory.Schedule.listTasks( id, 0, -1,
                null, null );
        int taskCount = ( ( BasicBSONList ) BsonUtils.fromResponse( listResp ) )
                .size();
        BusinessApiFactory.Schedule.switchEnable( id, true );
        TestUtils.waitFinish( id, taskCount + 2 );
        BusinessApiFactory.Schedule.switchEnable( id, false );
        Assert.assertTrue(
                TestUtils.compareAutoInc( sDB, tDB, csName + "." + clName ),
                "AutoIncrement settings are not the same after transfer" );
        BusinessApiFactory.Schedule.delete( id );
    }

    @AfterClass()
    public void tearDown() {
        if ( sDB != null ) {
            TestUtils.cleanCS( sDB, csName );
            sDB.close();
        }
        if ( tDB != null ) {
            TestUtils.cleanCS( tDB, csName );
            tDB.close();
        }
    }
}
