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
 * @Description: 存在多个版本对象，doesObjectExist查询对象 testlink-case: seqDB-16672
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestDoesObjectExist16672 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16672";
    private String userName = "user16672";
    private String roleName = "normal";
    private String keyName = "key16672";
    private String content = "content16672";
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
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, content );
        s3Client.putObject( bucketName, keyName, content );
        Assert.assertTrue( s3Client.doesObjectExist( bucketName, keyName ) );
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
