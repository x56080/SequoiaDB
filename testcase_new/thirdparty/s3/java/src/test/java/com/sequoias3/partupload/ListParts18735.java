package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
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
import java.util.Arrays;
import java.util.List;

/**
 * @Description seqDB-18735:带partnumberMarker和nextPartnumberMarker查询分段列表
 * @Author wangkexin
 * @Date 2019.08.05
 */

public class ListParts18735 extends S3TestBase {
    private boolean runSuccess = false;
    private int partNumber = 6;
    private String bucketName = "bucket18735";
    private String keyName = "key18735";
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

    }

    @Test
    private void testListParts() throws Exception {
        List< Integer > partNumbers = new ArrayList<>(
                Arrays.asList( 1, 3, 5, 6, 7, 8 ) );
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        partUpload( uploadId, partNumbers );

        int maxParts = 3;
        int partNumberMarker = 1;
        List< Integer > actPartNumbersList = new ArrayList<>();
        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        request.setMaxParts( maxParts );
        request.setPartNumberMarker( partNumberMarker );
        PartListing listResult = s3Client.listParts( request );
        List< PartSummary > listParts = listResult.getParts();
        for ( PartSummary parts : listParts ) {
            int partNumber = parts.getPartNumber();
            actPartNumbersList.add( partNumber );
        }

        // 获取返回的nextPartNumberMarker，再次查询
        int nextPartNumberMarker = listResult.getNextPartNumberMarker();
        request.setPartNumberMarker( nextPartNumberMarker );
        listResult = s3Client.listParts( request );
        listParts = listResult.getParts();
        for ( PartSummary parts : listParts ) {
            int partNumber = parts.getPartNumber();
            actPartNumbersList.add( partNumber );
        }
        // 检查结果
        List< Integer > expPartNumbers = new ArrayList<>();
        expPartNumbers.addAll( partNumbers );
        expPartNumbers.remove( 0 );
        Assert.assertEquals( actPartNumbersList, expPartNumbers );
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

    private List< PartETag > partUpload( String uploadId,
            List< Integer > partNumbers ) {
        List< PartETag > partEtags = new ArrayList<>();
        int filePosition = 0;
        long partSize = PartUploadUtils.partLimitMinSize;
        for ( int i = 0; filePosition < fileSize; i++ ) {
            long eachPartSize = Math.min( partSize, fileSize - filePosition );
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( partNumbers.get( i ) )
                    .withPartSize( eachPartSize ).withBucketName( bucketName )
                    .withKey( keyName ).withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            partEtags.add( uploadPartResult.getPartETag() );
            filePosition += partSize;
        }
        return partEtags;
    }
}
