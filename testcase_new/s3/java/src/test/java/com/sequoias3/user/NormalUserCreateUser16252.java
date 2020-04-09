package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

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
 * @Description: seqDB-16252 :: 普通（normal）用户创建用户（管理员用户，普通用户）
 * @author wangkexin
 * @Date:2018年10月31日
 * @version:1.0
 */

public class NormalUserCreateUser16252 extends S3TestBase {
    private String userName = "CreateAdminUser16252";
    private String normalUserName = "normalUser16252";
    private String adminUserName = "adminUser16252";
    private String accessKeyID = null;
    private String secretAccessKey = null;
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
    }

    @Test
    public void test() throws JSONException {
        // create user
        JSONObject actUser = UserUtils.createUser( userName,
                UserCommDefind.normal, UserUtils.accessKeyId );
        // check create user result
        checkUser( actUser );
        // create bucket for check
        checkByCreateUser();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        }
    }

    private void checkUser( JSONObject actUser ) {
        // get user
        JSONObject expUser = UserUtils.getUser( userName,
                UserUtils.accessKeyId );

        JSONObject actJSON = actUser.getJSONObject( UserCommDefind.accessKeys );
        JSONObject expJSON = expUser.getJSONObject( UserCommDefind.accessKeys );

        // check
        accessKeyID = actJSON.getString( UserCommDefind.accessKeyID );
        secretAccessKey = actJSON.getString( UserCommDefind.secretAccessKey );
        Assert.assertEquals( accessKeyID,
                expJSON.getString( UserCommDefind.accessKeyID ),
                "actJSON = " + actJSON.toString() + ",expJSON = "
                        + expJSON.toString() );
        Assert.assertEquals( secretAccessKey,
                expJSON.getString( UserCommDefind.secretAccessKey ),
                "actJSON = " + actJSON.toString() + ",expJSON = "
                        + expJSON.toString() );
    }

    private void checkByCreateUser() {
        // create normal user
        try {
            UserUtils.createUser( normalUserName, UserCommDefind.normal,
                    accessKeyID );
            Assert.fail( "create normal user should fail!" );
        } catch ( HttpClientErrorException e ) {
            String errMsg = e.getResponseBodyAsString();
            JSONObject json = XML.toJSONObject( errMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "AccessDenied" ) ) {
                Assert.fail( e.getMessage() );
            }
        }
        // create admin user
        try {
            UserUtils.createUser( adminUserName, UserCommDefind.admin,
                    accessKeyID );
            Assert.fail( "create admin user should fail!" );
        } catch ( HttpClientErrorException e ) {
            String errMsg = e.getResponseBodyAsString();
            JSONObject json = XML.toJSONObject( errMsg );
            if ( !json.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "AccessDenied" ) ) {
                Assert.fail( e.getMessage() );
            }
        }
    }
}
