package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

import org.json.JSONException;
import org.json.XML;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-16265 :: 管理员删除不存在的用户
 * @author fanyu
 * @Date:2018年10月29日
 * @version:1.0
 */

public class DeleteInexistenceUser16265 extends S3TestBase {
    private String name = "DeleteInexistenceUser16265";

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    private void test() throws JSONException {
        // delete user again
        try {
            UserUtils.deleteUser( name, UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            org.json.JSONObject json = XML.toJSONObject( errorMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "NoSuchUser" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }

    @AfterClass
    private void tearDown() {
    }
}
