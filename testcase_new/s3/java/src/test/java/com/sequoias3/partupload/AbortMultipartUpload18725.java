package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
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

/**
 * test content:开启版本控制，更新对象指定上传多个分段，终止分段上传 testlink-case: seqDB-18725
 *
 * @author wangkexin
 * @Date 2019.8.5
 * @version 1.00
 */
public class AbortMultipartUpload18725 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18725";
    private String keyName = "key18725";
    private AmazonS3 s3Client = null;
    private String content = "content18725";
    private long fileSize = 10 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
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
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void testAbortMultipartUpload() throws Exception {
        // 初次上传对象
        s3Client.putObject( bucketName, keyName, content );

        // 更新对象
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );
        // 终止分段上传
        AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                bucketName, keyName, uploadId );
        s3Client.abortMultipartUpload( request );

        // 已手工检查上传元数据表中UploadStatus为3 （终止上传）
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versions = s3Client.listVersions( req );
        Assert.assertEquals( versions.getVersionSummaries().size(), 1 );
        // check
        String expMd5 = TestTools.getMD5( content.getBytes() );
        String downloadMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
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
}
