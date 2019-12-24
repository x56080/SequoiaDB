package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * @Description seqDB-19343:指定ifNoneMatch和ifModifiedSince条件获取对象，不匹配ifNoneMatch
 * @author wuyan
 * @Date 2019.09.19
 * @version 1.00
 */
public class CopyObject19343 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19343";
    private String srcKeyName = "/src/bb%/object19343";
    private String destKeyName = "/dest/object19343";
    private String srcKeyContent = "testsrcObject!19343";
    private AmazonS3 s3Client = null;
    private long lastModifiedTime = 0;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, srcKeyName, srcKeyContent );
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                bucketName, srcKeyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        Date lastModifiedDate = objMetadata.getLastModified();
        lastModifiedTime = lastModifiedDate.getTime();
    }

    @Test
    public void testCopyObject() throws Exception {
        // set date 3 minutes early at the lastModified time
        long timestamp = lastModifiedTime - 3 * 60 * 1000l;
        Date date = new Date( timestamp );

        // copy object
        String etag = TestTools.getMD5( srcKeyContent.getBytes() );
        try {
            CopyObjectRequest request = new CopyObjectRequest( bucketName,
                    srcKeyName, bucketName, destKeyName );
            request.withModifiedSinceConstraint( date )
                    .withNonmatchingETagConstraint( etag );
            s3Client.copyObject( request );
            Assert.fail( "copyObject must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 304,
                    e.getErrorCode() + e.getErrorMessage() + "\netag:" + etag );
        }

        // check the result
        Assert.assertFalse( s3Client.doesObjectExist( bucketName, destKeyName ),
                "destObject no exist!" );
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
}
