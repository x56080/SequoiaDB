package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 带versionId查询删除标记的对象 testlink-case: seqDB-16685
 *
 * @author wangkexin
 * @Date 2018.12.10
 * @version 1.00
 */

public class TestGetObjectMetadata16685 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16685";
    private String userName = "user16685";
    private String roleName = "normal";
    private String keyName = "key16685";
    private String content = "content16685";
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testGetObjectMetadata() throws Exception {
        String newestVersionId = null;
        String historyVersionId = null;

        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, content );
        s3Client.deleteObject( bucketName, keyName );
        s3Client.deleteObject( bucketName, keyName );
        s3Client.deleteObject( bucketName, keyName );

        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List<S3VersionSummary> verList = versionList.getVersionSummaries();
        for ( S3VersionSummary version : verList ) {
            if ( version.isDeleteMarker() ) {
                newestVersionId = version.getVersionId();
                break;
            }
        }
        historyVersionId = verList.get( verList.size() - 1 ).getVersionId();
        try {
            s3Client.getObjectMetadata(
                    new GetObjectMetadataRequest( bucketName, keyName,
                            newestVersionId ) );
            Assert.fail(
                    "get the object with the latest deletemarker should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404,
                    "get the object with the latest deletemarker is wrong" );
        }

        try {
            s3Client.getObjectMetadata(
                    new GetObjectMetadataRequest( bucketName, keyName,
                            historyVersionId ) );
            Assert.fail(
                    "get the object with history deletemarker should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404,
                    "get the object with history deletemarker is wrong" );
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
