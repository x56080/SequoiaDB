package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.CopyObjectResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-19333: 指定ifMatch和ifNoneMatch条件复制对象，指定源对象不匹配ifMatch
 * @author wuyan
 * @Date 2019.09.19
 * @version 1.00
 */
public class CopyObject19333 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19333";
    private String srcKeyName = "/src/bb%/object19333";
    private String destKeyName = "/dest/object19333";
    private AmazonS3 s3Client = null;
    private String hisVersionContent0 = "hisVersion0";
    private String hisVersionContent1 = "hisVersionContent1";

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, srcKeyName, hisVersionContent0 );
        s3Client.putObject( bucketName, srcKeyName, hisVersionContent1 );
        s3Client.putObject( bucketName, srcKeyName, "curVersionContent0002" );
    }

    @Test
    public void testCopyObject() throws Exception {
        String hisVersionETag0 = TestTools
                .getMD5( hisVersionContent0.getBytes() );
        String hisVersionETag1 = TestTools
                .getMD5( hisVersionContent1.getBytes() );
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, destKeyName );
        request.withMatchingETagConstraint( hisVersionETag1 )
                .withNonmatchingETagConstraint( hisVersionETag0 );
        CopyObjectResult result = s3Client.copyObject( request );
        Assert.assertNull( result, "does not match object!" );
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
