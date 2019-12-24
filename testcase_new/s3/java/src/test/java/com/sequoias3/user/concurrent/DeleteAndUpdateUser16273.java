package com.sequoias3.user.concurrent;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.json.XML;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description:seqDB-16273 :: 并发删除用户和更新用户
 * @author fanyu
 * @Date:2018年10月30日
 * @version:1.0
 */

public class DeleteAndUpdateUser16273 extends S3TestBase {
    private String userName = "DeleteAndUpdateUser16273";
    private String bucketName = "bucketName";

    private int num = 50;
    private List<String> nameList = new ArrayList<String>();

    @BeforeClass
    private void setUp() throws Exception {
        for ( int i = 0; i < num; i++ ) {
            try {
                nameList.add( userName + "." + i );
                UserUtils.deleteUser( userName + "." + i, UserUtils.accessKeyId,
                        true );
            } catch ( HttpClientErrorException e ) {
                if ( e.getStatusCode() != HttpStatus.NOT_FOUND ) {
                    Assert.fail( e.getMessage() );
                }
            }
        }

        for ( int i = 0; i < num; i++ ) {
            UserUtils.createUser( nameList.get( i ), UserCommDefind.normal,
                    UserUtils.accessKeyId );
        }
    }

    @Test
    public void test() throws Exception {
        DeleteUser dThread = new DeleteUser();
        UpdateUser uTread = new UpdateUser();
        dThread.start();
        uTread.start();
        Assert.assertTrue( dThread.isSuccess(), dThread.getErrorMsg() );
        Assert.assertTrue( uTread.isSuccess(), uTread.getErrorMsg() );

        // check
        checkResult();
    }

    @AfterClass
    private void tearDown() throws Exception {
    }

    private void checkResult() {
        for ( int i = 0; i < num; i++ ) {
            try {
                UserUtils.getUser( nameList.get( i ), UserUtils.accessKeyId );
                Assert.fail( "exp fail but act success" );
            } catch ( HttpClientErrorException e ) {
                String errorMsg = e.getResponseBodyAsString();
                org.json.JSONObject json1 = XML.toJSONObject( errorMsg );
                if ( !json1.getJSONObject( UserCommDefind.error )
                        .getString( UserCommDefind.errorCode )
                        .contains( "NoSuchUser" ) ) {
                    Assert.fail( e.getMessage() );
                }
            }
        }
    }

    public class DeleteUser extends S3ThreadBase {
        @Override
        public void exec() {
            for ( int i = 0; i < num; i++ ) {
                UserUtils.deleteUser( nameList.get( i ), UserUtils.accessKeyId,
                        true );
            }
        }
    }

    public class UpdateUser extends S3ThreadBase {
        @Override
        public void exec() {
            try {
                for ( int i = num - 1; i >= 0; i-- ) {
                    UserUtils.updateUser( nameList.get( i ),
                            UserUtils.accessKeyId );
                }
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
}
