package com.sequoiadb.sdbschedule;

import com.sequoiadb.sdbschedule.utils.BsonUtils;
import com.sequoiadb.sdbschedule.utils.BusinessApiFactory;
import com.sequoiadb.sdbschedule.utils.ScheduleFullEntity;
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
 * @Dscription Test of using valid parameters to create schedule
 */
public class SdbSchedule_34357 extends SdbTestBase {

    private String scheduleName = "SdbSchedule_34357_schedule";

    private String sourceSite = "rootsite";
    private String targetSite;

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
    }

    @Test
    public void test() {
        createClean();
        createDataSwitch();
        createTransfer();
    }

    @AfterClass()
    public void tearDown() {

    }

    private void createClean() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_clean" );
        scheduleUserEntity.setDesc( scheduleName + "_clean" );
        scheduleUserEntity.setType( "clean" );
        scheduleUserEntity.setCron( "0 0 */1 * * ?" );

        BSONObject content = new BasicBSONObject();
        content.put( "max_exec_time", 3600000 );
        BSONObject cleanRange = new BasicBSONObject();
        cleanRange.put( "max_retention_days", 30 );
        cleanRange.put( "clean_site", null );
        cleanRange.put( "clean_site_regex", ".*" );
        cleanRange.put( "cl_list", new BasicBSONList() );
        cleanRange.put( "cs_list", new BasicBSONList() );
        BasicBSONList regexList = new BasicBSONList();
        regexList.add( ".*" );
        cleanRange.put( "cs_regex", regexList );
        cleanRange.put( "cl_regex", regexList );
        BasicBSONList cleanRangeList = new BasicBSONList();
        cleanRangeList.add( cleanRange );
        content.put( "clean_range", cleanRangeList );
        scheduleUserEntity.setContent( content );

        createSchedule( scheduleUserEntity );
    }

    private void createDataSwitch() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_data_switch" );
        scheduleUserEntity.setDesc( scheduleName + "_data_switch" );
        scheduleUserEntity.setType( "data_switch" );
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
        content.put( "partition_interruption", true );
        content.put( "cl_create_time_threshold", 30 );
        content.put( "max_exec_time", 3600000 );
        scheduleUserEntity.setContent( content );

        createSchedule( scheduleUserEntity );
    }

    private void createTransfer() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_transfer" );
        scheduleUserEntity.setDesc( scheduleName + "_transfer" );
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

        createSchedule( scheduleUserEntity );
    }

    private void createSchedule( ScheduleUserEntity scheduleUserEntity ) {
        Response resp = BusinessApiFactory.Schedule
                .create( scheduleUserEntity.toBSONObject() );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Create schedule response code is not 200" );
        String id = ( String ) BsonUtils.fromResponse( resp ).get( "id" );

        resp = BusinessApiFactory.Schedule.get( id );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Get schedule response code is not 200" );
        BSONObject schedule = BsonUtils.fromResponse( resp );
        Assert.assertNotNull( schedule, "Get schedule response is null" );
        ScheduleFullEntity fullEntity = ScheduleFullEntity
                .fromBSONObject( schedule );
        Assert.assertEquals( fullEntity.getName(), scheduleUserEntity.getName(),
                "Schedule name mismatch" );
        Assert.assertEquals( fullEntity.getDesc(), scheduleUserEntity.getDesc(),
                "Schedule desc mismatch" );
        Assert.assertEquals( fullEntity.getType(), scheduleUserEntity.getType(),
                "Schedule type mismatch" );
        Assert.assertEquals( fullEntity.getCron(), scheduleUserEntity.getCron(),
                "Schedule cron mismatch" );
        Assert.assertEquals( fullEntity.getContent(),
                scheduleUserEntity.getContent(), "Schedule content mismatch" );

        BusinessApiFactory.Schedule.delete( id );
    }
}