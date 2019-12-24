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
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18675: upload multiple parts,all parts are of equal
 *              length(default size is 5M)
 * @author wuyan
 * @Date 2019.07.25
 * @version 1.00
 */
public class UploadPart18675 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String keyName = "/aa/maa/bb/object18675";
    private AmazonS3 s3Client = null;
    private File localPath = null;

    @DataProvider(name = "fileSizeProvider")
    public Object[][] generateFileSize() {
        return new Object[][] {
                // the parameter : fileSize
                // test a: all parts has the same length,default 1024 * 1024 * 5
                new Object[] { 1024 * 1024 * 5 * 10 },
                // test b: the last part length is 1,the fileSize is 1024 * 1024
                // * 5 * 10 + 1
                new Object[] { 1024 * 1024 * 50 + 1 },
                // test c: the last part length is 5M - 1,,the fileSize is 1024
                // * 1024 * 5 * 5 - 1
                new Object[] { 1024 * 1024 * 25 - 1 }, };
    }

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "fileSizeProvider")
    public void uploadParts( int fileSize ) throws Exception {
        String filePath = createFile( fileSize );
        File file = new File( filePath );
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, keyName );
        List<PartETag> partEtags = PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        // down file check the file content
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateFileSize().length ) {
                s3Client.deleteObject( S3TestBase.bucketName, keyName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private String createFile( int fileSize ) throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        String filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        return filePath;
    }
}
