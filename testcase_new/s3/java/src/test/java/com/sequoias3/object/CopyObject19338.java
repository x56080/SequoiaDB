package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.Date;

/**
 * @Description seqDB-19338:指定ifMatch和ifUnModifiedSince条件匹配对象复制
 * @author wuyan
 * @Date 2019.09.19
 * @version 1.00
 */
public class CopyObject19338 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19338";
    private String srcKeyName = "/src/bb%/object19338";
    private String destKeyName = "/dest/object19338";
    private AmazonS3 s3Client = null;
    private String curVersionContent = "currentVersionContent!";
    private File localPath = null;
    private long lastModifiedTime = 0;

    @BeforeClass
    private void setUp() {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, srcKeyName, "testcontent1" );
        s3Client.putObject( bucketName, srcKeyName, curVersionContent );
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                bucketName, srcKeyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        Date lastModifiedDate = objMetadata.getLastModified();
        lastModifiedTime = lastModifiedDate.getTime();
    }

    @Test
    public void testCopyObject() throws Exception {
        // set date 2 minutes later than lastModifiedTime time
        long timestamp = lastModifiedTime + 2 * 60 * 1000l;
        Date date = new Date( timestamp );

        // copy object
        String curVersionEtag = TestTools
                .getMD5( curVersionContent.getBytes() );
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, destKeyName );
        request.withUnmodifiedSinceConstraint( date )
                .withMatchingETagConstraint( curVersionEtag );
        s3Client.copyObject( request );

        // check the result
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, destKeyName );
        Assert.assertEquals( downfileMd5, curVersionEtag );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
