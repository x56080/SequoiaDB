package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.List;

/**
 * @Description seqDB-18179: the object name include delimiter,delete
 *              currentVersion object with versionId
 * @author wuyan
 * @Date 2019.4.11
 * @version 1.00
 */
public class DeleteObject18179 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "aa/bb/3/object18179";
    private String delimiter = "3";
    private String bucketName = "bucket18179";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void test() throws Exception {
        s3Client.putObject( bucketName, keyName, "test18179a" );
        s3Client.putObject( bucketName, keyName, "test18179withVersionId:1" );

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        String currentVersion = "1";
        String historyVersion = "0";
        s3Client.deleteVersion( bucketName, keyName, currentVersion );
        checkDeleteObjectReslut( historyVersion );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDeleteObjectReslut( String versionId ) throws Exception {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List< S3VersionSummary > versionSummary = versionList
                .getVersionSummaries();

        int existObjectNum = 1;
        Assert.assertEquals( versionSummary.size(), existObjectNum );
        Assert.assertEquals( versionSummary.get( 0 ).getKey(), keyName );
        Assert.assertEquals( versionSummary.get( 0 ).getVersionId(),
                versionId );

    }
}
