package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 指定区域创建桶，分段上传对象 testlink-case: seqDB-18716
 *
 * @author wangkexin
 * @Date 2019.8.1
 * @version 1.00
 */
public class UploadPart18716 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region18716";
    private String bucketName = "bucket18716";
    private String SamePartSizekeyName = "samekey18716";
    private String diffPartSizekeyName = "diffkey18716";
    private AmazonS3 s3Client = null;
    private long fileSize = 18 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

    @SuppressWarnings("deprecation")
    @BeforeClass
    private void setUp() throws Exception {
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
        RegionUtils.clearRegion( regionName );

        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );
        s3Client.createBucket( bucketName, regionName );
    }

    @Test
    private void testUpload() throws Exception {
        // 上传分段长度相同
        String uploadId1 = PartUploadUtils.initPartUpload( s3Client, bucketName,
                SamePartSizekeyName );
        List< PartETag > partEtags1 = PartUploadUtils.partUpload( s3Client,
                bucketName, SamePartSizekeyName, uploadId1, file );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                SamePartSizekeyName, uploadId1, partEtags1 );

        // 上传分段长度不同
        long[] partSizes = { 6 * 1024 * 1024, 5 * 1024 * 1024,
                7 * 1024 * 1024 };
        String uploadId2 = PartUploadUtils.initPartUpload( s3Client, bucketName,
                diffPartSizekeyName );
        List< PartETag > partEtags2 = partUpload( uploadId2, partSizes );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName,
                diffPartSizekeyName, uploadId2, partEtags2 );

        // check
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, SamePartSizekeyName );
        Assert.assertEquals( downloadMd5, expMd5 );

        downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, diffPartSizekeyName );
        Assert.assertEquals( downloadMd5, expMd5 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                RegionUtils.deleteRegion( regionName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private List< PartETag > partUpload( String uploadId, long partSizes[] ) {
        List< PartETag > partEtags = new ArrayList<>();
        int filePosition = 0;
        for ( int i = 1; i < partSizes.length + 1; i++ ) {
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( i ).withPartSize( partSizes[ i - 1 ] )
                    .withBucketName( bucketName ).withKey( diffPartSizekeyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += partSizes[ i - 1 ];
        }
        return partEtags;
    }
}
