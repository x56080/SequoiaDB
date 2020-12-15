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
 * @Description seqDB-18678: upload multiple parts,the length of the middle
 *              parts is inconsistent.
 * @author wuyan
 * @Date 2019.07.26
 * @version 1.00
 */
public class UploadPart18678 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String keyName = "/aa/object18678";
    private AmazonS3 s3Client = null;
    private File localPath = null;

    @DataProvider(name = "fileSizeProvider")
    public Object[][] generateFileSize() {
        return new Object[][] {
                // the parameter :
                // fileSize,eachPartSize,secondPartSize,thirdPartSize,fourthPartSize,fifthPartSize
                // test a: the length of the middle parts is inconsistent
                new Object[] { 1024 * 1024 * 45, 1024 * 1024 * 8,
                        1024 * 1024 * 5, 1024 * 1024 * 9, 1024 * 1024 * 9,
                        1024 * 1024 * 6 },
                // test b: the length of the middle parts grows from small to
                // large
                new Object[] { 1024 * 1024 * 40, 1024 * 1024 * 7,
                        1024 * 1024 * 5, 1024 * 1024 * 6, 1024 * 1024 * 7,
                        1024 * 1024 * 8 },
                // test c: the length of the middle parts grows from large to
                // small
                new Object[] { 1024 * 1024 * 40, 1024 * 1024 * 5,
                        1024 * 1024 * 9, 1024 * 1024 * 8, 1024 * 1024 * 7,
                        1024 * 1024 * 6 }, };
    }

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "fileSizeProvider")
    public void uploadParts( int fileSize, long eachPartSize,
            long secondPartSize, long thirdPartSize, long fourthPartSize,
            long fifthPartSize ) throws Exception {
        String filePath = createFile( fileSize );
        File file = new File( filePath );
        String uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        List< PartETag > partEtags = partUpload( uploadId, file, eachPartSize,
                secondPartSize, thirdPartSize, fourthPartSize, fifthPartSize );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
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
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        String filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        return filePath;
    }

    private List< PartETag > partUpload( String uploadId, File file,
            long partSize, long secondPartSize, long thirdPartSize,
            long fourthPartSize, long fifthPartSize ) {
        List< PartETag > partEtags = new ArrayList<>();
        int filePosition = 0;
        long fileSize = file.length();
        for ( int i = 1; filePosition < fileSize; i++ ) {
            long eachPartSize = 0;
            if ( i == 2 ) {
                eachPartSize = secondPartSize;
            } else if ( i == 3 ) {
                eachPartSize = thirdPartSize;
            } else if ( i == 4 ) {
                eachPartSize = fourthPartSize;
            } else if ( i == 5 ) {
                eachPartSize = fifthPartSize;
            } else {
                eachPartSize = partSize;
            }
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( i ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += eachPartSize;
        }
        return partEtags;
    }
}
