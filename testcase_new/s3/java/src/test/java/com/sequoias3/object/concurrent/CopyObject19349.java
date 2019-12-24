package com.sequoias3.object.concurrent;

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
 * @Description seqDB-19349:携带相同元数据并发复制对象，指定源和目标对象相同
 * @author wuyan
 * @Date 2019.09.20
 * @version 1.00
 */
public class CopyObject19349 extends S3TestBase {

    private boolean runSuccess = false;
    private String keyName = "/bb/aa%tobject19349";
    private String bucketName = "bucket19349";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 2;
    private File localPath = null;
    private String filePath = null;
    private Map<String, String> userMeta = new HashMap<>();
    private ObjectMetadata metaData = new ObjectMetadata();
    private String contentDisposition = "this is copy object!";

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
        CommLib.clearBucket( s3Client, bucketName );

        s3Client.createBucket( bucketName );
        Map<String, String> srcMeta = new HashMap<>();
        srcMeta.put( "oldtag", "test125" );
        ObjectMetadata srcMetaData = new ObjectMetadata();
        srcMetaData.setUserMetadata( srcMeta );
        PutObjectRequest request = new PutObjectRequest( bucketName, keyName,
                new File( filePath ) );
        request.withMetadata( srcMetaData );
        s3Client.putObject( request );

        // meta set the userMeta and contentDisposition
        userMeta.put( "tag1", "testa" );
        userMeta.put( "tag2", "testa2" );
        metaData = new ObjectMetadata();
        metaData.setUserMetadata( userMeta );
        metaData.setContentDisposition( contentDisposition );
    }

    @Test(invocationCount = 3, threadPoolSize = 3)
    private void testCopyObject() {
        AmazonS3 s3Client1 = CommLib.buildS3Client();
        try {
            CopyObjectRequest request = new CopyObjectRequest( bucketName,
                    keyName, bucketName, keyName );
            request.setMetadataDirective( "REPLACE" );
            request.withNewObjectMetadata( metaData );
            s3Client1.copyObject( request );
        } finally {
            if ( s3Client1 != null ) {
                s3Client1.shutdown();
            }
        }
    }

    @Test(dependsOnMethods = "testCopyObject")
    private void checkResult() throws Exception {
        checkObjectMetaData( userMeta, contentDisposition );
        checkObjectContent( bucketName, keyName );
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

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectMetaData( Map<String, String> expMeta,
            String contentDisposition ) {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );

        Map<String, String> actMeta = result.getUserMetadata();
        Assert.assertEquals( actMeta.size(), expMeta.size(),
                "expMetaB is : " + expMeta.toString() + "actMetaB is : "
                        + actMeta.toString() );
        for ( Map.Entry<String, String> entry : expMeta.entrySet() ) {
            Object key = entry.getKey();
            Assert.assertEquals( actMeta.get( key ), expMeta.get( key ),
                    "actMetaB = " + actMeta.toString() + ",expMeta = " + expMeta
                            .toString() );
        }
        Assert.assertEquals( result.getContentDisposition(),
                contentDisposition );

    }
}
