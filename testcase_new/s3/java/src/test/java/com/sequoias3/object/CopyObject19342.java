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
import java.io.IOException;
import java.util.Date;

/**
 * @Description seqDB-19342:指定ifNoneMatch和ifModifiedSince条件复制对象，
 *              源对象不匹配ifModifiedSince
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19342 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String srcBucketName = "bucket19342a";
    private String dstBucketName = "bucket19342b";
    private String keyName = "obj19342";
    private int fileSize = 5 * 1024 * 1024;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath1 =
                localPath + File.separator + "localFile_" + fileSize + "_1.txt";
        filePath2 =
                localPath + File.separator + "localFile_" + fileSize + "_2.txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize );
        TestTools.LocalFile.createFile( filePath2, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, dstBucketName );
        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( dstBucketName );
        CommLib.setBucketVersioning( s3Client, srcBucketName, "Enabled" );
        CommLib.setBucketVersioning( s3Client, dstBucketName, "Enabled" );

        s3Client.putObject( srcBucketName, keyName, new File( filePath1 ) );
    }

    @Test
    private void test() throws Exception {
        ObjectMetadata objMetadata;
        // get last modified date of the current version object
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                srcBucketName, keyName );
        objMetadata = s3Client.getObjectMetadata( metadataRequest );
        String srcObjHisVerETag = objMetadata.getETag();

        // put object after last modified date of current version object
        s3Client.putObject( srcBucketName, keyName, new File( filePath2 ) );
        objMetadata = s3Client.getObjectMetadata( metadataRequest );
        Date srcObjCurLastModDate = objMetadata.getLastModified();

        // copy object
        CopyObjectRequest request = new CopyObjectRequest( srcBucketName,
                keyName, dstBucketName, keyName );
        request.withNonmatchingETagConstraint( srcObjHisVerETag );
        request.withModifiedSinceConstraint( srcObjCurLastModDate );
        s3Client.copyObject( request );

        // check results
        checkObjectAttribute( filePath2 );
        checkObjectContent( filePath2 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, dstBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String filePath ) throws Exception {
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, dstBucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttribute( String filePath ) throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                dstBucketName, keyName );
        ObjectMetadata objMetadata = s3Client.getObjectMetadata( request );
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objMetadata.getETag(), expMd5 );
        Assert.assertEquals( objMetadata.getContentLength(), fileSize );
        Assert.assertEquals( objMetadata.getVersionId(), "0",
                "the keyName=" + keyName );
    }
}
