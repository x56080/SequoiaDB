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
 * @Dscription Test of global configuration rest api
 */
public class SdbSchedule_34373 extends SdbTestBase {

    private String checkedConfKey = "task_record_retention_days";
    private String defaultConfValue = "30";

    @BeforeClass()
    public void setUp() {
    }

    @Test
    public void test() {
        // check default conf
        checkDefault();

        // update conf
        updateConf( checkedConfKey, "60" );

        // update invalid conf
        updateConfWithInvalidConf( checkedConfKey, "ABC" );

        // restore default conf
        updateConf( checkedConfKey, defaultConfValue );
    }

    @AfterClass()
    public void tearDown() {

    }

    private void updateConfWithInvalidConf( String confKey, String newValue ) {
        Response resp = BusinessApiFactory.GlobalConf.update( confKey,
                newValue );
        Assert.assertEquals( resp.getStatusCode(), 500,
                "Update invalid GlobalConf response code is not 500" );
    }

    private void updateConf( String confKey, String newValue ) {
        Response resp = BusinessApiFactory.GlobalConf.update( confKey,
                newValue );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "GlobalConf update response code is not 200" );

        resp = BusinessApiFactory.GlobalConf.list( 0, 10 );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "GlobalConf list response code is not 200" );

        BasicBSONList confList = ( BasicBSONList ) BsonUtils
                .fromResponse( resp );
        Assert.assertNotNull( confList, "GlobalConf list response is null" );
        Assert.assertEquals( confList.size(), 1,
                "GlobalConf list size is not 1" );
        BSONObject conf = ( BSONObject ) confList.get( 0 );
        String confValue = ( String ) conf.get( "conf_value" );

        Assert.assertEquals( confValue, newValue,
                "conf value is not match after update" );
    }

    private void checkDefault() {
        Response resp = BusinessApiFactory.GlobalConf.list( 0, 10 );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "GlobalConf list response code is not 200" );
        BasicBSONList confList = ( BasicBSONList ) BsonUtils
                .fromResponse( resp );
        Assert.assertNotNull( confList, "GlobalConf list response is null" );
        Assert.assertEquals( confList.size(), 1,
                "GlobalConf list size is not 1" );
        BSONObject conf = ( BSONObject ) confList.get( 0 );
        String confKey = ( String ) conf.get( "conf_key" );
        String confValue = ( String ) conf.get( "conf_value" );

        Assert.assertEquals( confKey, "task_record_retention_days",
                "conf key is not match" );
        Assert.assertEquals( confValue, defaultConfValue,
                "conf value is not match" );
    }
}
