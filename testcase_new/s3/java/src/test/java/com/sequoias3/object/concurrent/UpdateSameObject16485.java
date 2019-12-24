package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * test content: 对象已存在，并发更新相同对象 testlink-case: seqDB-16485
 *
 * @author wangkexin
 * @Date 2018.12.18
 * @version 1.00
 */
public class UpdateSameObject16485 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16485";
    private String bucketName = "bucket16485";
    private String keyName = "key16485";
    private String roleName = "normal";
    private String oldContent = "testContentold16485";
    private String newContent = "testContentnew16485";
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, oldContent );
    }

    @Test
    public void testUpdateObject() throws Exception {
        UpdateObjectThread updateSameObject = new UpdateObjectThread();
        updateSameObject.start( 50 );

        Assert.assertTrue( updateSameObject.isSuccess(),
                updateSameObject.getErrorMsg() );

        checkUpdateObjectResult( s3Client );
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

    private void checkUpdateObjectResult( AmazonS3 s3Client )
            throws IOException {
        ListObjectsV2Result listObjectsV2Result = s3Client
                .listObjectsV2( bucketName );
        Assert.assertEquals( listObjectsV2Result.getObjectSummaries().size(),
                1 );
        Assert.assertEquals( listObjectsV2Result.getObjectSummaries().get( 0 )
                .getBucketName(), bucketName, "bucketName is wrong!" );
        Assert.assertEquals(
                listObjectsV2Result.getObjectSummaries().get( 0 ).getKey(),
                keyName, "keyName is wrong!" );
        Assert.assertEquals(
                listObjectsV2Result.getObjectSummaries().get( 0 ).getETag(),
                TestTools.getMD5( newContent.getBytes() ),
                "content is wrong!" );

    }

    private class UpdateObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                s3Client.putObject( bucketName, keyName,
                        "testContentnew16485" );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
