package com.sequoiadb.sdbschedule;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
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
 * @Dscription Test of transfer sub collection with enable interruption
 */
public class SdbSchedule_34393 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34393_schedule";
    private String csName = "SdbSchedule_34393";
    private String mainCLName = "mainCL";
    private String subCLName1 = "subCL1";
    private String subCLName2 = "subCL2";
    private String sourceSite = "rootsite";
    private String targetSite;
    private int recordCount = 10;

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

        cs.createCollection( subCLName1 );
        cs.createCollection( subCLName2 );

        TestUtils.attachCL( mainCL, new BasicBSONObject( "date", "20220101" ),
                new BasicBSONObject( "date", "20220201" ),
                csName + "." + subCLName1 );
        TestUtils.attachCL( mainCL, new BasicBSONObject( "date", "20220201" ),
                new BasicBSONObject( "date", "20220301" ),
                csName + "." + subCLName2 );

        ReplicaGroup sysCatalogGroup = sDB.getReplicaGroup( "SYSCatalogGroup" );
        Sequoiadb sysCataConnect = sysCatalogGroup.getMaster().connect();
        try {
            DBCollection collection = sysCataConnect
                    .getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONS" );
            collection.updateRecords(
                    new BasicBSONObject( "Name", csName + "." + subCLName2 ),
                    new BasicBSONObject( "$set", new BasicBSONObject(
                            "CreateTime", "2020-01-22-17.36.13.231000" ) ) );
        } finally {
            sysCataConnect.close();
        }
    }

    @Test
    public void test() {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName );
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
        content.put( "cl_create_time_threshold", 10 );
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
        Assert.assertFalse( tCS.isCollectionExist( subCLName1 ),
                "SubCL1 should exist in datasource site" );
        Assert.assertFalse( tCS.isCollectionExist( subCLName2 ),
                "SubCL2 should not exist in datasource site" );

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