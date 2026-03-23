package com.sequoiadb.sdbschedule;

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
 * @Description : Test of clean schedule with special clean site
 */
public class SdbSchedule_34354 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34354_schedule";
    private String csName = "SdbSchedule_34354";
    private String clName = "SdbSchedule_34354";
    private String sourceSite = "rootsite";
    private String targetSite;

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tDB = DataSourceUtils.getDsConnect(sDB, getScheduleDataSourceName());
        TestUtils.initCS( sDB, csName ).createCollection( clName );
    }

    @Test
    public void test() {

        transfer();
        dataSwitch();

        Assert.assertTrue( tDB.isCollectionSpaceExist( csName ),
                "CollectionSpace should exist in datasource site" );
        Assert.assertTrue(
                tDB.getCollectionSpace( csName ).isCollectionExist( clName ),
                "Collection should exist in datasource site" );

        Assert.assertTrue( TestUtils.isMapped( sDB, csName + "." + clName ) );
        Assert.assertTrue(
                TestUtils.isRenameCLExist( sDB, csName + "." + clName ) );

        String id = clean();

        Assert.assertTrue(
                sDB.getCollectionSpace( csName ).isCollectionExist( clName ) );
        Assert.assertFalse(
                TestUtils.isRenameCLExist( sDB, csName + "." + clName ) );
        checkProgress( id );
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

    private String clean() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_clean" );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( "clean" );
        scheduleUserEntity.setCron( "* * * * * ?" );

        BSONObject content = new BasicBSONObject();
        content.put( "max_exec_time", 3600000 );
        BSONObject cleanRange = new BasicBSONObject();
        cleanRange.put( "max_retention_days", 0 );
        cleanRange.put( "clean_site", sourceSite );
        cleanRange.put( "clean_site_regex", null );
        BasicBSONList clList = new BasicBSONList();
        clList.add( csName + "." + clName );
        cleanRange.put( "cl_list", clList );
        cleanRange.put( "cs_list", new BasicBSONList() );
        cleanRange.put( "cs_regex", null );
        cleanRange.put( "cl_regex", null );
        BasicBSONList cleanRangeList = new BasicBSONList();
        cleanRangeList.add( cleanRange );
        content.put( "clean_range", cleanRangeList );
        scheduleUserEntity.setContent( content );

        Response resp = BusinessApiFactory.Schedule
                .create( scheduleUserEntity.toBSONObject() );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Create schedule response code is not 200" );
        String id = ( String ) BsonUtils.fromResponse( resp ).get( "id" );
        TestUtils.waitFinish( id, 2 );
        BusinessApiFactory.Schedule.switchEnable( id, false );
        return id;
    }

    private void dataSwitch() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_dataSwitch" );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( "data_switch" );
        scheduleUserEntity.setCron( "* * * * * ?" );

        BSONObject content = new BasicBSONObject();
        content.put( "source_site", sourceSite );
        content.put( "target_site", targetSite );
        BasicBSONList clList = new BasicBSONList();
        clList.add( csName + "." + clName );
        content.put( "cl_list", clList );
        content.put( "cs_list", new BasicBSONList() );
        content.put( "no_write_time_threshold", 0 );
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
        BusinessApiFactory.Schedule.delete( id );
    }

    private void transfer() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_transfer" );
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
        BusinessApiFactory.Schedule.delete( id );
    }

    private void checkProgress( String id ) {
        Response resp = BusinessApiFactory.Schedule.listTasks( id, 0, -1, null,
                null );
        BasicBSONList taskList = ( BasicBSONList ) BsonUtils
                .fromResponse( resp );
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
                String sourceCl = ( String ) progress.get( "source_cl" );
                if ( sourceCl.contains( csName + "." + clName ) ) {
                    Boolean success = ( Boolean ) progress.get( "success" );
                    Assert.assertTrue( success,
                            "Clean cl should be successful" );
                    return;
                }
            }
        }
        Assert.fail( "Cannot find clean progress for cl" );

    }
}