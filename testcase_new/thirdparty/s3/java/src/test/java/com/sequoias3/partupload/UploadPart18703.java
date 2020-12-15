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
 * @Description seqDB-18703: upload multiple parts,the specified partNums is
 *              inconsistent with the partNums actually uploaded.
 * @author wuyan
 * @Date 2019.07.30
 * @version 1.00
 */
@Test(groups = "partlistinuseoff")
public class UploadPart18703 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyNameA = "/aa/maa/bb/object18703A";
    private String keyNameB = "/aa/maa/bb/object18703B";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 50;
    private int partSize = 1024 * 5;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void uploadParts() throws Exception {
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
                s3Client.deleteObject( S3TestBase.bucketName, keyNameA );
                s3Client.deleteObject( S3TestBase.bucketName, keyNameB );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void uploadPartsA( File file, String keyName ) throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId, file, partSize );

        // remove the first partNumber
        partEtags.remove( 0 );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        // down file check the file content
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void uploadPartsB( File file, String keyName ) throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId, file, partSize );

        // add a part,the partNumber is 11,the part Etag is the Etag of
        // partNumber 1
        int addPartNumber = 11;
        String addEtag = partEtags.get( 0 ).getETag();
        PartETag addPartEtag = new PartETag( addPartNumber, addEtag );
        partEtags.add( addPartEtag );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        // down file check the file content
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}
