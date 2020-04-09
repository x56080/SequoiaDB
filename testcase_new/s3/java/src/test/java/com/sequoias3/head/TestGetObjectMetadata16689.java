package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 指定vesion查询对象，其中指定key不存在 testlink-case: seqDB-16689
 *
 * @author wangkexin
 * @Date 2018.12.10
 * @version 1.00
 */

public class TestGetObjectMetadata16689 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16689";
    private String userName = "user16689";
    private String roleName = "normal";
    private String keyName = "key16689";
    private String nonExistKeyName = "nonexistkeyname16689";
    private String content = "content16689";
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
        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                content );
        String versionid = result.getVersionId();

        // 查询不存在的对象keyB,指定versionid为已存在的versionid
        try {
            s3Client.getObjectMetadata( new GetObjectMetadataRequest(
                    bucketName, nonExistKeyName, versionid ) );
            Assert.fail( "exp fail but found success" );
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
