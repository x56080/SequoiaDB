package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
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

/**
 * test content: 开启分段检测，开启版本控制，相同key不同uploadId多次分段上传 testlink-case: seqDB-18710
 *
 * @author wangkexin
 * @Date 2019.7.30
 * @version 1.00
 */
public class UploadPart18710 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18710";
    private String keyName = "key18710";
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024 * 1024;
    private File localPath = null;
    private File oldFile = null;
    private File newFile = null;
    private String oldfilePath = null;
    private String newfilePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        oldfilePath = localPath + File.separator + "localFile_" + fileSize
                + "old.txt";
        newfilePath = localPath + File.separator + "localFile_" + fileSize
                + "new.txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( oldfilePath, fileSize );
        TestTools.LocalFile.createFile( newfilePath, fileSize );
        oldFile = new File( oldfilePath );
        newFile = new File( newfilePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void testUpload() throws Exception {
        String uploadId1 = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        // 分10个分段上传
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadId1, oldFile, 10 * 1024 * 1024 );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId1, partEtags );

        // 再次指定相同key初始化分段上传对象
        String uploadId2 = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        // 分20个分段上传
        partEtags = PartUploadUtils.partUpload( s3Client, bucketName, keyName,
                uploadId2, newFile );

        // 查询分段列表
        checkPartList( 20, uploadId2 );

        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId2, partEtags );
        checkUploadResult();
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

    private void checkPartList( int partNum, String uploadId ) {
        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        PartListing result = s3Client.listParts( request );
        List< PartSummary > parts = result.getParts();
        Assert.assertEquals( parts.size(), partNum );
        for ( int i = 0; i < parts.size(); i++ ) {
            int partNumber = parts.get( i ).getPartNumber();
            Assert.assertEquals( partNumber, i + 1 );
            long partSize = parts.get( i ).getSize();
            Assert.assertEquals( partSize, PartUploadUtils.partLimitMinSize,
                    "partnumber = " + partNumber );
        }
    }

    private void checkUploadResult() throws Exception {
        String actMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName, "1" );
        String expMd5 = TestTools.getMD5( newfilePath );
        Assert.assertEquals( actMd5, expMd5, "version id = 1" );

        actMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath, bucketName,
                keyName, "0" );
        expMd5 = TestTools.getMD5( oldfilePath );
        Assert.assertEquals( actMd5, expMd5, "version id = 0" );
    }
}
