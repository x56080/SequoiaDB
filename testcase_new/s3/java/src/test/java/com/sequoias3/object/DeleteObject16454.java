package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
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
 * test content: 开启版本控制，带versionId删除对象，该对象无历史版本
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */

public class DeleteObject16454 extends S3TestBase {
    private String bucketName = "bucket16454";
    private String keyName = "testkey16454";
    private String file = "object16454";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket status is enabled
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, file );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // delete object with version id
        s3Client.deleteVersion( bucketName, keyName, "0" );

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
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }
}
