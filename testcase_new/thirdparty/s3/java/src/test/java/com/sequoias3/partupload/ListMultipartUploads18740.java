package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.amazonaws.services.s3.model.PartETag;
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
 * @Description seqDB-18740: lists in-progress multipart uploads by bucket.the
 *              current version has not completed MultipartUpload;the history
 *              version object has completed MultipartUpload.
 * @author wuyan
 * @Date 2019.08.05
 * @version 1.00
 */
public class ListMultipartUploads18740 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18740";
    private String keyName = "/aa/object18740";

    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private File file = null;
    private int fileSize = 1024 * 1024 * 20;

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
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void uploadParts() throws Exception {
        // upload the history version object
        String uploadIdA = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3Client,
                bucketName, keyName, uploadIdA, file );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadIdA, partEtags );

        // upload the current version object, no completeMultipartUpload
        String uploadIdB = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        PartUploadUtils.partUpload( s3Client, bucketName, keyName, uploadIdB,
                file );

        // list multipartUploads,then check the list has only current version
        // object info
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        MultiValueMap< String, String > expUpload = new LinkedMultiValueMap< String, String >();
        expUpload.add( keyName, uploadIdB );
        List< String > expCommonPrefixes = new ArrayList<>();
        PartUploadUtils.checkListMultipartUploadsResults( result,
                expCommonPrefixes, expUpload );
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
