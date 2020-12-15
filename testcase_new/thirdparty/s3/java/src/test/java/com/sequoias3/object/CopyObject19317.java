package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/**
 * @Description seqDB-19317:桶内复制对象，携带x-amz-metadata-directive
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19317 extends S3TestBase {
    private int runSuccessNum = 0;
    private int expRunSuccessNum = 4;
    private AmazonS3 s3Client = null;
    private String bucketName;
    private String keyNameA = "srcObj19317A";
    private String keyNameB = "srcObj19317B";
    private Map< String, String > xMeta = new HashMap<>();
    private ObjectMetadata metadata = new ObjectMetadata();
    private int fileSize = 6 * 1024 * 1024;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        bucketName = S3TestBase.bucketName;
        s3Client = CommLib.buildS3Client();

        // put object A, and set user-defined metadata
        PutObjectResult objResultA = s3Client.putObject( bucketName, keyNameA,
                new File( filePath ) );
        xMeta.put( "def", "test" );
        metadata.setUserMetadata( xMeta );
        objResultA.setMetadata( metadata );
        PutObjectRequest request = new PutObjectRequest( bucketName, keyNameA,
                new File( filePath ) );
        request.withMetadata( metadata );
        s3Client.putObject( request );

        // put object B
        s3Client.putObject( bucketName, keyNameB, new File( filePath ) );
    }

    // init keyNameB after copy
    @AfterMethod
    private void afterMethod() {
        s3Client.deleteObject( bucketName, keyNameB );
        s3Client.putObject( bucketName, keyNameB, new File( filePath ) );
    }

    // a.setMetadataDirective is "COPY", object A copy to object B
    @Test
    private void testCopyObject_A() throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( bucketName, keyNameA,
                bucketName, keyNameB );
        request.setMetadataDirective( "COPY" );
        s3Client.copyObject( request );
        checkObjectAttribute( keyNameB, xMeta );
        checkObjectContent( keyNameB );
        runSuccessNum++;
    }

    // b.setMetadataDirective is "REPLACE", object A copy to object B
    @Test
    private void testCopyObject_B() throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( bucketName, keyNameA,
                bucketName, keyNameB );
        request.setMetadataDirective( "REPLACE" );
        s3Client.copyObject( request );
        checkObjectAttribute( keyNameB, new HashMap< String, String >() );
        checkObjectContent( keyNameB );
        runSuccessNum++;
    }

    // c.setMetadataDirective is "COPY", srcObj and dstObj are objB
    @Test
    private void testCopyObject_C() throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( bucketName, keyNameB,
                bucketName, keyNameB );
        request.setMetadataDirective( "COPY" );
        try {
            s3Client.copyObject( request );
            Assert.fail( "expect fail, but actual success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidRequest" );
        }
        checkObjectAttribute( keyNameB, new HashMap< String, String >() );
        checkObjectContent( keyNameB );
        runSuccessNum++;
    }

    // d.setMetadataDirective is "REPLACE", srcObj and dstObj are objB, and not
    // set metadata
    @Test
    private void testCopyObject_D() throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( bucketName, keyNameB,
                bucketName, keyNameB );
        request.setMetadataDirective( "REPLACE" );
        s3Client.copyObject( request );
        checkObjectAttribute( keyNameB, new HashMap< String, String >() );
        checkObjectContent( keyNameB );
        runSuccessNum++;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccessNum == expRunSuccessNum ) {
                s3Client.deleteObject( bucketName, keyNameA );
                s3Client.deleteObject( bucketName, keyNameB );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String keyName ) throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttribute( String keyName,
            Map< String, String > expMeta ) throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        Assert.assertEquals( result.getETag(), TestTools.getMD5( filePath ) );
        Assert.assertEquals( result.getContentLength(), fileSize );

        Map< String, String > actMeta = result.getUserMetadata();
        Assert.assertEquals( actMeta.size(), expMeta.size(), "expMeta is : "
                + expMeta.toString() + "actMeta is : " + actMeta.toString() );
        for ( Map.Entry< String, String > entry : expMeta.entrySet() ) {
            Object key = entry.getKey();
            Assert.assertEquals( actMeta.get( key ), expMeta.get( key ),
                    "actMeta = " + actMeta.toString() + ",expMeta = "
                            + expMeta.toString() );
        }
    }
}
