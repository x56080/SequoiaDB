package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
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
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18677: upload multiple parts,the length of the first part
 *              is different form the other parts.
 * @author wuyan
 * @Date 2019.07.26
 * @version 1.00
 */
public class UploadPart18677 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String keyName = "/aa/object18677";
    private AmazonS3 s3Client = null;
    private File localPath = null;

    @DataProvider(name = "fileSizeProvider")
    public Object[][] generateFileSize() {
        return new Object[][] {
                // the parameter : fileSize,firstPartSize, otherPartSize
                // test a: the length of the first part is greater than the
                // other parts
                new Object[] { 1024 * 1024 * 26, 1024 * 1024 * 6,
                        1024 * 1024 * 5 },
                // test b: the length of the first part is smaller than the
                // other parts
                new Object[] { 1024 * 1024 * 27, 1024 * 1024 * 6,
                        1024 * 1024 * 7 },
                // test c: the length of the first part is different from the
                // last part
                // the first part length is 1024 * 1024 * 5 + 1024, the last
                // part length is 1024 * 1024 ,other pasts is 1024 * 1024 * 5
                new Object[] { 1024 * 1024 * 5 + 1024 + 1024 * 1024 * 5 * 4
                        + 1024 * 1024, 1024 * 1024 * 5 + 1024,
                        1024 * 1024 * 5 }, };
    }

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "fileSizeProvider")
    public void uploadParts( int fileSize, long firstPartSize,
            long eachPartSize ) throws Exception {
        String filePath = createFile( fileSize );
        File file = new File( filePath );
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, keyName );
        List<PartETag> partEtags = partUpload( s3Client, bucketName, keyName,
                uploadId, file, firstPartSize, eachPartSize );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

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

    private List<PartETag> partUpload( AmazonS3 s3Client, String bucketName,
            String key, String uploadId, File file, Long firstPartSize,
            long partSize ) {
        List<PartETag> partEtags = new ArrayList<>();
        int filePosition = 0;
        long fileSize = file.length();
        for ( int i = 1; filePosition < fileSize; i++ ) {
            long eachPartSize = 0;
            if ( i == 1 ) {
                eachPartSize = firstPartSize;
            } else {
                eachPartSize = Math.min( partSize, fileSize - filePosition );
            }
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( i ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( key )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += eachPartSize;
        }
        return partEtags;
    }
}
