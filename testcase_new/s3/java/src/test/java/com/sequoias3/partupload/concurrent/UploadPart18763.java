package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
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
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.apache.commons.codec.binary.Hex;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * @Description seqDB-18763:相同key相同uploadId并发上传分段
 * @Author wangkexin
 * @Date 2019.08.07
 */
@Test(groups = "partsizelimitoff") public class UploadPart18763
        extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18763";
    private String keyName = "key18763";
    private AmazonS3 s3Client = null;
    private long fileSize = 30 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId = "";
    private List<int[]> samePartNumberList = new ArrayList<>();
    private List<int[]> diffPartNumberList = new ArrayList<>();
    private List<String> expSamePartNumberEtags = new ArrayList<>();
    private List<PartETag> diffPartNumberEtags = new CopyOnWriteArrayList<>();

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

        // 准备相同分段数据，如指定分段号为1，覆盖a:分段长度不同 ，b:分段长度相同
        prepareSamePartNumberList();
        // 准备其他分段数据，如指定分段号为2,3,4,5,6，覆盖a:分段长度不同 ，b:分段长度相同
        prepareDiffPartNumberList();
    }

    @Test
    private void testUpload() throws Exception {
        uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < samePartNumberList.size(); i++ ) {
            es.addWorker( new ThreadUploadSamePart18763(
                    samePartNumberList.get( i ) ) );
        }
        for ( int i = 0; i < diffPartNumberList.size(); i++ ) {
            es.addWorker( new ThreadUploadDiffPart18763(
                    diffPartNumberList.get( i ) ) );
        }
        es.run();
        // 构造预期的PartNumberEtags
        getExpPartNumberEtags();

        // 完成分段上传后检查对象内容与预期结果一致
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, diffPartNumberEtags );
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

    private void prepareSamePartNumberList() throws IOException {
        // parameter: parts{offset, partSize, partNumber}
        int[] parts = { 0, 2 * 1024 * 1024, 1 };
        samePartNumberList.add( parts );
        // expSamePartNumberEtags[part1Md5, part2Md5, part3Md5, ...]
        expSamePartNumberEtags.add( TestTools
                .getFilePartMD5( file, parts[ 0 ], parts[ 1 ] ) );
        parts = new int[] { 0, 3 * 1024 * 1024, 1 };
        samePartNumberList.add( parts );
        expSamePartNumberEtags.add( TestTools
                .getFilePartMD5( file, parts[ 0 ], parts[ 1 ] ) );
        parts = new int[] { 5 * 1024 * 1024, 2 * 1024 * 1024, 1 };
        samePartNumberList.add( parts );
        expSamePartNumberEtags.add( TestTools
                .getFilePartMD5( file, parts[ 0 ], parts[ 1 ] ) );
        parts = new int[] { 0, 1 * 1024 * 1024, 1 };
        samePartNumberList.add( parts );
        expSamePartNumberEtags.add( TestTools
                .getFilePartMD5( file, parts[ 0 ], parts[ 1 ] ) );
        parts = new int[] { 3 * 1024 * 1024, 2 * 1024 * 1024, 1 };
        samePartNumberList.add( parts );
        expSamePartNumberEtags.add( TestTools
                .getFilePartMD5( file, parts[ 0 ], parts[ 1 ] ) );
    }

    private void prepareDiffPartNumberList() throws IOException {
        // parameter: parts{offset, partSize, partNumber}
        int[] parts = new int[] { 10 * 1024 * 1024, 5 * 1024 * 1024, 2 };
        diffPartNumberList.add( parts );
        parts = new int[] { 15 * 1024 * 1024, 5 * 1024 * 1024, 3 };
        diffPartNumberList.add( parts );
        parts = new int[] { 20 * 1024 * 1024, 2 * 1024 * 1024, 4 };
        diffPartNumberList.add( parts );
        parts = new int[] { 25 * 1024 * 1024, 2 * 1024 * 1024, 5 };
        diffPartNumberList.add( parts );
        parts = new int[] { 28 * 1024 * 1024, 1 * 1024 * 1024, 6 };
        diffPartNumberList.add( parts );
    }

    private void getExpPartNumberEtags() {
        // 查询分段上传列表 查出实际上传成功part1的etag值
        String actSamePartNumberEtag = "";
        PartETag samePartEtag = null;
        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        PartListing listResult = s3Client.listParts( request );
        List<PartSummary> listParts = listResult.getParts();
        int partNumber = listParts.get( 0 ).getPartNumber();
        actSamePartNumberEtag = listParts.get( 0 ).getETag();
        samePartEtag = new PartETag( partNumber, actSamePartNumberEtag );

        // 查看相同分段（分段1）的etag值是否正确
        Assert.assertTrue(
                expSamePartNumberEtags.contains( actSamePartNumberEtag ),
                "actSamePartNumberEtag : " + actSamePartNumberEtag
                        + ", expSamePartNumberEtags : " + expSamePartNumberEtags
                        .toString() + ", diffPartNumberEtags : "
                        + diffPartNumberEtags.toString() );

        // 将分段1的PartETag值与其他分段PartETag值放在一起
        diffPartNumberEtags.add( samePartEtag );

        // 找到实际上传的etag对应的分段信息（filepositon和partsize）,计算最终上传对象的完整md5并和实际上传对象的md5进行比较
        int index = expSamePartNumberEtags.indexOf( actSamePartNumberEtag );
        diffPartNumberList.add( 0, samePartNumberList.get( index ) );
    }

    private void checkResult() throws Exception {
        FileInputStream fileInputStream = null;
        int length = ( int ) file.length();
        try {
            MessageDigest md5 = MessageDigest.getInstance( "MD5" );
            fileInputStream = new FileInputStream( file );
            byte[] buffer = new byte[ length ];
            if ( fileInputStream.read( buffer ) != -1 ) {
                for ( int i = 0; i < diffPartNumberList.size(); i++ ) {
                    md5.update( buffer, diffPartNumberList.get( i )[ 0 ],
                            diffPartNumberList.get( i )[ 1 ] );
                }
            }
            String expMd5 = new String( Hex.encodeHex( md5.digest() ) );
            String actMd5 = ObjectUtils
                    .getMd5OfObject( s3Client, localPath, bucketName, keyName );
            Assert.assertEquals( actMd5, expMd5 );
        } finally {
            if ( fileInputStream != null ) {
                fileInputStream.close();
            }
        }
    }

    class ThreadUploadSamePart18763 {
        private AmazonS3 s3Client = CommLib.buildS3Client();
        private long filepositon;
        private long partSize;
        private int partNumber;

        public ThreadUploadSamePart18763( int[] parts ) {
            this.filepositon = parts[ 0 ];
            this.partSize = parts[ 1 ];
            this.partNumber = parts[ 2 ];
        }

        @ExecuteOrder(step = 1, desc = "分段上传对象")
        public void putObject() {
            try {
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( filepositon )
                        .withPartNumber( partNumber ).withPartSize( partSize )
                        .withBucketName( bucketName ).withKey( keyName )
                        .withUploadId( uploadId );
                s3Client.uploadPart( partRequest );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    class ThreadUploadDiffPart18763 {
        private AmazonS3 s3Client = CommLib.buildS3Client();
        private long filepositon;
        private long partSize;
        private int partNumber;

        public ThreadUploadDiffPart18763( int[] parts ) {
            this.filepositon = parts[ 0 ];
            this.partSize = parts[ 1 ];
            this.partNumber = parts[ 2 ];
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
                diffPartNumberEtags.add( uploadPartResult.getPartETag() );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

}
