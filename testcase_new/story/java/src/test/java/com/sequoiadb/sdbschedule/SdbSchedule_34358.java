package com.sequoiadb.sdbschedule;

import com.sequoiadb.base.CollectionSpace;
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

public class SdbSchedule_34358 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34358_schedule";
    private String csName = "SdbSchedule_34358";
    private String mainCLName = "mainCL";
    private String subCLName = "subCL1";
    private String sourceSite = "rootsite";
    private String targetSite;

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tDB = DataSourceUtils.getDsConnect(sDB, getScheduleDataSourceName());
        BSONObject mainCLOption = new BasicBSONObject();
        mainCLOption.put( "LobShardingKeyFormat", "YYYYMMDD" );
        mainCLOption.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        mainCLOption.put( "IsMainCL", true );
        mainCLOption.put( "ShardingType", "range" );

        CollectionSpace cs = sDB.createCollectionSpace( csName );
        DBCollection mainCL = cs.createCollection( mainCLName, mainCLOption );
        cs.createCollection( subCLName );

        TestUtils.attachCL( mainCL, new BasicBSONObject( "date", "20220101" ),
                new BasicBSONObject( "date", "20220201" ),
                csName + "." + subCLName );
    }

    @Test
    public void test() {
        transfer();
        String id = dataSwitch();

        Assert.assertTrue( tDB.isCollectionSpaceExist( csName ),
                "CollectionSpace should exist in datasource site" );
        Assert.assertTrue(
                tDB.getCollectionSpace( csName ).isCollectionExist( subCLName ),
                "Collection should exist in datasource site" );

        Assert.assertTrue(
                TestUtils.isMapped( sDB, csName + "." + subCLName ) );
        Assert.assertTrue( TestUtils.compareSubCLRange( sDB,
                csName + "." + mainCLName, csName + "." + subCLName,
                new BasicBSONObject( "date", "20220101" ),
                new BasicBSONObject( "date", "20220201" ) ) );

        BusinessApiFactory.Schedule.delete( id );

        dropClAndUpdateStatus();

        checkResult();
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

    private void checkResult() {
        boolean found = false;
        int maxWaitCount = 121;
        for ( int i = 0; i <= maxWaitCount; i++ ) {
            try {
                Thread.sleep( 1000 );
            } catch ( Exception e ) {
                e.printStackTrace();
            }

            if ( sDB.getCollectionSpace( csName )
                    .isCollectionExist( subCLName ) ) {
                found = true;
                break;
            }
        }

        Assert.assertTrue( found, "cl should be switched again" );
        Assert.assertTrue(
                TestUtils.isMapped( sDB, csName + "." + subCLName ) );
        Assert.assertTrue( TestUtils.compareSubCLRange( sDB,
                csName + "." + mainCLName, csName + "." + subCLName,
                new BasicBSONObject( "date", "20220101" ),
                new BasicBSONObject( "date", "20220201" ) ) );
    }

    private void dropClAndUpdateStatus() {
        sDB.getCollectionSpace( csName ).dropCollection( subCLName );

        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "collection", csName + "." + subCLName );
        matcher.put( "source_site", sourceSite );
        matcher.put( "target_site", targetSite );
        sDB.getCollectionSpace( "SDB_SCHEDULE_SYSTEM" )
                .getCollection( "COLLECTION_DATA_SWITCH_EVENT" )
                .updateRecords( matcher, new BasicBSONObject( "$set",
                        new BasicBSONObject( "status", "data_switching" ) ) );
    }

    private String dataSwitch() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName + "_dataSwitch" );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( "data_switch" );
        scheduleUserEntity.setCron( "0/10 * * * * ?" );

        BSONObject content = new BasicBSONObject();
        content.put( "source_site", sourceSite );
        content.put( "target_site", targetSite );
        content.put( "cl_list", new BasicBSONList() );
        BasicBSONList csList = new BasicBSONList();
        csList.add( csName );
        content.put( "cs_list", csList );
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
        return id;
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
        content.put( "cl_list", new BasicBSONList() );
        BasicBSONList csList = new BasicBSONList();
        csList.add( csName );
        content.put( "cs_list", csList );
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
}
