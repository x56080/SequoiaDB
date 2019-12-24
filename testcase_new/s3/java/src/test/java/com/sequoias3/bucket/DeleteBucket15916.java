package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-15916:delete bucket,the bucket existing object
 * @author wuyan
 * @Date 2018.10.10
 * @version 1.00
 */
public class DeleteBucket15916 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket15916";
    private String key = "key15916";
    private String userName = "user15916";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, key, "testdeletebucket" );
    }

    @SuppressWarnings("deprecation")
    @Test
    public void testCreateBucket() throws Exception {
        try {
            s3Client.deleteBucket( bucketName );
            Assert.fail( "delete bucket must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "BucketNotEmpty" );
        }

        Assert.assertTrue( s3Client.doesBucketExist( bucketName ),
                "the bucket is exist!" );
        Assert.assertTrue( s3Client.doesObjectExist( bucketName, key ),
                "the object is exist!" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
