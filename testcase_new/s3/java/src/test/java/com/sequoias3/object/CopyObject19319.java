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
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19319:桶内指定versionId复制对象，源和目标对象名不同
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19319 extends S3TestBase {
    private int runSuccessNum = 0;
    private int expRunSuccessNum = 2;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19319";
    private String srcKeyName = "srcObj19319";
    private String dstKeyName = "dstObj19319";
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

        s3Client.putObject( bucketName, srcKeyName, new File( filePath1 ) );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath2 ) );
    }

    // init keyNameB after copy
    @AfterMethod
    private void afterMethod() {
        String dstObjCurVer = "1";
        s3Client.deleteVersion( bucketName, dstKeyName, dstObjCurVer );
        String dstObjHisVer = "0";
        s3Client.deleteVersion( bucketName, dstKeyName, dstObjHisVer );
    }

    @Test
    private void testCopyObject_curVer() throws Exception {
        String srcObjCurVer = "1";
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, srcObjCurVer, bucketName, dstKeyName );
        s3Client.copyObject( request );

        String expDstObjVer = "0";
        checkObjectAttribute( dstKeyName, filePath2, expDstObjVer );
        checkObjectContent( dstKeyName, filePath2 );
        runSuccessNum++;
    }

    @Test
    private void testCopyObject_hisVer() throws Exception {
        String srcObjDstVer = "0";
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, srcObjDstVer, bucketName, dstKeyName );
        s3Client.copyObject( request );

        String expDstObjVer = "0";
        checkObjectAttribute( dstKeyName, filePath1, expDstObjVer );
        checkObjectContent( dstKeyName, filePath1 );
        runSuccessNum++;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccessNum == expRunSuccessNum ) {
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

    private void checkObjectAttribute( String keyName, String filePath,
            String expVersion ) throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata objMetadata = s3Client.getObjectMetadata( request );
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objMetadata.getETag(), expMd5 );
        Assert.assertEquals( objMetadata.getContentLength(), fileSize );
        Assert.assertEquals( objMetadata.getVersionId(), expVersion,
                "the keyName=" + keyName );
    }
}
