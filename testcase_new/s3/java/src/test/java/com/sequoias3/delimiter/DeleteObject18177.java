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
 * @Description seqDB-18177: the object name include delimiter,detele object
 *              with versionId,the dir of object is used.
 * @author wuyan
 * @Date 2019.4.11
 * @version 1.00
 */
public class DeleteObject18177 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyNameA = "aa/bb%/test1/object18177";
    private String keyNameB = "aa/bb%object2_18177";
    private String delimiter = "%";
    private String bucketName = "bucket18177";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        s3Client.putObject( bucketName, keyNameA, keyNameA + "_testa" );
        s3Client.putObject( bucketName, keyNameB, keyNameB + "_testb" );
    }

    @Test
    public void test() throws Exception {
        String currentVersionId = "0";
        s3Client.deleteVersion( bucketName, keyNameA, currentVersionId );

        checkDeleteObjectReslut( keyNameB, currentVersionId );
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

    private void checkDeleteObjectReslut( String keyName, String versionId )
            throws Exception {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List< S3VersionSummary > versionSummary = versionList
                .getVersionSummaries();

        int existObjectNum = 1;
        Assert.assertEquals( versionSummary.size(), existObjectNum,
                "the listVersions :" + versionSummary.toString() );
        Assert.assertEquals( versionSummary.get( 0 ).getKey(), keyName );
        Assert.assertEquals( versionSummary.get( 0 ).isDeleteMarker(), false );
        Assert.assertEquals( versionSummary.get( 0 ).getVersionId(),
                versionId );

    }
}
