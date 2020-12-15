package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
import com.amazonaws.services.s3.model.UploadPartRequest;
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
 * @Description seqDB-18736: list parts with partnumberMarker and
 *              maxparts.specified nextPartNumberMarker matching conditions are
 *              inconsistent
 * @author wuyan
 * @Date 2019.08.01
 * @version 1.00
 */
public class ListParts18736 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18730";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 50;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void listParts() throws Exception {
        File file = new File( filePath );
        String uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, keyName );
        List< Integer > partNumbers = partUpload( s3Client,
                S3TestBase.bucketName, keyName, uploadId, file );

        // first listParts,set the partNumberMarker is 2
        ListPartsRequest listRequest = new ListPartsRequest( bucketName,
                keyName, uploadId );
        int maxParts = 4;
        int partNumberMarker = partNumbers.get( 1 );
        listRequest.withMaxParts( maxParts )
                .withPartNumberMarker( partNumberMarker );
        PartListing listResult = s3Client.listParts( listRequest );
        List< Integer > actPartNumbersList1 = getPartNumbers( listResult );
        List< Integer > expPartNumbers1 = partNumbers.subList( partNumberMarker,
                partNumberMarker + maxParts );
        Assert.assertEquals( actPartNumbersList1, expPartNumbers1 );

        // second listParts, reset PartNumberMarker,eg:begin to partNumber is 8
        Integer nextMarker = partNumbers.get( 7 );
        listRequest.setPartNumberMarker( nextMarker );
        listResult = s3Client.listParts( listRequest );
        List< Integer > actPartNumbersList2 = getPartNumbers( listResult );
        List< Integer > expPartNumbers2 = partNumbers.subList( nextMarker,
                partNumbers.size() );
        Assert.assertEquals( actPartNumbersList2, expPartNumbers2 );
        Assert.assertFalse( listResult.isTruncated() );

        AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                S3TestBase.bucketName, keyName, uploadId );
        s3Client.abortMultipartUpload( request );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private List< Integer > partUpload( AmazonS3 s3Client, String bucketName,
            String key, String uploadId, File file ) {
        List< Integer > partNumbers = new ArrayList<>();
        int filePosition = 0;
        int partSize = 1024 * 1024 * 5;
        long fileSize = file.length();
        for ( int i = 1; filePosition < fileSize; i++ ) {
            long eachPartSize = Math.min( partSize, fileSize - filePosition );
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( filePosition )
                    .withPartNumber( i ).withPartSize( eachPartSize )
                    .withBucketName( bucketName ).withKey( key )
                    .withUploadId( uploadId );
            s3Client.uploadPart( partRequest );
            partNumbers.add( i );
            filePosition += partSize;
        }
        return partNumbers;
    }

    private List< Integer > getPartNumbers( PartListing listResult ) {
        List< PartSummary > listParts = listResult.getParts();
        List< Integer > actPartNumbersList = new ArrayList<>();
        for ( PartSummary partNumbers : listParts ) {
            int partNumber = partNumbers.getPartNumber();
            actPartNumbersList.add( partNumber );
        }
        return actPartNumbersList;
    }
}
