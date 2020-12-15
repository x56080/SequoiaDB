package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-18096: get delimiter,specified bucket does not exist.
 * @author wuyan
 * @Date 2019.04.10
 * @version 1.00
 */
public class UpdateDelimiter18096 extends S3TestBase {
    private boolean runSuccess = false;
    private String userNameA = "UserA18096";
    private String userNameB = "UserB18096";
    private String roleName = "normal";
    private String bucketName = "bucket18096";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;

    @BeforeClass
    private void setUp() throws IOException {
        // create user A
        String[] acessKeys = UserUtils.createUser( userNameA, roleName );
        s3ClientA = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        // create bucket
        s3ClientA.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testUpdateDelimiter() throws Exception {
        // create user B
        String[] acessKeys = UserUtils.createUser( userNameB, roleName );
        s3ClientB = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        try {
            DelimiterUtils.getDelimiter( bucketName, acessKeys[ 0 ] );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied",
                    "errorCode is " + e.getErrorCode() + "  statusCode:"
                            + e.getStatusCode() );
        } finally {
            if ( s3ClientB != null ) {
                s3ClientB.shutdown();
            }
        }
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
        }
    }
}
