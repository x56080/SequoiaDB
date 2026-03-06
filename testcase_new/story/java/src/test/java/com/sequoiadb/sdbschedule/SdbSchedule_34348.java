package com.sequoiadb.sdbschedule;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.sdbschedule.utils.BsonUtils;
import com.sequoiadb.sdbschedule.utils.BusinessApiFactory;
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

public class SdbSchedule_34348 extends SdbTestBase {

    private String scheduleName = "SdbSchedule_34348_schedule";

    private String sourceSite = "rootsite";
    private String targetSite;
    private Sequoiadb sDB;

    @BeforeClass()
    public void setUp() {
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        targetSite = getDatasourceSiteName();
    }

    @Test
    public void test() {
        String id = createTransfer();

        BasicBSONObject matcher = new BasicBSONObject( "schedule_id", id );
        BasicBSONObject modify = new BasicBSONObject();
        modify.put( "running_flag", 2 );
        modify.put( "create_time", System.currentTimeMillis() - 1200000 );
        sDB.getCollectionSpace( "SDB_SCHEDULE_SYSTEM" ).getCollection( "TASK" )
                .updateRecords( matcher,
                        new BasicBSONObject( "$set", modify ) );

        int maxWait = 60;
        boolean found = false;
        for ( int i = 0; i <= maxWait; i++ ) {
            Response resp = BusinessApiFactory.Schedule.listTasks( id, 0, 1,
                    new BasicBSONObject( "running_flag", 4 ), null );
            BasicBSONList list = ( BasicBSONList ) BsonUtils
                    .fromResponse( resp );
            if ( list != null && list.size() > 0 ) {
                found = true;
                break;
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }
        Assert.assertTrue( found,
                "Scheduled task with running_flag=4 not found within " + maxWait
                        + " seconds" );
        BusinessApiFactory.Schedule.delete( id );
    }

    @AfterClass()
    public void tearDown() {
        if (sDB != null) {
            sDB.close();
        }
    }

    private String createTransfer() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_transfer" );
        scheduleUserEntity.setDesc( scheduleName + "_transfer" );
        scheduleUserEntity.setType( "transfer" );
        scheduleUserEntity.setCron( "0/3 * * * * ?" );

        BSONObject content = new BasicBSONObject();
        content.put( "source_site", sourceSite );
        content.put( "target_site", targetSite );
        BasicBSONList csList = new BasicBSONList();
        csList.add( "aaa" );
        content.put( "cs_list", csList );
        BasicBSONList csRegex = new BasicBSONList();
        csRegex.add( "bbb" );
        content.put( "cs_regex", csRegex );
        BasicBSONList clList = new BasicBSONList();
        clList.add( "ccc" );
        content.put( "cl_list", clList );
        BasicBSONList clRegex = new BasicBSONList();
        clRegex.add( "ddd" );
        content.put( "cl_regex", clRegex );
        content.put( "no_write_time_threshold", 30 );
        content.put( "delete_more_lob_in_target", false );
        content.put( "partition_interruption", true );
        content.put( "cl_create_time_threshold", 30 );
        content.put( "max_exec_time", 3600000 );
        scheduleUserEntity.setContent( content );

        return createSchedule( scheduleUserEntity );
    }

    private String createSchedule( ScheduleUserEntity scheduleUserEntity ) {
        Response resp = BusinessApiFactory.Schedule
                .create( scheduleUserEntity.toBSONObject() );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Create schedule response code is not 200" );
        String id = ( String ) BsonUtils.fromResponse( resp ).get( "id" );
        TestUtils.waitFinish( id, 1 );
        return id;
    }
}
