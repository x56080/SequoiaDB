package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @Description: 设置不同版本控制状态，删除对象
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */

public class DeleteObject16458 extends S3TestBase {
    private String bucketName = "bucket16458";
    private String keyName = "testkey16458";
    private String file = "object16458";
    private String[] versionId = new String[ 2 ];
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, keyName, file );
    }

    @Test
    public void testGetObjectList() throws Exception {
        PutObjectResult result = null;
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        result = s3Client.putObject( bucketName, keyName, file );
        versionId[ 0 ] = result.getVersionId();

        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        s3Client.putObject( bucketName, keyName, file );

        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        result = s3Client.putObject( bucketName, keyName, file );
        versionId[ 1 ] = result.getVersionId();

        // delete object with versions
        for ( int i = 0; i < versionId.length; i++ ) {
            s3Client.deleteVersion( bucketName, keyName, versionId[ i ] );
        }
        s3Client.deleteVersion( bucketName, keyName, "null" );
        // check the object version list
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List< S3VersionSummary > verList = versionList.getVersionSummaries();
        Assert.assertEquals( verList.size(), 0, "object is still exist!" );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            s3Client.deleteBucket( bucketName );
        }
    }
}