package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-18748:带prefix、keyMarker和uploadIdMarker匹配查询桶分段上传列表
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class ListMultipartUploads18748 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client;
    private String bucketName = "bucket18748";
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 2 * 1024 * 1024;
    private int maxPartNumber = 2;
    private String[] keys = { "atest18748_0", "/dir1/test18748_1",
            "/dir1/dir2/test18748_2", "/dira/test18748_3", "test18748_4" };
    private List< String > uploadIdsOld = new ArrayList<>();
    private List< String > uploadIdsNew = new ArrayList<>();

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void test() throws Exception {
        // initPartUpload
        for ( String key : keys ) {
            String uploadId = PartUploadUtils.initPartUpload( s3Client,
                    bucketName, key );
            uploadIdsOld.add( uploadId );
        }
        // initPartUpload again
        for ( String key : keys ) {
            String uploadId = PartUploadUtils.initPartUpload( s3Client,
                    bucketName, key );
            uploadIdsNew.add( uploadId );
        }

        // uploadPart, multi part
        for ( int i = 0; i < 2; i++ ) {
            PartUploadUtils.partUpload( s3Client, bucketName, keys[ i ],
                    uploadIdsNew.get( i ), file, fileSize / maxPartNumber );
        }
        // uploadPart, only one part
        for ( int i = 2; i < keys.length; i++ ) {
            PartUploadUtils.partUpload( s3Client, bucketName, keys[ i ],
                    uploadIdsNew.get( i ), file, fileSize );
        }

        // list
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withPrefix( "/dir" ).withKeyMarker( keys[ 2 ] )
                        .withUploadIdMarker( uploadIdsOld.get( 2 ) );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );

        // check results
        List< String > expCommonPrefixes = new ArrayList<>();
        MultiValueMap< String, String > expUploads = new LinkedMultiValueMap< String, String >();
        expUploads.add( keys[ 2 ], uploadIdsNew.get( 2 ) );
        expUploads.add( keys[ 1 ], uploadIdsOld.get( 1 ) );
        expUploads.add( keys[ 1 ], uploadIdsNew.get( 1 ) );
        expUploads.add( keys[ 3 ], uploadIdsOld.get( 3 ) );
        expUploads.add( keys[ 3 ], uploadIdsNew.get( 3 ) );
        PartUploadUtils.checkListMultipartUploadsResults( result,
                expCommonPrefixes, expUploads );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void initFile() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );
    }
}