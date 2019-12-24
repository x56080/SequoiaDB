package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-18711: enabling bucket versioning,upload multiple parts,the key specified
 *              different uploadId.
 * @author wuyan
 * @Date 2019.07.30
 * @version 1.00
 */
@Test(groups = "partlistinuseoff") public class UploadPart18711
        extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18711";
    private String keyName = "/aa/object18711";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;
    private int fileSize1 = 1024 * 50;
    private int fileSize2 = 1024 * 30;
    private int partSize1 = 1024 * 5;
    private int partSize2 = 1024 * 6;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath1 =
                localPath + File.separator + "localFile_" + fileSize1 + ".txt";
        filePath2 =
                localPath + File.separator + "localFile_" + fileSize2 + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize1 );
        TestTools.LocalFile.createFile( filePath2, fileSize2 );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void uploadParts() throws Exception {
        File file1 = new File( filePath1 );
        String uploadId1 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        List<PartETag> partEtags1 = PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId1, file1,
                        partSize1 );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId1, partEtags1 );

        File file2 = new File( filePath2 );
        String uploadId2 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        List<PartETag> partEtags2 = PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId2, file2,
                        partSize2 );
        PartUploadUtils
                .listPartsAndCheckPartNumbers( s3Client, bucketName, keyName,
                        partEtags2, uploadId2 );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId2, partEtags2 );

        // down file check the file content
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
        String historyVersionId = "0";

        // check the content of the current version object
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath2 ) );

        // check the content of the first upload object
        String updateMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName,
                        historyVersionId );
        Assert.assertEquals( updateMd5, TestTools.getMD5( filePath1 ) );
    }

}
