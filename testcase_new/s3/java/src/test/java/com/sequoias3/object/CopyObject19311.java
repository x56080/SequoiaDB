package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/**
 * @Description seqDB-19311:不同桶复制对象，获取源对象元数据
 * @author wuyan
 * @Date 2019.09.17
 * @version 1.00
 */
public class CopyObject19311 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "aa%maa/bb%object19311a";
    private String destKeyNameA = "/object19311a";
    private String destKeyNameB = "/object19311b";
    private String srcBucketName = "bucket19311a";
    private String destBucketName = "bucket19311b";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 30;
    private File localPath = null;
    private String filePath = null;
    private Map<String, String> expMeta = new HashMap<>();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, destBucketName );
        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( destBucketName );

        expMeta.put( "tag1", "test1" );
        expMeta.put( "tag2", "copyObject" );
        expMeta.put( "tag3", "copy123" );
        PutObjectRequest request = new PutObjectRequest( srcBucketName,
                srcKeyName, new File( filePath ) );
        ObjectMetadata metaData = new ObjectMetadata();
        metaData.setUserMetadata( expMeta );
        request.withMetadata( metaData );
        s3Client.putObject( request );
    }

    @Test
    public void testCopyObject() throws Exception {
        // test a:setMetadataDirective is null
        copyObjectWithMeta( destKeyNameA, null );
        // test b:setMetadataDirective is COPY
        copyObjectWithMeta( destKeyNameB, "COPY" );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, destBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void copyObjectWithMeta( String destKeyName,
            String metadataDirective ) throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( srcBucketName,
                srcKeyName, destBucketName, destKeyName );
        request.setMetadataDirective( metadataDirective );

        s3Client.copyObject( request );
        checkObjectAttributeInfo( destBucketName, destKeyName, expMeta );
        checkObjectContent( destBucketName, destKeyName );
    }

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        // down file
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttributeInfo( String bucketName, String keyName,
            Map<String, String> expMeta ) throws IOException {
        // check the attributeInfo of get object
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        Assert.assertEquals( result.getETag(), TestTools.getMD5( filePath ) );
        Assert.assertEquals( result.getContentLength(), fileSize );

        Map<String, String> actMeta = result.getUserMetadata();
        Assert.assertEquals( actMeta.size(), expMeta.size(),
                "expMeta is : " + expMeta.toString() + "actMeta is : " + actMeta
                        .toString() );
        for ( Map.Entry<String, String> entry : expMeta.entrySet() ) {
            Object key = entry.getKey();
            Assert.assertEquals( actMeta.get( key ), expMeta.get( key ),
                    "actMeta = " + actMeta.toString() + ",expMeta = " + expMeta
                            .toString() );
        }
    }
}
