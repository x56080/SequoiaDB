package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-16259 :: 管理员更新不存在的用户
 * @author wangkexin
 * @Date:2018年10月31日
 * @version:1.0
 */

public class UpdateNonexistentUser16259 extends S3TestBase {
    private String userName = "UpdateNonexistentUser16259";
    private String updateUsername = "NonexistentUser16259";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        try {
            UserUtils.deleteUser( updateUsername, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    public void test() throws JSONException {
        // create an admin user
        UserUtils.createUser( userName, UserCommDefind.admin,
                UserUtils.accessKeyId );

        // there is no such user here.
        try {
            UserUtils.updateUser( updateUsername, UserUtils.accessKeyId );
            Assert.fail( "update user should be failed!" );
        } catch ( HttpClientErrorException e ) {
            JSONObject json = XML.toJSONObject( e.getResponseBodyAsString() );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "NoSuchUser" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        }
    }
}
