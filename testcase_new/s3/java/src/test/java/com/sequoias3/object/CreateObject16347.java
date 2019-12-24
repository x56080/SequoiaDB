package com.sequoias3.object;

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
 * test content: 非桶管理用户增加对象 testlink-case: seqDB-16347
 *
 * @author wangkexin
 * @Date 2018.11.13
 * @version 1.00
 */
public class CreateObject16347 extends S3TestBase {
    private String userNameA = "UserA16347";
    private String userNameB = "UserB16347";
    private String roleName = "normal";
    private String bucketName = "bucket16347";
    private String keyName = "/aa/bb/object16347.png";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;
    private String expContent = "object_file16347";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        // create user A
        String[] acessKeys = UserUtils.createUser( userNameA, roleName );
        s3ClientA = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        // create bucket
        s3ClientA.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testPutObject() throws Exception {
        // create user B
        String[] acessKeys = UserUtils.createUser( userNameB, roleName );
        s3ClientB = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        try {
            s3ClientB.putObject( bucketName, keyName, expContent );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        } finally {
            if ( s3ClientB != null ) {
                s3ClientB.shutdown();
            }
        }
        checkPutObjResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            try {
                s3ClientA.deleteBucket( bucketName );
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
            } catch ( Exception e ) {
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

    private void checkPutObjResult() {
        try {
            s3ClientA.getObject( bucketName, keyName );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
    }
}
