package com.sequoias3.partupload;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * test content: ListPartsRequest接口参数校验 testlink-case: seqDB-18809
 *
 * @author wangkexin
 * @Date 2019.8.6
 * @version 1.00
 */
public class ListPartsRequest18809 extends S3TestBase {
    private String bucketName = "bucket18809";
    private String keyName = "key18809";
    private AmazonS3 s3Client = null;
    private long fileSize = 15 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testIllegalParameter() throws Exception {
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );

        // a.接口参数取值合法---已在功能测试中验证
        // b.接口参数取值非法---bucketName\key取值为null
        ListPartsRequest request = new ListPartsRequest( null, keyName,
                uploadId );
        try {
            s3Client.listParts( request );
            Assert.fail( "when bucketName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when listing parts" );
        }

        request = new ListPartsRequest( bucketName, null, uploadId );
        try {
            s3Client.listParts( request );
            Assert.fail( "when keyName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The key parameter must be specified when listing parts" );
        }

        // key取值为空串
        request = new ListPartsRequest( bucketName, "", uploadId );
        try {
            s3Client.listParts( request );
            Assert.fail( "when keyName is '', it should fail." );
        } catch ( AmazonServiceException e ) {
            if ( !e.getErrorCode().equals( "InvalidRequest" ) || !e
                    .getErrorMessage().equals( "A key must be specified." ) ) {
                throw e;
            }
        }

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
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}