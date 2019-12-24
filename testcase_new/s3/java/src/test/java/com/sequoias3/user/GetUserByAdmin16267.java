package com.sequoias3.user;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.json.JSONObject;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-16267 :: 管理员获取用户
 * @author fanyu
 * @Date:2018年10月30日
 * @version:1.0
 */

public class GetUserByAdmin16267 extends S3TestBase {
    private String userName1 = "GetUser16267_A";
    private String userName2 = "GetUser16267_B";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( userName1, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }

        try {
            UserUtils.deleteUser( userName2, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
            }
        }
    }

    @Test
    private void test() {
        // create user
        JSONObject expUser1 = UserUtils
                .createUser( userName1, UserCommDefind.admin,
                        UserUtils.accessKeyId );
        JSONObject expUser2 = UserUtils
                .createUser( userName2, UserCommDefind.normal,
                        UserUtils.accessKeyId );

        // get user
        JSONObject actUser1 = UserUtils
                .getUser( userName1, UserUtils.accessKeyId );
        JSONObject actUser2 = UserUtils
                .getUser( userName2, UserUtils.accessKeyId );

        // check
        checkResult( actUser1, expUser1 );
        checkResult( actUser2, expUser2 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            UserUtils.deleteUser( userName1, UserUtils.accessKeyId, true );
            UserUtils.deleteUser( userName2, UserUtils.accessKeyId, true );
        }
    }

    private void checkResult( JSONObject actUser, JSONObject expUser ) {
        JSONObject actJSON = actUser.getJSONObject( UserCommDefind.accessKeys );
        JSONObject expJSON = expUser.getJSONObject( UserCommDefind.accessKeys );
        Assert.assertEquals( actJSON.getString( UserCommDefind.accessKeyID ),
                expJSON.getString( UserCommDefind.accessKeyID ),
                "actJSON = " + actJSON + ",expJSON = " + expJSON );
        Assert.assertEquals(
                actJSON.getString( UserCommDefind.secretAccessKey ),
                expJSON.getString( UserCommDefind.secretAccessKey ),
                "actJSON = " + actJSON + ",expJSON = " + expJSON );
    }
}
