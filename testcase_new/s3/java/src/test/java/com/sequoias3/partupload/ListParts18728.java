package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
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
 * @Description seqDB-18728: list parts after part uploaded.
 * @author wuyan
 * @Date 2019.07.31
 * @version 1.00
 */
public class ListParts18728 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18728";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 20;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void listParts() throws Exception {
        File file = new File( filePath );
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, keyName );
        long partSize = 1024 * 1024 * 5;
        List<PartETag> partEtags = PartUploadUtils
                .partUpload( s3Client, S3TestBase.bucketName, keyName, uploadId,
                        file, partSize );
        listPartsAndCheckPartNumbers( s3Client, S3TestBase.bucketName, keyName,
                partEtags, uploadId, partSize );

        // add upload a part
        int partNumber = 5;
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( 0 ).withPartNumber( partNumber )
                .withPartSize( partSize )
                .withBucketName( S3TestBase.bucketName ).withKey( keyName )
                .withUploadId( uploadId );
        UploadPartResult uploadPartResult = s3Client.uploadPart( partRequest );
        partEtags.add( uploadPartResult.getPartETag() );
        listPartsAndCheckPartNumbers( s3Client, S3TestBase.bucketName, keyName,
                partEtags, uploadId, partSize );

        // check listparts no upload part after completeMultipartUpload
        PartUploadUtils
                .completeMultipartUpload( s3Client, S3TestBase.bucketName,
                        keyName, uploadId, partEtags );
        try {
            ListPartsRequest listRequest = new ListPartsRequest( bucketName,
                    keyName, uploadId );
            s3Client.listParts( listRequest );
            Assert.fail( "listParts must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchUpload",
                    "---statuscode=" + e.getStatusCode() );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( S3TestBase.bucketName, keyName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void listPartsAndCheckPartNumbers( AmazonS3 s3Client,
            String bucketName, String keyName, List<PartETag> partEtags,
            String uploadId, long partSize ) {
        List<Integer> expPartNumbersList = new ArrayList<>();
        for ( PartETag expPartNumbers : partEtags ) {
            int partNumber = expPartNumbers.getPartNumber();
            expPartNumbersList.add( partNumber );
        }

        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        PartListing listResult = s3Client.listParts( request );
        List<PartSummary> listParts = listResult.getParts();
        List<Integer> actPartNumbersList = new ArrayList<>();
        for ( PartSummary partNumbers : listParts ) {
            int partNumber = partNumbers.getPartNumber();
            long size = partNumbers.getSize();
            Assert.assertEquals( size, partSize );
            actPartNumbersList.add( partNumber );
        }

        // check the keyName
        Assert.assertEquals( actPartNumbersList, expPartNumbersList,
                "actPartNumbersList:" + actPartNumbersList
                        + "  expPartNumbersList:" + expPartNumbersList
                        .toString() );
    }
}
