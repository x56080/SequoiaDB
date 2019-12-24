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
 * @Description: seqDB-16269 :: 普通用户获取用户
 * @author fanyu
 * @Date:2018年10月30日
 * @version:1.0
 */

public class GetUserByNormal16269 extends S3TestBase {
    private String userName = "GetUserByNormal16269";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            UserUtils.deleteUser( userName, UserUtils.accessKeyId, true );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode() != ( HttpStatus.NOT_FOUND ) ) {
                throw e;
            }
        }
    }

    @Test
    private void test() {
        // create user
        JSONObject userJSON = UserUtils
                .createUser( userName, UserCommDefind.normal,
                        UserUtils.accessKeyId );
        String accessKeyId = userJSON.getJSONObject( UserCommDefind.accessKeys )
                .getString( UserCommDefind.accessKeyID );

        try {
            // get user
            UserUtils.getUser( userName, accessKeyId );
            Assert.fail( "exp success but act success" );
        } catch ( HttpClientErrorException e ) {
            String errorMsg = e.getResponseBodyAsString();
            JSONObject json1 = XML.toJSONObject( errorMsg );
            if ( !json1.getJSONObject( UserCommDefind.error )
                    .getString( UserCommDefind.errorCode )
                    .contains( "AccessDenied" ) ) {
                throw e;
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
