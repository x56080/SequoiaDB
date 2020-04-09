package com.sequoias3.partupload.serial;

import com.amazonaws.services.s3.AmazonS3;
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
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 不开启版本控制，对象较大，分段上传更新对象 testlink-case: seqDB-18713
 *
 * @author wangkexin
 * @Date 2019.8.1
 * @version 1.00
 */
public class UploadPart18713 extends S3TestBase {
    private static final long M = 1024 * 1024L;
    private boolean runSuccess = false;
    private String bucketName = "bucket18713";
    private String keyName = "key18713";
    private AmazonS3 s3Client = null;
    private long oldFileSize = 5 * 1024 * 1024 * 1024L;
    private long newFileSize = 4 * 1024 * 1024 * 1024L;
    private File localPath = null;
    private File oldfile = null;
    private File newfile = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + oldFileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, oldFileSize );
        oldfile = new File( filePath );

        filePath = localPath + File.separator + "localFile_" + newFileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, newFileSize );
        newfile = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    private void testUpload() throws Exception {
        // put object
        long[] partSizes = { 100 * M, 100 * M, 150 * M, 200 * M, 50 * M, 50 * M,
                100 * M, 500 * M, 500 * M, 200 * M, 500 * M, 300 * M, 600 * M,
                100 * M, 300 * M, 1050 * M, 320 * M };
        putObject( partSizes, oldfile );

        long[] partSizes2 = { 100 * M, 500 * M, 200 * M, 150 * M, 300 * M,
                500 * M, 200 * M, 50 * M, 600 * M, 100 * M, 1000 * M, 100 * M,
                296 * M };
        putObject( partSizes2, newfile );

        // check
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void putObject( long[] partSizes, File file ) {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        List< PartETag > partEtags = partUpload( uploadId, partSizes, file );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
    }

    private List< PartETag > partUpload( String uploadId, long partSizes[],
            File file ) {
        List< PartETag > partEtags = new ArrayList<>();
        long filePosition = 0;
        for ( int i = 1; i < partSizes.length + 1; i++ ) {
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( i ).withPartSize( partSizes[ i - 1 ] )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += partSizes[ i - 1 ];
        }
        return partEtags;
    }
}