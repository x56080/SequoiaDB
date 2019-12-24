package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
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
 * @Description seqDB-18682:上传相同分段，其中分段长度不同
 * @Author wangkexin
 * @Date 2019.07.25
 */
@Test(groups = "partsizelimitoff") public class UploadPart18682
        extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket18682";
    private AmazonS3 s3Client = null;
    private long fileSize = 10 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId = "";
    private List<PartETag> partEtags = null;

    @DataProvider(name = "uploadProvider")
    public Object[][] generateObjectNumber() {
        // parameter : keyName, oldPartSize, newPartSize
        return new Object[][] {
                // test a : 再次上传分段长度大于原分段长度
                new Object[] { "/dir1/dir2/obj18682a.tar", 4 * 1024 * 1024,
                        5 * 1024 * 1024 },
                // test b : 再次上传分段长度小于原分段长度
                new Object[] { "/dir1/dir2/obj18682b.tar", 5 * 1024 * 1024,
                        2 * 1024 * 1024 } };
    }

    @BeforeClass
    private void setUp() throws IOException {
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

    @Test(dataProvider = "uploadProvider")
    private void testUpload( String keyName, long oldPartSize,
            long newPartSize ) throws Exception {
        partEtags = new ArrayList<>();
        uploadPartFirst( keyName, oldPartSize );
        long partTwoOffset = oldPartSize;

        uploadPartTwoAgain( keyName, partTwoOffset, newPartSize );
        long currentFileSize = partTwoOffset + newPartSize;
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
        checkResult( keyName, currentFileSize );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateObjectNumber().length ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void uploadPartFirst( String keyName, long partSize )
            throws IOException {
        uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );

        int filepositon = 0;
        for ( int i = 1; i < 3; i++ ) {
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filepositon )
                    .withPartNumber( i ).withPartSize( partSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            String expPartMd5 = TestTools
                    .getFilePartMD5( file, filepositon, partSize );
            String actPartMd5 = uploadPartResult.getPartETag().getETag();
            Assert.assertEquals( actPartMd5, expPartMd5,
                    "part number = " + uploadPartResult.getPartETag()
                            .getPartNumber() );
            filepositon += partSize;
        }
    }

    private void uploadPartTwoAgain( String keyName, long fileOffset,
            long partSize ) throws IOException {
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( fileOffset ).withPartNumber( 2 )
                .withPartSize( partSize ).withBucketName( bucketName )
                .withKey( keyName ).withUploadId( uploadId );
        UploadPartResult uploadPartResult = s3Client.uploadPart( partRequest );
        partEtags.set( 1, uploadPartResult.getPartETag() );
        String expPartMd5 = TestTools
                .getFilePartMD5( file, fileOffset, partSize );
        String actPartMd5 = uploadPartResult.getPartETag().getETag();
        Assert.assertEquals( actPartMd5, expPartMd5,
                "part number = " + uploadPartResult.getPartETag()
                        .getPartNumber() );
    }

    private void checkResult( String keyName, long fileSize ) throws Exception {
        String expMd5 = TestTools.getFilePartMD5( file, 0, fileSize );
        String downloadMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
    }
}
