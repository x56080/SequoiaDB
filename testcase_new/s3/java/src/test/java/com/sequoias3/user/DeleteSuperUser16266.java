package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.json.JSONException;
import org.json.XML;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-16266 :: 管理员删除超级管理员用户
 * @author fanyu
 * @Date:2018年10月29日
 * @version:1.0
 */

public class DeleteSuperUser16266 extends S3TestBase {
    private String superUserName = "administrator";

    @BeforeClass
    private void setUp() {
    }

    @Test
    private void test() throws JSONException {
        // delete super user
        try {
            UserUtils.deleteUser( superUserName, UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            org.json.JSONObject json = XML.toJSONObject( errorMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "InitAdminCannotDelete" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }

    @AfterClass
    private void tearDown() {
    }
}
