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
 * @Description: 不带versionId查询删除标记的对象 testlink-case: seqDB-16678
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestGetObjectMetadata16678 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16678";
    private String userName = "user16678";
    private String roleName = "normal";
    private String keyName = "key16678";
    private String content = "content16678";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testGetObjectMetadata() throws Exception {
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, content );
        s3Client.deleteObject( bucketName, keyName );
        try {
            s3Client.getObjectMetadata( bucketName, keyName );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }
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
