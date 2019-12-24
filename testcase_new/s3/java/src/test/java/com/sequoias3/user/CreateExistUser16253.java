package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.json.XML;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-16250 :: 管理员创建管理员（admin）用户
 * @author fanyu
 * @Date:2018年10月29日
 * @version:1.0
 */

public class CreateExistUser16253 extends S3TestBase {
    private String username = "CreateAdminUser16253";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( username, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    private void test() {
        // create user
        UserUtils.createUser( username, UserCommDefind.admin,
                UserUtils.accessKeyId );
        // create user again
        try {
            UserUtils.createUser( username, UserCommDefind.admin,
                    UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            org.json.JSONObject json = XML.toJSONObject( errorMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "UserAlreadyExists" ) ) {
                Assert.fail( e.getMessage() );
            }
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( username, UserUtils.accessKeyId, true );
        }
    }
}
