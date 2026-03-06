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

/**
 * @Dscription Test of transfer sub collection with change auto increment
 */
public class SdbSchedule_34389 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34389_schedule";
    private String csName = "SdbSchedule_34389";
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
        mainCL.createAutoIncrement(
                new BasicBSONObject( "Field", "incField1" ) );

        DBCollection subCL = cs.createCollection( subCLName );
        TestUtils.attachCL( mainCL, new BasicBSONObject( "date", "20220101" ),
                new BasicBSONObject( "date", "20220201" ),
                csName + "." + subCLName );
        for ( int i = 0; i < 10; i++ ) {
            mainCL.insertRecord( new BasicBSONObject( "id", i ).append( "date",
                    "20220101" ) );
        }
        BasicBSONObject basicBSONObject = new BasicBSONObject( "Field",
                "incField2" );
        basicBSONObject.put( "Generated", "always" );
        subCL.createAutoIncrement( basicBSONObject );

        for ( int i = 0; i < 10; i++ ) {
            mainCL.insertRecord( new BasicBSONObject( "id", i ).append( "date",
                    "20220101" ) );
        }
    }

    @Test
    public void test() throws Exception {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( "transfer" );
        scheduleUserEntity.setCron( "0/10 * * * * ?" );

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

        Assert.assertTrue( tDB.isCollectionSpaceExist( csName ),
                "CollectionSpace should exist in datasource site" );
        CollectionSpace tCS = tDB.getCollectionSpace( csName );
        Assert.assertTrue( tCS.isCollectionExist( mainCLName ),
                "MainCL should exist in datasource site" );
        Assert.assertTrue( tCS.isCollectionExist( subCLName ),
                "SubCL should exist in datasource site" );
        Assert.assertTrue(
                TestUtils.compareCataInfo( sDB, tDB,
                        csName + "." + mainCLName ),
                "MainCL cataInfo should be same" );
        Assert.assertTrue( TestUtils.compareRecord(
                sDB.getCollectionSpace( csName ).getCollection( mainCLName ),
                tDB.getCollectionSpace( csName )
                        .getCollection( mainCLName ) ) );
        Assert.assertTrue( TestUtils.compareRecord(
                sDB.getCollectionSpace( csName ).getCollection( subCLName ),
                tDB.getCollectionSpace( csName ).getCollection( subCLName ) ) );
        Assert.assertTrue(
                TestUtils.compareAutoInc( sDB, tDB, csName + "." + mainCLName ),
                "MainCL AutoInc should be same" );
        Assert.assertTrue(
                TestUtils.compareAutoInc( sDB, tDB, csName + "." + subCLName ),
                "SubCL AutoInc should be same" );

        CollectionSpace sCS = sDB.getCollectionSpace( csName );
        sCS.getCollection( subCLName ).createAutoIncrement(
                new BasicBSONObject( "Field", "incField3" ) );

        Response listResp = BusinessApiFactory.Schedule.listTasks( id, 0, -1,
                null, null );
        int taskCount = ( ( BasicBSONList ) BsonUtils.fromResponse( listResp ) )
                .size();
        BusinessApiFactory.Schedule.switchEnable( id, true );
        TestUtils.waitFinish( id, taskCount + 2 );
        BusinessApiFactory.Schedule.switchEnable( id, false );

        Assert.assertTrue(
                TestUtils.compareAutoInc( sDB, tDB, csName + "." + mainCLName ),
                "MainCL AutoInc should be same" );
        Assert.assertTrue(
                TestUtils.compareAutoInc( sDB, tDB, csName + "." + subCLName ),
                "SubCL AutoInc should be same" );

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
}
