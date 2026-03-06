package com.sequoiadb.sdbschedule;

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
 * @Dscription Test of creating a schedule with unsupported type
 */
public class SdbSchedule_34355 extends SdbTestBase {

    private String scheduleName = "SdbSchedule_34355_schedule";
    private String type = "unknown";

    @BeforeClass()
    public void setUp() {
    }

    @Test
    public void test() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( type );
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

        Response resp = BusinessApiFactory.Schedule
                .create( scheduleUserEntity.toBSONObject() );
        Assert.assertEquals( resp.getStatusCode(), 500,
                "Create schedule with unknown type should fail" );

    }

    @AfterClass()
    public void tearDown() {

    }
}