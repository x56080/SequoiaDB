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
 * @Dscription Test of transfer but the collection not meet the transfer
 *             condition
 */
public class SdbSchedule_34385 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34385_schedule";
    private String csName = "SdbSchedule_34385";
    private String clName = "SdbSchedule_34385";
    private String sourceSite = "rootsite";
    private String targetSite;

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tDB = DataSourceUtils.getDsConnect(sDB, getScheduleDataSourceName());
        TestUtils.initCS( sDB, csName ).createCollection( clName, new BasicBSONObject( "ReplSize", 0 ) );
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
        BasicBSONList clList = new BasicBSONList();
        clList.add( csName + "." + clName );
        content.put( "cl_list", clList );
        content.put( "cs_list", new BasicBSONList() );
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
        TestUtils.waitFinish( id, 2 );
        BusinessApiFactory.Schedule.switchEnable( id, false );

        Assert.assertFalse( tDB.isCollectionSpaceExist( csName ),
                "CollectionSpace should not exist in datasource site" );

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
