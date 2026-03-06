package com.sequoiadb.sdbschedule;

import com.sequoiadb.sdbschedule.utils.BsonUtils;
import com.sequoiadb.sdbschedule.utils.BusinessApiFactory;
import com.sequoiadb.testcommon.SdbTestBase;
import io.restassured.response.Response;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @Dscription Test of node operation rest api
 */
public class SdbSchedule_34374 extends SdbTestBase {

    private List< String > nodeAddrList;

    @BeforeClass()
    public void setUp() {
        nodeAddrList = getSdbScheduleNodes();
    }

    @Test
    public void test() {
        Response resp = BusinessApiFactory.Node.list( 0, 10 );
        Assert.assertEquals( resp.statusCode(), 200,
                "Node list response code is not 200" );
        BasicBSONList nodeList = ( BasicBSONList ) BsonUtils
                .fromResponse( resp );
        int leaderNum = 0;
        for ( Object obj : nodeList ) {
            BSONObject node = ( BSONObject ) obj;
            Boolean isLeader = ( Boolean ) node.get( "is_leader" );
            if ( isLeader != null && isLeader ) {
                leaderNum++;
            }

            String hostName = ( String ) node.get( "host_name" );
            String ipAddr = ( String ) node.get( "ip_Addr" );
            Integer port = ( Integer ) node.get( "port" );
            if ( !nodeAddrList.contains( ipAddr + ":" + port )
                    && !nodeAddrList.contains( hostName + ":" + port ) ) {
                Assert.fail( "Node " + ipAddr + ":" + port
                        + " is not in the expected node list" );
            }
        }
        Assert.assertEquals( leaderNum, 1,
                "There should be exactly one leader node" );
    }

    @AfterClass()
    public void tearDown() {

    }
}
