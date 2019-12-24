package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: doesObjectExist查询指定桶不存在 testlink-case: seqDB-16671
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestDoesObjectExist16671 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16671";
    private String nonexistBucketName = "nonexistbucket16671";
    private String userName = "user16671";
    private String roleName = "normal";
    private String keyName = "key16671";
    private String content = "content16671";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testDoesObjectExist() throws Exception {
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, content );

        // the specified bucket name does not exist
        Assert.assertFalse(
                s3Client.doesObjectExist( nonexistBucketName, keyName ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
