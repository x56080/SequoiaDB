package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18751:带prefix、keyMarker匹配查询桶分段上传列表
 * @Author wangkexin
 * @Date 2019.08.05
 */

public class ListMultipartUploads18751 extends S3TestBase {
    private boolean runSuccess = false;
    private int partNumber = 3;
    private String bucketName = "bucket18751";
    private String[] keyNames = { "atets18751", "dir1/test18751",
            "dir1/test2/test18751", "dira/test18751", "test18751" };
    private AmazonS3 s3Client = null;
    private long fileSize = partNumber * PartUploadUtils.partLimitMinSize;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

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
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

    }

    @Test
    private void testListMultipartUploads() throws Exception {
        List< String > uploadIds = new ArrayList<>();
        List< String > newUploadIds = new ArrayList<>();
        String uploadId = "";
        for ( String keyName : keyNames ) {
            uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                    keyName );
            uploadIds.add( uploadId );
            uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                    keyName );
            newUploadIds.add( uploadId );
        }
        // 指定对象"dir1/test18751"上传多个分段
        PartUploadUtils.partUpload( s3Client, bucketName, keyNames[ 1 ],
                newUploadIds.get( 1 ), file );

        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        request.setPrefix( "dir" );
        // keyMarKer："dir1/test2/test18751"
        request.setKeyMarker( keyNames[ 2 ] );
        MultipartUploadListing partUploadList = s3Client
                .listMultipartUploads( request );
        List< String > expCommonPrefixes = new ArrayList<>();
        MultiValueMap< String, String > expUploads = new LinkedMultiValueMap< String, String >();
        expUploads.add( keyNames[ 3 ], uploadIds.get( 3 ) );
        expUploads.add( keyNames[ 3 ], newUploadIds.get( 3 ) );
        PartUploadUtils.checkListMultipartUploadsResults( partUploadList,
                expCommonPrefixes, expUploads );
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
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
