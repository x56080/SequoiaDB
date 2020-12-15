package com.sequoias3.object;

import java.io.File;
import java.io.IOException;
import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

/**
 * @Description seqDB-19334:指定ifNoneMatch和ifUnModifiedSince条件匹配源对象复制
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19334 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user19334";
    private AmazonS3 s3Client = null;
    private String srcBucketName = "bucket19334a";
    private String dstBucketName = "bucket19334b";
    private String keyName = "obj19334";
    private int fileSize = 5 * 1024 * 1024;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;
    private String filePath3 = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath1 = localPath + File.separator + "localFile_" + fileSize
                + "_1.txt";
        filePath2 = localPath + File.separator + "localFile_" + fileSize
                + "_2.txt";
        filePath3 = localPath + File.separator + "localFile_" + fileSize
                + "_2.txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize );
        TestTools.LocalFile.createFile( filePath2, fileSize );
        TestTools.LocalFile.createFile( filePath3, fileSize );

        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName,
                UserCommDefind.normal );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( dstBucketName );
        CommLib.setBucketVersioning( s3Client, srcBucketName, "Enabled" );
        CommLib.setBucketVersioning( s3Client, dstBucketName, "Enabled" );

        s3Client.putObject( srcBucketName, keyName, new File( filePath1 ) );
        s3Client.putObject( srcBucketName, keyName, new File( filePath2 ) );
    }

    @Test
    private void test() throws Exception {
        // get last modified date of the current version object
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                srcBucketName, keyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        String srcObjHisVerETag = objMetadata.getETag();
        Date srcObjHisLastModDate = objMetadata.getLastModified();

        // put object after last modified date of current version object
        s3Client.putObject( srcBucketName, keyName, new File( filePath3 ) );

        // copy object
        String srcObjHisVer = "0";
        CopyObjectRequest request = new CopyObjectRequest( srcBucketName,
                keyName, srcObjHisVer, dstBucketName, keyName );
        request.withNonmatchingETagConstraint( srcObjHisVerETag );
        request.withUnmodifiedSinceConstraint( srcObjHisLastModDate );
        s3Client.copyObject( request );

        // check results
        String expVer = "2";
        checkObjectAttribute( filePath3, expVer );
        checkObjectContent( filePath3 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String filePath ) throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                srcBucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttribute( String filePath, String expVersion )
            throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                srcBucketName, keyName );
        ObjectMetadata objMetadata = s3Client.getObjectMetadata( request );
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objMetadata.getETag(), expMd5 );
        Assert.assertEquals( objMetadata.getContentLength(), fileSize );
        Assert.assertEquals( objMetadata.getVersionId(), expVersion,
                "the keyName=" + keyName );
    }
}
