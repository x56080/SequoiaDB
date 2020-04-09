package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CreateBucketRequest;
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
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-18694:开启版本控制，多次分段上传相同对象
 * @Author wangkexin
 * @Date 2019.07.30
 */
@Test(groups = "partsizelimitoff")
public class UploadPart18694 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18694";
    private String keyName = "key18694";
    private AmazonS3 s3Client = null;
    private long fileSize = 300 * 1024;
    private long partSize = 100 * 1024;
    private File localPath = null;
    private File oldFile = null;
    private File newFile = null;
    private String oldFilePath = null;
    private String newFilePath = null;
    private List< PartETag > partEtags = new CopyOnWriteArrayList<>();
    private int[] partNums1 = new int[] { 1, 3, 5 };
    private int[] partNums2 = new int[] { 2, 3, 5 };

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        oldFilePath = localPath + File.separator + "localFile_" + fileSize
                + "old.txt";
        newFilePath = localPath + File.separator + "localFile_" + fileSize
                + "new.txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( oldFilePath, fileSize );
        TestTools.LocalFile.createFile( newFilePath, fileSize );
        oldFile = new File( oldFilePath );
        newFile = new File( newFilePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void testUpload() throws Exception {
        long filepositon = 0;
        // 指定多个分段上传对象
        String uploadId1 = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        ThreadExecutor es = new ThreadExecutor();
        for ( int i : partNums1 ) {
            es.addWorker( new ThreadUploadPart18694( i, filepositon, oldFile,
                    uploadId1 ) );
            filepositon += partSize;
        }
        es.run();
        List< PartETag > partEtags1 = partEtags;

        // 再次指定多个分段上传对象
        filepositon = 0;
        partEtags = new CopyOnWriteArrayList<>();
        String uploadId2 = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        es = new ThreadExecutor();
        for ( int i : partNums2 ) {
            es.addWorker( new ThreadUploadPart18694( i, filepositon, newFile,
                    uploadId2 ) );
            filepositon += partSize;
        }
        es.run();
        List< PartETag > partEtags2 = partEtags;

        // 完成分段上传
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId1, partEtags1 );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId2, partEtags2 );
        checkResult();
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

    private void checkResult() throws Exception {
        String expMd5 = TestTools.getMD5( newFilePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName, "1" );
        Assert.assertEquals( downloadMd5, expMd5, "version id = 1" );

        expMd5 = TestTools.getMD5( oldFilePath );
        downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName, "0" );
        Assert.assertEquals( downloadMd5, expMd5, "version id = 0" );
    }

    class ThreadUploadPart18694 {
        private AmazonS3 s3Client = CommLib.buildS3Client();
        private int partNumber;
        private long filepositon;
        private File file;
        private String uploadId;

        public ThreadUploadPart18694( int partNumber, long filepositon,
                File file, String uploadId ) {
            this.partNumber = partNumber;
            this.filepositon = filepositon;
            this.file = file;
            this.uploadId = uploadId;
        }

        @ExecuteOrder(step = 1, desc = "分段上传对象")
        public void putObject() {
            try {
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( filepositon )
                        .withPartNumber( partNumber ).withPartSize( partSize )
                        .withBucketName( bucketName ).withKey( keyName )
                        .withUploadId( uploadId );
                UploadPartResult uploadPartResult = s3Client
                        .uploadPart( partRequest );
                partEtags.add( uploadPartResult.getPartETag() );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
