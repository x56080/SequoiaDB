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
 * test content: 非桶管理用户删除对象
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */

public class DeleteObject16455 extends S3TestBase {
    private String bucketName = "bucket16455";
    private String keyName = "testkey16455";
    private String file = "object16455";
    private String userNameA = "user16455a";
    private String userNameB = "user16455b";
    private String roleName = "normal";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;
    private String[] accessKeysA = null;
    private String[] accessKeysB = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        // create user A , user B
        accessKeysA = UserUtils.createUser( userNameA, roleName );
        s3ClientA = CommLib.buildS3Client( accessKeysA[ 0 ], accessKeysA[ 1 ] );

        accessKeysB = UserUtils.createUser( userNameB, roleName );
        s3ClientB = CommLib.buildS3Client( accessKeysB[ 0 ], accessKeysB[ 1 ] );

        // create bucket
        s3ClientA.createBucket( new CreateBucketRequest( bucketName ) );
        s3ClientA.putObject( bucketName, keyName, file );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // delete object by user b
        try {
            s3ClientB.deleteObject( bucketName, keyName );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        // check the result
        boolean isObjexist = s3ClientA.doesObjectExist( bucketName, keyName );
        Assert.assertTrue( isObjexist, "the object should be exist!" );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
            }
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
