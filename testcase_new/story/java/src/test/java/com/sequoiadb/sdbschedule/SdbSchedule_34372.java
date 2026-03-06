package com.sequoiadb.sdbschedule;

import com.sequoiadb.sdbschedule.utils.BsonUtils;
import com.sequoiadb.sdbschedule.utils.BusinessApiFactory;
import com.sequoiadb.sdbschedule.utils.ScheduleUserEntity;
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
 * @Dscription Test of delete schedule
 */
public class SdbSchedule_34372 extends SdbTestBase {

    private String scheduleName = "SdbSchedule_34372_schedule";

    private String sourceSite = "rootsite";
    private String targetSite;

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
    }

    @Test
    public void test() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( "transfer" );
        scheduleUserEntity.setCron( "0 0 */1 * * ?" );

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

        Response resp = BusinessApiFactory.Schedule
                .create( scheduleUserEntity.toBSONObject() );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Create schedule response code is not 200" );
        String id = ( String ) BsonUtils.fromResponse( resp ).get( "id" );

        resp = BusinessApiFactory.Schedule.delete( id );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Delete schedule response code is not 200" );

        resp = BusinessApiFactory.Schedule.get( id );
        Assert.assertEquals( resp.getStatusCode(), 500,
                "Get not found schedule should fail" );
    }

    @AfterClass()
    public void tearDown() {

    }
}