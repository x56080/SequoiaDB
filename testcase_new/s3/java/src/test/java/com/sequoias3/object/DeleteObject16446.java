package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.Iterator;

/**
 * @Description seqDB-16446: enabling bucket versioning , delete object of
 *              delete tag
 * @author wuyan
 * @Date 2018.11.21
 * @version 1.00
 */
public class DeleteObject16446 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16446";
    private String key = "&aa&%maa&bb*中文&objectOfdeleteTag16446";
    private AmazonS3 s3Client = null;

    @SuppressWarnings("deprecation")
    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        if ( s3Client.doesBucketExist( bucketName ) ) {
            ObjectUtils.deleteObjectAllVersions( s3Client, bucketName, key );
            s3Client.deleteBucket( bucketName );
        }
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        // generate object of delete tag,the vesionID is "0" and "1"
        s3Client.deleteObject( bucketName, key );
        s3Client.deleteObject( bucketName, key );
    }

    @Test
    public void testDeleteObject() throws Exception {
        s3Client.deleteObject( bucketName, key );
        checkDeleteObjectResult( bucketName, key );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client, bucketName,
                        key );
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDeleteObjectResult( String bucketName, String key )
            throws Exception {
        // current version object not exist
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertFalse( isExistObject, "the object should not exist!" );

        // delete the object of delete tag , add a delete tag
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        Iterator< S3VersionSummary > versionIter = versionList
                .getVersionSummaries().iterator();
        int count = 0;
        while ( versionIter.hasNext() ) {
            S3VersionSummary vs = versionIter.next();
            String getKey = vs.getKey();
            boolean isDeleteMarker = vs.isDeleteMarker();
            Assert.assertEquals( getKey, key );
            Assert.assertTrue( isDeleteMarker );
            count++;
        }
        int deleteTagNums = 3;
        Assert.assertEquals( count, deleteTagNums );
    }
}
