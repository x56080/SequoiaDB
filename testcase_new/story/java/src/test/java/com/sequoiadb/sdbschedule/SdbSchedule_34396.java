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

import java.util.ArrayList;

/**
 * @Dscription Test of transfer and specify cs restore domain
 */
public class SdbSchedule_34396 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34396_schedule";
    private String csName = "SdbSchedule_34396";
    private String clName = "SdbSchedule_34396";
    private String sourceSite = "rootsite";
    private String targetSite;
    private String domainName = "SdbSchedule_34396";

    @BeforeClass()
    public void setUp() {
        targetSite = getDatasourceSiteName();
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tDB = DataSourceUtils.getDsConnect(sDB, getScheduleDataSourceName());
        TestUtils.initCS( sDB, csName ).createCollection( clName );
        createDomain( tDB );
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
        content.put( "no_write_time_threshold", 0 );
        content.put( "delete_more_lob_in_target", false );
        content.put( "partition_interruption", true );
        content.put( "cl_create_time_threshold", 0 );
        content.put( "max_exec_time", 3600000 );
        content.put( "data_domain", domainName );
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
        Assert.assertTrue(
                tDB.getCollectionSpace( csName ).isCollectionExist( clName ),
                "Collection should exist in datasource site" );
        String name = tDB.getCollectionSpace( csName ).getDomainName();
        Assert.assertEquals( name, domainName );

        BusinessApiFactory.Schedule.delete( id );
    }

    @AfterClass()
    public void tearDown() {
        if ( sDB != null ) {
            TestUtils.cleanCS( sDB, csName);
            sDB.close();
        }
        if ( tDB != null ) {
            TestUtils.cleanCS( tDB, csName );
            if ( tDB.isDomainExist( domainName ) ) {
                tDB.dropDomain( domainName );
            }
            tDB.close();
        }
    }

    private void createDomain( Sequoiadb db ) {
        ArrayList< String > dataGroups = getDataGroups( db );
        BSONObject obj = new BasicBSONObject();
        obj.put( "Groups", dataGroups.toArray() );
        if ( db.isDomainExist( domainName ) ) {
            db.dropDomain( domainName );
            db.createDomain( domainName, obj );
        } else {
            db.createDomain( domainName, obj );
        }
    }

    private ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        ArrayList< String > groupList = null;
        groupList = sdb.getReplicaGroupNames();
        groupList.remove( "SYSCatalogGroup" );
        groupList.remove( "SYSCoord" );
        groupList.remove( "SYSSpare" );
        return groupList;
    }
}
