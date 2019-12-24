package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
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
 * @Description seqDB-19437:关闭分段检测开关，相同对象并发上传一个分段和完成分段上传
 * @author wangkexin
 * @Date 2019.09.16
 * @version 1.00
 */
@Test(groups = "partlistinuseoff") public class UploadPartAndCompleteMultipartUpload19437
        extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String keyName = "/aa/object19437";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 100;
    private int partSize = 1024 * 1024 * 20;
    private String uploadId = null;
    private List<PartETag> partEtags = new ArrayList<PartETag>();

    @DataProvider(name = "thirdPartSize")
    private Object[][] generateFirstPartSize() {
        // parameter : thirdPartSize
        return new Object[][] { new Object[] { 20 * 1024 * 1024 },
                new Object[] { 30 * 1024 * 1024 }, new Object[] { 0 } };
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
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "thirdPartSize")
    public void uploadParts( long thirdPartSize ) throws Exception {
        File file = new File( filePath );
        uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, keyName );
        // upload part 1 and part 2
        partEtags = partUpload( file, 2 );
        PartETag partETag = new PartETag( 3, "" );
        partEtags.add( partETag );

        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new PartUpload( thirdPartSize, file ) );

        threadExec.addWorker( new CompletePartUpload( uploadId ) );
        threadExec.run();

        // get the upload object to check content by md5
        String expMd5WithPart3 = TestTools
                .getLargeFilePartMD5( file, 0, 2 * partSize + thirdPartSize );
        String expMd5WithoutPart3 = TestTools
                .getLargeFilePartMD5( file, 0, 2 * partSize );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        if ( !downfileMd5.equals( expMd5WithoutPart3 ) && !downfileMd5
                .equals( expMd5WithPart3 ) ) {
            Assert.fail( "actMd5 is :" + downfileMd5 + ", expMd5 is "
                    + expMd5WithoutPart3 + " or " + expMd5WithPart3 );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateFirstPartSize().length ) {
                s3Client.deleteObject( S3TestBase.bucketName, keyName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    public List<PartETag> partUpload( File file, int partNumber ) {
        List<PartETag> partEtags = new ArrayList<PartETag>();
        int filePosition = 0;
        long fileSize = file.length();
        for ( int i = 1; filePosition <= partSize; i++ ) {
            long eachPartSize = Math.min( partSize, fileSize - filePosition );
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( i ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult result = s3Client.uploadPart( partRequest );
            partEtags.add( result.getPartETag() );
            filePosition += partSize;
        }
        return partEtags;
    }

    private class PartUpload {
        private long partSize;
        private File file;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        private PartUpload( long partSize, File file ) {
            this.partSize = partSize;
            this.file = file;
        }

        @ExecuteOrder(step = 1)
        private void partUpload() {
            try {
                int filePosition = ( int ) ( 2 * partSize );
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( filePosition )
                        .withPartNumber( 3 ).withPartSize( partSize )
                        .withBucketName( S3TestBase.bucketName )
                        .withKey( keyName ).withUploadId( uploadId );
                s3Client1.uploadPart( partRequest );
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "NoSuchUpload" ) ) {
                    throw e;
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private class CompletePartUpload {
        private String uploadId;
        private AmazonS3 s3Client2 = CommLib.buildS3Client();

        private CompletePartUpload( String uploadId ) {
            this.uploadId = uploadId;
        }

        @ExecuteOrder(step = 1)
        private void completeMultipartUpload() throws InterruptedException {
            try {
                PartUploadUtils.completeMultipartUpload( s3Client,
                        S3TestBase.bucketName, keyName, uploadId, partEtags );
            } finally {
                if ( s3Client2 != null ) {
                    s3Client2.shutdown();
                }
            }
        }
    }
}
