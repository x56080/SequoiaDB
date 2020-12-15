package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-19325:非桶管理用户复制对象
 * @author wuyan
 * @Date 2019.09.18
 * @version 1.00
 */
public class CopyObject19325 extends S3TestBase {
    private boolean runSuccess = false;
    private String userNameA = "UserA19325";
    private String userNameB = "UserB19325";
    private String roleName = "normal";
    private String srcKeyName = "/src/object19325";
    private String destKeyName = "/dest/object19325";
    private String srcBucketName = "srcbucket19325";
    private String destBucketName = "destbucket19325";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;

    @BeforeClass
    private void setUp() {
        CommLib.clearUser( userNameA );
        CommLib.clearUser( userNameB );
        String[] acessKeys1 = UserUtils.createUser( userNameA, roleName );
        String[] acessKeys2 = UserUtils.createUser( userNameB, roleName );
        s3ClientA = CommLib.buildS3Client( acessKeys1[ 0 ], acessKeys1[ 1 ] );
        s3ClientB = CommLib.buildS3Client( acessKeys2[ 0 ], acessKeys2[ 1 ] );

        s3ClientA.createBucket( srcBucketName );
        s3ClientB.createBucket( destBucketName );
        s3ClientA.putObject( srcBucketName, srcKeyName, "test copy object." );
    }

    @Test
    public void testCopyObject() throws Exception {

        try {
            CopyObjectRequest request = new CopyObjectRequest( srcBucketName,
                    srcKeyName, destBucketName, destKeyName );
            s3ClientB.copyObject( request );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied",
                    e.getStatusCode() + e.getErrorMessage() );
        }

        Assert.assertFalse(
                s3ClientB.doesObjectExist( destBucketName, destKeyName ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
            }
        } finally {
            s3ClientA.shutdown();
            s3ClientB.shutdown();
        }
    }

}
