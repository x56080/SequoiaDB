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
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

/**
 * @Description seqDB-19312:不同桶复制对象，自定义目标对象元数据信息
 * @author wuyan
 * @Date 2019.09.18
 * @version 1.00
 */
public class CopyObject19312 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "/bb%/object19312a.txt";
    private String destKeyName = "/object19312a.png";
    private String srcBucketName = "bucket19312a";
    private String destBucketName = "bucket19312b";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 12;
    private File localPath = null;
    private String filePath = null;
    private Date httpExpiresDate = null;

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

        Map<String, String> srcObjectMeta = new HashMap<>();
        srcObjectMeta.put( "tag1", "testa" );
        srcObjectMeta.put( "tag2", "testa2" );
        PutObjectRequest request = new PutObjectRequest( srcBucketName,
                srcKeyName, new File( filePath ) );
        ObjectMetadata metaData = new ObjectMetadata();
        metaData.setContentDisposition( "this is src object!" );
        metaData.setContentType( "txt" );
        metaData.setCacheControl( "null" );
        metaData.setContentEncoding( "tar" );
        metaData.setContentLanguage( "zh" );
        metaData.setHttpExpiresDate( new Date() );
        metaData.setUserMetadata( srcObjectMeta );
        request.withMetadata( metaData );
        s3Client.putObject( request );

        // set the httpExpiresData
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                srcBucketName, srcKeyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        Date lastModifiedDate = objMetadata.getLastModified();
        long lastModifiedTime = lastModifiedDate.getTime();
        // set date 1 hour later than lastModified time
        long timestamp = lastModifiedTime + 60 * 60 * 1000l;
        httpExpiresDate = new Date( timestamp );
    }

    @Test
    public void testCopyObject() throws Exception {
        // put user-defined metadata
        Map<String, String> destObjectMeta = setUserDefinedMetaData();
        ObjectMetadata metaData = new ObjectMetadata();
        metaData.setUserMetadata( destObjectMeta );
        metaData.setContentDisposition( "this is copy object!" );
        metaData.setContentType( "png" );
        metaData.setCacheControl( "RFC2616" );
        metaData.setContentEncoding( "gzip" );
        metaData.setContentLanguage( "en" );
        metaData.setHttpExpiresDate( httpExpiresDate );

        // copy object
        CopyObjectRequest request = new CopyObjectRequest( srcBucketName,
                srcKeyName, destBucketName, destKeyName );
        request.setMetadataDirective( "REPLACE" );
        request.withNewObjectMetadata( metaData );
        s3Client.copyObject( request );

        // check result
        checkObjectAttributeInfo( destBucketName, destKeyName, destObjectMeta );
        checkObjectContent( destBucketName, destKeyName );

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

    private Map<String, String> setUserDefinedMetaData() {
        Map<String, String> destObjectMeta = new HashMap<>();
        destObjectMeta.put( "tag1", "testa" );
        destObjectMeta.put( "tag2", "testb2" );
        destObjectMeta.put( "tag3", "testb-03." );
        return destObjectMeta;
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
        Assert.assertEquals( result.getContentDisposition(),
                "this is copy object!" );
        Assert.assertEquals( result.getCacheControl(), "RFC2616" );
        Assert.assertEquals( result.getContentEncoding(), "gzip" );
        Assert.assertEquals( result.getContentLanguage(), "en" );
        Assert.assertEquals( result.getContentType(), "png" );
        Assert.assertEquals( result.getHttpExpiresDate(), httpExpiresDate );

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
