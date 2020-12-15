package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.PartETag;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * @Description seqDB-18702: setting detection,upload multiple parts,the
 *              specified partNums is inconsistent with the partNums actually
 *              uploaded.
 * @author wuyan
 * @Date 2019.07.30
 * @version 1.00
 */
public class UploadPart18702 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18702";
    private String keyNameA = "/aa/maa/bb/object18702A";
    private String keyNameB = "/aa/maa/bb/object18702B";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private String tmpFile = null;
    private int fileSize = 1024 * 1024 * 20;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        tmpFile = localPath + File.separator + "localFile_seekRead.txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void uploadPartsB() throws Exception {
        File file = new File( filePath );
        // test a: the specified partNums is less than the partNums actually
        // uploaded.
        uploadPartsA( file, keyNameA );
        // test a: the specified partNums is greater than the partNums actually
        // uploaded.
        uploadPartsB( file, keyNameB );
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

    private void uploadPartsA( File file, String keyName ) throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId, file );

        // remove the first partNumber
        partEtags.remove( 0 );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        // down file check the file content
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        int seekSize = 1024 * 1024 * 5;
        TestTools.seekReadFile( filePath, seekSize, tmpFile );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( tmpFile ) );
    }

    private void uploadPartsB( File file, String keyName ) throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId, file );

        // add a part,the partNumber is 6,the part Etag is the Etag of
        // partNumber 1
        int addPartNumber = 6;
        String addEtag = partEtags.get( 0 ).getETag();
        PartETag addPartEtag = new PartETag( addPartNumber, addEtag );
        partEtags.add( addPartEtag );

        try {
            PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                    keyName, uploadId, partEtags );
            Assert.fail( "completeMultipartUpload should be fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidPart",
                    "get status code:" + e.getStatusCode() );
        }

        // get key is not exist.
        try {
            s3Client.getObject( bucketName, keyName );
            Assert.fail( "get not exist key must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
    }
}
