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
 * @Description: seqDB-16262 :: 管理员删除自身
 * @author fanyu
 * @Date:2018年10月30日
 * @version:1.0
 */

public class DeleteSelf16262 extends S3TestBase {
    private String name = "DeleteSelf16262";
    private String accessKeyID = null;

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
    }

    @Test
    private void test() {
        // create user
        JSONObject userJSON = UserUtils.createUser( name, UserCommDefind.admin,
                UserUtils.accessKeyId );

        // get the accessKeyID from userJSON
        JSONObject json = userJSON.getJSONObject( UserCommDefind.accessKeys );
        accessKeyID = json.getString( UserCommDefind.accessKeyID );

        // delete user
        UserUtils.deleteUser( name, accessKeyID );

        // check
        try {
            UserUtils.getUser( name, UserUtils.accessKeyId );
            Assert.fail( "exp fail but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json1 = XML.toJSONObject( errorMsg );
            if ( !json1.getJSONObject( UserCommDefind.error )
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
