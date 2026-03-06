package com.sequoiadb.sdbschedule;

import com.sequoiadb.sdbschedule.utils.BsonUtils;
import com.sequoiadb.sdbschedule.utils.BusinessApiFactory;
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
 * @Dscription Test of checking default schedule info
 */
public class SdbSchedule_34371 extends SdbTestBase {

    @BeforeClass()
    public void setUp() {
    }

    @Test
    public void test() {
        Response resp = BusinessApiFactory.Schedule.list( 0, 1,
                new BasicBSONObject( "name", "系统内置清理任务" ), null );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Schedule list response code is not 200" );
        BasicBSONList scheduleList = ( BasicBSONList ) BsonUtils
                .fromResponse( resp );
        Assert.assertNotNull( scheduleList, "Schedule list response is null" );
        Assert.assertEquals( scheduleList.size(), 1,
                "Default schedule is missing" );
        BasicBSONObject schedule = ( BasicBSONObject ) scheduleList.get( 0 );

        Assert.assertEquals( schedule.getString( "name" ), "系统内置清理任务",
                "Default schedule name is incorrect" );
        Assert.assertEquals( schedule.getString( "type" ), "clean",
                "Default schedule type is incorrect" );
        Assert.assertTrue( schedule.getBoolean( "enable" ),
                "Default schedule enable status is incorrect" );
        Assert.assertEquals( schedule.getString( "cron" ), "0 0 0 * * ?",
                "Default schedule cron is incorrect" );

        BSONObject expectContent = new BasicBSONObject();
        expectContent.put( "max_exec_time", 3600000 );
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
        expectContent.put( "clean_range", cleanRangeList );
        Assert.assertEquals( schedule.get( "content" ), expectContent,
                "Default schedule content is incorrect" );
    }

    @AfterClass()
    public void tearDown() {

    }
}