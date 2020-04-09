package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PartETag;
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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-18761:相同key并发初始化上传对象
 * @Author wangkexin
 * @Date 2019.08.07
 */

public class UploadPart18761 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18761";
    private String keyName = "key18761";
    private AmazonS3 s3Client = null;
    private long fileSize = 24 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private File file2 = null;
    private String filePath = null;
    private String filePath2 = null;
    private List< AmazonS3 > clientList = Collections
            .synchronizedList( new ArrayList< AmazonS3 >() );

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        filePath2 = localPath + File.separator + "localFile2_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( filePath2, fileSize );
        file = new File( filePath );
        file2 = new File( filePath2 );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    private void testUpload() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new ThreadUploadPart18761( 6 * 1024 * 1024, file ) );
        es.addWorker( new ThreadUploadPart18761( 6 * 1024 * 1024, file2 ) );
        es.addWorker( new ThreadUploadPart18761( 5 * 1024 * 1024, file ) );
        es.run();
        // 未开启版本控制，对象内容为最后一次完成分段上传的对象内容，因只上传了两种内容file和file2，故实际对象内容应为2者中的一种
        String expMd5 = TestTools.getMD5( filePath );
        String expMd5_2 = TestTools.getMD5( filePath2 );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        if ( !downloadMd5.equals( expMd5 )
                && !downloadMd5.equals( expMd5_2 ) ) {
            Assert.fail( "actMd5 = " + downloadMd5 + "expMd5=[" + expMd5 + ", "
                    + expMd5_2 + "]" );
        }
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
            for ( AmazonS3 client : clientList ) {
                if ( client != null ) {
                    client.shutdown();
                }
            }
        }
    }

    class ThreadUploadPart18761 {
        private AmazonS3 inner_s3Client;
        private long partSize;
        private File file;
        private String uploadId;
        private List< PartETag > partEtags = new ArrayList<>();

        public ThreadUploadPart18761( long partSize, File file ) {
            inner_s3Client = CommLib.buildS3Client();
            this.partSize = partSize;
            this.file = file;
            clientList.add( inner_s3Client );
        }

        @ExecuteOrder(step = 1, desc = "初始化分段上传")
        public void initPartUpload() {
            uploadId = PartUploadUtils.initPartUpload( inner_s3Client,
                    bucketName, keyName );
        }

        @ExecuteOrder(step = 2, desc = "分段上传对象")
        public void UploadPart() {
            partEtags = PartUploadUtils.partUpload( inner_s3Client, bucketName,
                    keyName, uploadId, file, partSize );
        }

        @ExecuteOrder(step = 3, desc = "完成分段上传")
        public void CompleteMultipartUpload() {
            PartUploadUtils.completeMultipartUpload( inner_s3Client, bucketName,
                    keyName, uploadId, partEtags );
        }
    }
}
