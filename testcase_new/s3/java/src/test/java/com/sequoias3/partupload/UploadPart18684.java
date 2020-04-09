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
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18684:关闭检测开关，上传多个分段存在分段为空
 * @Author wangkexin
 * @Date 2019.07.29
 */
@Test(groups = "partlistinuseoff")
public class UploadPart18684 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18684";
    private String keyName = "key18684";
    private AmazonS3 s3Client = null;
    private long fileSize = 500 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId = "";
    private List< PartETag > partEtags = new ArrayList<>();

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
        long[] offsetArr = { 0, 0, 100 * 1024, 100 * 1024, 300 * 1024,
                500 * 1024 };
        long[] partSizeArr = { 0, 100 * 1024, 0, 200 * 1024, 200 * 1024, 0 };
        int partNumber = 1;
        for ( int i = 0; i < partSizeArr.length; i++ ) {
            uploadPart( offsetArr[ i ], partSizeArr[ i ], partNumber );
            partNumber++;
        }

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

    private void uploadPart( long filepositon, long partSize, int partNumber )
            throws IOException {
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( filepositon ).withPartNumber( partNumber )
                .withPartSize( partSize ).withBucketName( bucketName )
                .withKey( keyName ).withUploadId( uploadId );
        UploadPartResult uploadPartResult = s3Client.uploadPart( partRequest );
        partEtags.add( uploadPartResult.getPartETag() );
        String expPartMd5 = TestTools.getFilePartMD5( file, filepositon,
                partSize );
        String actPartMd5 = uploadPartResult.getPartETag().getETag();
        Assert.assertEquals( actPartMd5, expPartMd5, "part number = "
                + uploadPartResult.getPartETag().getPartNumber() );
    }
}
