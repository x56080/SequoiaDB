package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description: 开启检测开关，上传多个分段不存在partNum为1、连续分段号且分段长度不一致
 * testlink-case:seqDB-18700
 *
 * @author wangkexin
 * @Date 2019.7.30
 * @version 1.00
 */
public class UploadPart18700 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18700";
    private String keyName = "key18700";
    private AmazonS3 s3Client = null;
    private long m = 1 * 1024 * 1024;
    private long fileSize = 35 * m;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId = "";
    private List< PartETag > partEtags = new CopyOnWriteArrayList<>();
    private long[] partSize = new long[] { 6 * m, 10 * m, 5 * m, 6 * m, 8 * m };

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void testUpload() throws Exception {
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        ThreadExecutor es = new ThreadExecutor();
        int index = 0;
        long filepositon = 0;
        for ( int i = 2; i < 7; i++ ) {
            es.addWorker( new ThreadUploadPart18700( filepositon,
                    partSize[ index ], i ) );
            filepositon += partSize[ index ];
            index++;
        }
        es.run();

        // 完成分段上传
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
        String expMd5 = TestTools.getMD5( filePath );
        String actMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( actMd5, expMd5 );
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

    class ThreadUploadPart18700 {
        private AmazonS3 s3Client = CommLib.buildS3Client();
        private long filepositon;
        private long partSize;
        private int partNumber;

        public ThreadUploadPart18700( long filepositon, long partSize,
                int partNumber ) {
            this.filepositon = filepositon;
            this.partSize = partSize;
            this.partNumber = partNumber;
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
