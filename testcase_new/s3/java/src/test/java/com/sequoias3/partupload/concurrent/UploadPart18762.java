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
 * @Description seqDB-18762:不同key并发上传分段
 * @Author wangkexin
 * @Date 2019.08.07
 */
@Test(groups = "partsizelimitoff") public class UploadPart18762
        extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18762";
    private String[] keyName = { "key18762_0", "key18762_1", "key18762_2" };
    private AmazonS3 s3Client = null;
    private long fileSize = 16 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private File file2 = null;
    private String filePath = null;
    private String filePath2 = null;
    private List<AmazonS3> clientList = Collections
            .synchronizedList( new ArrayList<AmazonS3>() );

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        filePath2 =
                localPath + File.separator + "localFile2_" + fileSize + ".txt";

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
        es.addWorker( new ThreadUploadPart18762( keyName[ 0 ], 4 * 1024 * 1024,
                file ) );
        es.addWorker( new ThreadUploadPart18762( keyName[ 1 ], 4 * 1024 * 1024,
                file2 ) );
        es.addWorker( new ThreadUploadPart18762( keyName[ 2 ], 3 * 1024 * 1024,
                file ) );
        es.run();

        String expMd5_1_3 = TestTools.getMD5( filePath );
        String expMd5_2 = TestTools.getMD5( filePath2 );
        String downloadMd5_1 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName,
                        keyName[ 0 ] );
        String downloadMd5_2 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName,
                        keyName[ 1 ] );
        String downloadMd5_3 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName,
                        keyName[ 2 ] );
        Assert.assertEquals( downloadMd5_1, expMd5_1_3 );
        Assert.assertEquals( downloadMd5_2, expMd5_2 );
        Assert.assertEquals( downloadMd5_3, expMd5_1_3 );
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

    class ThreadUploadPart18762 {
        private String keyName;
        private long partSize;
        private File file;
        private String uploadId;
        private List<PartETag> partEtags = new ArrayList<>();
        private AmazonS3 inner_s3Client = CommLib.buildS3Client();

        public ThreadUploadPart18762( String keyName, long partSize,
                File file ) {
            clientList.add( inner_s3Client );
            this.keyName = keyName;
            this.partSize = partSize;
            this.file = file;
        }

        @ExecuteOrder(step = 1, desc = "初始化分段上传")
        public void initPartUpload() {
            uploadId = PartUploadUtils
                    .initPartUpload( inner_s3Client, bucketName, keyName );
        }

        @ExecuteOrder(step = 2, desc = "分段上传对象")
        public void UploadPart() {
            partEtags = PartUploadUtils
                    .partUpload( inner_s3Client, bucketName, keyName, uploadId,
                            file, partSize );
        }

        @ExecuteOrder(step = 3, desc = "完成分段上传")
        public void CompleteMultipartUpload() {
            PartUploadUtils.completeMultipartUpload( inner_s3Client, bucketName,
                    keyName, uploadId, partEtags );
        }
    }
}
