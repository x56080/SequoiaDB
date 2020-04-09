package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * test content: 桶版本控制状态切换 testlink-case: seqDB-16612
 *
 * @author wangkexin
 * @Date 2018.11.19
 * @version 1.00
 */

public class SetBucketVersioning16612 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16612";
    private String userName = "user16612";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
    }

    @Test
    private void testSwitchBucketVersioning() throws Exception {
        // set bucket versioning status
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put some objects
        List< PutObjectResult > objectResults = new ArrayList< PutObjectResult >();
        objectResults.add( s3Client.putObject( bucketName, "test1",
                "object1_file16612" ) );
        objectResults.add( s3Client.putObject( bucketName, "test2",
                "object2_file16612" ) );
        objectResults.add( s3Client.putObject( bucketName, "test3",
                "object3_file16612" ) );
        checkListObjectsV2Result( objectResults, true );

        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        List< PutObjectResult > objectResults2 = new ArrayList< PutObjectResult >();
        objectResults2.add( s3Client.putObject( bucketName, "test1",
                "object1_file16612" ) );
        objectResults2.add( s3Client.putObject( bucketName, "test2",
                "object2_file16612" ) );
        objectResults2.add( s3Client.putObject( bucketName, "test3",
                "object3_file16612" ) );
        checkListObjectsV2Result( objectResults2, false );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkListObjectsV2Result(
            List< PutObjectResult > objectResults, boolean VersionIdNotNull ) {
        if ( VersionIdNotNull ) {
            for ( PutObjectResult objectResult : objectResults ) {
                Assert.assertNotEquals( objectResult.getVersionId(), "null" );
            }
        } else {
            for ( PutObjectResult objectResult : objectResults ) {
                Assert.assertEquals( objectResult.getVersionId(), "null" );
            }
        }
    }
}
