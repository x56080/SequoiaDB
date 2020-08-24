package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: doesObjectExist查询其它用户的对象 testlink-case: seqDB-16670
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestDoesObjectExist16670 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16670";
    private String userNameA = "user16670a";
    private String userNameB = "user16670b";
    private String roleName = "normal";
    private String keyName = "key16670";
    private String content = "content16670";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userNameA );
        CommLib.clearUser( userNameB );
        String[] accessKeysA = UserUtils.createUser( userNameA, roleName );
        s3ClientA = CommLib.buildS3Client( accessKeysA[ 0 ], accessKeysA[ 1 ] );
        String[] accessKeysB = UserUtils.createUser( userNameB, roleName );
        s3ClientB = CommLib.buildS3Client( accessKeysB[ 0 ], accessKeysB[ 1 ] );
    }

    @Test
    private void testDoesObjectExist() throws Exception {
        s3ClientA.createBucket( bucketName );
        s3ClientA.putObject( bucketName, keyName, content );
        try {
            s3ClientB.doesObjectExist( bucketName, keyName );
            Assert.fail( "exp fail but found success!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 403 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3ClientA != null ) {
                s3ClientA.shutdown();
            }
            if ( s3ClientB != null ) {
                s3ClientB.shutdown();
            }
        }
    }
}
