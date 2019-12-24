package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @author wangkexin
 * @Description: seqDB-16260:管理员非强制删除管理员用户
 * @Date:2018年11月06日
 * @version:1.0
 */

public class DeleteAdminUser16260 extends S3TestBase {
    private String adminName = "DeleteAdminUser16260";
    private String deleteAdminName = "ToBeDeleteAdminUser16260";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( adminName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                Assert.fail( e.getMessage() );
            }
        }
        try {
            UserUtils
                    .deleteUser( deleteAdminName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    private void test() {
        // create user
        JSONObject adminUserJSON = UserUtils
                .createUser( adminName, UserCommDefind.admin,
                        UserUtils.accessKeyId );

        // get the accessKeyID and secretAccessKey from userJSON
        JSONObject jsonAdmin = adminUserJSON
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyIDAdmin = jsonAdmin
                .getString( UserCommDefind.accessKeyID );

        // admin user create admin user
        UserUtils.createUser( deleteAdminName, UserCommDefind.admin,
                accessKeyIDAdmin );

        // delete admin user
        UserUtils.deleteUser( deleteAdminName, accessKeyIDAdmin );

        // check: admin user does not exist;
        checkDeletedAdminUser( deleteAdminName, accessKeyIDAdmin );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( adminName, UserUtils.accessKeyId, true );
        }
    }

    private void checkDeletedAdminUser( String name, String accessKeyID ) {
        try {
            UserUtils.getUser( name, accessKeyID );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            org.json.JSONObject json1 = XML.toJSONObject( errorMsg );
            if ( !json1.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "NoSuchUser" ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }
}