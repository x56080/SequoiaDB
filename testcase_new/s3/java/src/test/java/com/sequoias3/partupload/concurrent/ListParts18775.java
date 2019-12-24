package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * test content: 版本: 1 :: 并发上传分段和查询分段列表 testlink-case: seqDB-18775
 *
 * @author wangkexin
 * @Date 2019.8.8
 * @version 1.00
 */
public class ListParts18775 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18775";
    private String keyName = "key18775";
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024 * 1024;
    private long partSize = 2 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String uploadId;
    private List<Integer> expPartNumberList = new ArrayList<>();
    private List<String> expEtagList = new ArrayList<>();
    private List<Integer> existPartNumberList = new CopyOnWriteArrayList<>();
    private List<PartETag> partEtags = new CopyOnWriteArrayList<>();
    private String filePath = null;

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
        s3Client.createBucket( bucketName );
    }

    @Test
    private void testUpload() throws Exception {
        uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        ThreadExecutor es = new ThreadExecutor( 300000 );
        int filePosition = 0;
        for ( int i = 1; filePosition < fileSize; i++ ) {
            es.addWorker( new ThreadUploadPart18775( filePosition, i ) );
            expPartNumberList.add( i );
            expEtagList.add( TestTools
                    .getLargeFilePartMD5( file, filePosition, partSize ) );
            filePosition += partSize;
        }
        es.addWorker( new ThreadListParts18775() );
        es.run();

        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        PartListing listResult = s3Client.listParts( request );
        List<PartSummary> listParts = listResult.getParts();
        List<Integer> actPartNumbersList = new ArrayList<>();
        List<String> actEtagList = new ArrayList<>();
        for ( PartSummary partNumbers : listParts ) {
            int partNumber = partNumbers.getPartNumber();
            String etag = partNumbers.getETag();
            actPartNumbersList.add( partNumber );
            actEtagList.add( etag );
        }
        Assert.assertEquals( actPartNumbersList, expPartNumberList );
        Assert.assertEquals( actEtagList, expEtagList );

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

    class ThreadUploadPart18775 {
        private AmazonS3 s3Client = CommLib.buildS3Client();
        private long filePosition;
        private int partNumber;

        public ThreadUploadPart18775( long filePosition, int partNumber ) {
            this.filePosition = filePosition;
            this.partNumber = partNumber;
        }

        @ExecuteOrder(step = 1, desc = "分段上传对象")
        public void UploadPart() {
            try {
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( filePosition )
                        .withPartNumber( partNumber ).withPartSize( partSize )
                        .withBucketName( bucketName ).withKey( keyName )
                        .withUploadId( uploadId );
                UploadPartResult result = s3Client.uploadPart( partRequest );
                existPartNumberList.add( partNumber );
                partEtags.add( result.getPartETag() );
            } finally {
                s3Client.shutdown();
            }
        }
    }

    class ThreadListParts18775 {
        private AmazonS3 s3Client = CommLib.buildS3Client();

        @ExecuteOrder(step = 1, desc = "查询对象分段列表")
        public void ListParts() {
            ListPartsRequest request = new ListPartsRequest( bucketName,
                    keyName, uploadId );
            PartListing listResult = s3Client.listParts( request );
            List<PartSummary> listParts = listResult.getParts();
            List<Integer> actPartNumbersList = new ArrayList<>();
            for ( PartSummary partNumbers : listParts ) {
                int partNumber = partNumbers.getPartNumber();
                actPartNumbersList.add( partNumber );
            }
            if ( actPartNumbersList.size() > fileSize / partSize ) {
                // 如果list时只有部分part上传actPartNumbersList的大小应该小于总分段数
                Assert.fail( "actPartNumbersList=" + actPartNumbersList );
            }
        }
    }
}
