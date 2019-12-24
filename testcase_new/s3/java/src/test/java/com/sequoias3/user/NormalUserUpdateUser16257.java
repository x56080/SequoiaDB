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
 * @Description: seqDB-16257 :: 普通用户更新用户（管理员用户，普通用户）
 * @author wangkexin
 * @Date:2018年10月31日
 * @version:1.0
 */

public class NormalUserUpdateUser16257 extends S3TestBase {
    private String userName = "NormalUserUpdateUser16257";
    private String adminUserName = "adminUser16257";
    private String normalUserName = "normalUser16257";
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
            UserUtils.deleteUser( adminUserName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        try {
            UserUtils.deleteUser( normalUserName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    public void test() throws JSONException {
        // create user
        JSONObject testUser = UserUtils
                .createUser( userName, UserCommDefind.normal,
                        UserUtils.accessKeyId );
        JSONObject adminUser = UserUtils
                .createUser( adminUserName, UserCommDefind.admin,
                        UserUtils.accessKeyId );
        JSONObject normalUser = UserUtils
                .createUser( normalUserName, UserCommDefind.normal,
                        UserUtils.accessKeyId );

        JSONObject actJSON = testUser
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = actJSON.getString( UserCommDefind.accessKeyID );

        // update user
        try {
            UserUtils.updateUser( userName, accessKeyID );
            Assert.fail( "Self updating should fail!" );
        } catch ( HttpClientErrorException e ) {
            JSONObject json = XML.toJSONObject( e.getResponseBodyAsString() );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "AccessDenied" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        JSONObject expUser = UserUtils
                .getUser( userName, UserUtils.accessKeyId );
        checkResult( testUser, expUser );
        // update admin user
        try {
            UserUtils.updateUser( adminUserName, accessKeyID );
            Assert.fail( "update admin user should be failed!" );
        } catch ( HttpClientErrorException e ) {
            JSONObject json = XML.toJSONObject( e.getResponseBodyAsString() );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "AccessDenied" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        JSONObject expAdminUser = UserUtils
                .getUser( adminUserName, UserUtils.accessKeyId );
        checkResult( adminUser, expAdminUser );

        // update normal user
        try {
            UserUtils.updateUser( normalUserName, accessKeyID );
            Assert.fail( "update normal user should be failed!" );
        } catch ( HttpClientErrorException e ) {
            JSONObject json = XML.toJSONObject( e.getResponseBodyAsString() );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "AccessDenied" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        JSONObject expNormalUser = UserUtils
                .getUser( normalUserName, UserUtils.accessKeyId );
        checkResult( normalUser, expNormalUser );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
            UserUtils.deleteUser( adminUserName, UserUtils.accessKeyId, true );
            UserUtils.deleteUser( normalUserName, UserUtils.accessKeyId, true );
        }
    }

    private void checkResult( JSONObject craeteUser, JSONObject updateUser ) {
        JSONObject createJSON = craeteUser
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyIdOld = createJSON
                .getString( UserCommDefind.accessKeyID );
        String secretAccessKeyOld = createJSON
                .getString( UserCommDefind.secretAccessKey );

        JSONObject updateJSON = updateUser
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = updateJSON.getString( UserCommDefind.accessKeyID );
        String secretAccessKey = updateJSON
                .getString( UserCommDefind.secretAccessKey );

        // check accessKeyID and secretAccessKey was updated failed.
        Assert.assertEquals( accessKeyIdOld, accessKeyID );
        Assert.assertEquals( secretAccessKeyOld, secretAccessKey );
    }
}
