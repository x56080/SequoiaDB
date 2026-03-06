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

/**
 * @Dscription Test of site operation rest api
 */
public class SdbSchedule_34375 extends SdbTestBase {

    private String datasourceSite;

    @BeforeClass()
    public void setUp() {
        datasourceSite = getDatasourceSiteName();
    }

    @Test
    public void test() {
        Response resp = BusinessApiFactory.Site.list( 0, 10 );
        Assert.assertEquals( resp.statusCode(), 200,
                "Site list response code is not 200" );
        BasicBSONList siteList = ( BasicBSONList ) BsonUtils
                .fromResponse( resp );
        Assert.assertTrue( siteList.size() >= 2,
                "There should be at least two sites" );
        int rootSiteNum = 0;
        boolean foundDatasourceSite = false;
        for ( Object obj : siteList ) {
            BSONObject site = ( BSONObject ) obj;
            String name = ( String ) site.get( "name" );
            if ( "rootsite".equals( name ) ) {
                rootSiteNum++;
            } else {
                String datasource = ( String ) site.get( "datasource" );
                Assert.assertNotNull( datasource,
                        "Non-rootsite site should have datasource name info" );
                if ( datasourceSite.equals( name ) ) {
                    foundDatasourceSite = true;
                }
            }
        }

        Assert.assertEquals( rootSiteNum, 1,
                "There should be exactly one rootsite" );
        Assert.assertTrue( foundDatasourceSite, "Datasource site "
                + datasourceSite + " not found in site list" );
    }

    @AfterClass()
    public void tearDown() {

    }
}
