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
 * @Description seqDB-18737:查询桶中对象分段上传列表
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class ListMultipartUploads18737 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client;
    private String bucketName = "bucket18737";
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 9 * 1024 * 1024;
    private int maxPartNumber = 2;
    private String keyBase = "/aa/bb/test18737";
    private String[] keys = { keyBase + "1", keyBase + "2", keyBase + "3",
            keyBase + "4" };

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void test() throws Exception {
        MultiValueMap< String, String > expUploads = new LinkedMultiValueMap< String, String >();
        for ( String key : keys ) {
            // uploadPart multi object
            String uploadId = PartUploadUtils.initPartUpload( s3Client,
                    bucketName, key );
            PartUploadUtils.partUpload( s3Client, bucketName, key, uploadId,
                    file, fileSize / maxPartNumber );
            expUploads.add( key, uploadId );
        }

        // listMultipartUploads
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        List< String > expCommonPrefixes = new ArrayList<>();
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