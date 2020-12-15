package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-16264 :: 普通用户删除用户
 * @author wangkexin
 * @Date:2018年11月02日
 * @version:1.0
 */

public class DeleteUser16264 extends S3TestBase {
    private String name = "DeleteUser16264";
    private String deleteUserName = "ToDelete16264";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        try {
            UserUtils.deleteUser( deleteUserName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    private void test() {
        // create user
        JSONObject userJSON = UserUtils.createUser( name, UserCommDefind.normal,
                UserUtils.accessKeyId );

        // get the accessKeyID and secretAccessKey from userJSON
        JSONObject json = userJSON.getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = json.getString( UserCommDefind.accessKeyID );

        // create user to be deleted
        UserUtils.createUser( deleteUserName, UserCommDefind.normal,
                UserUtils.accessKeyId );

        // delete user
        try {
            UserUtils.deleteUser( deleteUserName, accessKeyID );
            Assert.fail( "exp fail but found success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            org.json.JSONObject json1 = XML.toJSONObject( errorMsg );
            if ( !json1.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "AccessDenied" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( name, UserUtils.accessKeyId, true );
            UserUtils.deleteUser( deleteUserName, UserUtils.accessKeyId, true );
        }
    }
}
