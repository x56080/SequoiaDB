package com.sequoias3.object;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;

/**
 * @Description seqDB-19329:复制对象指定ifNoneMatch条件
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19329 extends S3TestBase {
    private AtomicInteger runSuccessNum = new AtomicInteger( 0 );
    private int expRunSuccessNum = 3;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19329";
    private String srcKeyName = "srcObj19329";
    private String dstKeyName = "dstObj19329";
    private String srcCurVerETag;
    private String srcHisVerETag;
    private int fileSize = 5 * 1024 * 1024;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath1 = localPath + File.separator + "localFile_" + fileSize
                + "_1.txt";
        filePath2 = localPath + File.separator + "localFile_" + fileSize
                + "_2.txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize );
        TestTools.LocalFile.createFile( filePath2, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        PutObjectResult result;
        result = s3Client.putObject( bucketName, srcKeyName,
                new File( filePath1 ) );
        srcHisVerETag = result.getETag();
        result = s3Client.putObject( bucketName, srcKeyName,
                new File( filePath2 ) );
        srcCurVerETag = result.getETag();
    }

    // init keyNameB after copy
    @AfterMethod
    private void afterMethod() {
        String dstObjCurVer = "1";
        s3Client.deleteVersion( bucketName, dstKeyName, dstObjCurVer );
        String dstObjHisVer = "0";
        s3Client.deleteVersion( bucketName, dstKeyName, dstObjHisVer );
    }

    // a.versionId is history version, but appoint eTag is current version eTag
    @Test
    private void testCopyObject_A() throws Exception {
        String srcObjHisVer = "0";
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, srcObjHisVer, bucketName, dstKeyName );
        request.withNonmatchingETagConstraint( srcCurVerETag );
        s3Client.copyObject( request );

        String expDstObjVer = "0";
        checkObjectAttribute( dstKeyName, expDstObjVer, srcHisVerETag );
        checkObjectContent( dstKeyName, filePath1 );
        runSuccessNum.getAndIncrement();
    }

    // b.appoint eTag is history version eTag
    @Test
    private void testCopyObject_B() throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, dstKeyName );
        request.withNonmatchingETagConstraint( srcHisVerETag );
        s3Client.copyObject( request );

        String expDstObjVer = "0";
        checkObjectAttribute( dstKeyName, expDstObjVer, srcCurVerETag );
        checkObjectContent( dstKeyName, filePath2 );
        runSuccessNum.getAndIncrement();
    }

    // c.appoint eTag is current version eTag
    @Test
    private void testCopyObject_C() throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, dstKeyName );
        request.withNonmatchingETagConstraint( srcCurVerETag );
        try {
            s3Client.copyObject( request );
            Assert.fail( "expect fail, but actual success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 304 );
        }
        Assert.assertFalse(
                s3Client.doesObjectExist( bucketName, dstKeyName ) );

        runSuccessNum.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccessNum.get() == expRunSuccessNum ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String keyName, String filePath )
            throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttribute( String keyName, String expVersion,
            String expETag ) throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata objMetadata = s3Client.getObjectMetadata( request );
        Assert.assertEquals( objMetadata.getETag(), expETag );
        Assert.assertEquals( objMetadata.getContentLength(), fileSize );
        Assert.assertEquals( objMetadata.getVersionId(), expVersion,
                "the keyName=" + keyName );
    }
}
