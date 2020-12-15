package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
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
 * @Description seqDB-18732:带maxparts查询分段列表
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class ListParts18732 extends S3TestBase {
    private int runSuccessNum = 0;
    private int expRunSuccessNum = 3;
    private AmazonS3 s3Client;
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 25 * 1024 * 1024;
    private int maxPartNumber = 5;
    private String key = "/aa/bb/obj18732";
    private String uploadId;
    private List< PartETag > partETags = new ArrayList<>();

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
        uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, key );
        partETags = PartUploadUtils.partUpload( s3Client, S3TestBase.bucketName,
                key, uploadId, file, fileSize / maxPartNumber );
        PartUploadUtils.listPartsAndCheckPartNumbers( s3Client,
                S3TestBase.bucketName, key, partETags, uploadId );
    }

    @Test
    private void test_ltMaxPartNumber() throws Exception {
        ListPartsRequest request = new ListPartsRequest( S3TestBase.bucketName,
                key, uploadId );
        request.setMaxParts( maxPartNumber - 1 );
        PartListing partList = s3Client.listParts( request );
        List< PartSummary > parts = partList.getParts();
        Assert.assertEquals( parts.size(), maxPartNumber - 1 );
        for ( int i = 0; i < parts.size(); i++ ) {
            PartSummary partSumm = parts.get( i );
            Assert.assertEquals( partSumm.getETag(),
                    partETags.get( i ).getETag() );
            Assert.assertEquals( partSumm.getPartNumber(), i + 1 );
        }
        int nextPartNumberMarker = partList.getNextPartNumberMarker();
        Assert.assertEquals( nextPartNumberMarker, maxPartNumber - 1 );

        // set nextPartNumberMarker
        request.setPartNumberMarker( nextPartNumberMarker );
        PartListing partList2 = s3Client.listParts( request );
        List< PartSummary > parts2 = partList2.getParts();
        Assert.assertEquals( parts2.size(), 1 );
        Assert.assertEquals( parts2.get( 0 ).getETag(),
                partETags.get( maxPartNumber - 1 ).getETag() );
        Assert.assertEquals( parts2.get( 0 ).getPartNumber(), maxPartNumber );

        runSuccessNum++;
    }

    @Test
    private void test_gtMaxPartNumber() throws Exception {
        ListPartsRequest request = new ListPartsRequest( S3TestBase.bucketName,
                key, uploadId );
        request.setMaxParts( maxPartNumber + 1 );
        PartListing partList = s3Client.listParts( request );
        List< PartSummary > parts = partList.getParts();
        Assert.assertEquals( parts.size(), maxPartNumber );
        for ( int i = 0; i < parts.size(); i++ ) {
            PartSummary partSumm = parts.get( i );
            Assert.assertEquals( partSumm.getETag(),
                    partETags.get( i ).getETag() );
            Assert.assertEquals( partSumm.getPartNumber(), i + 1 );
        }
        runSuccessNum++;
    }

    @Test
    private void test_etMaxPartNumber() throws Exception {
        ListPartsRequest request = new ListPartsRequest( S3TestBase.bucketName,
                key, uploadId );
        request.setMaxParts( maxPartNumber );
        PartListing partList = s3Client.listParts( request );
        List< PartSummary > parts = partList.getParts();
        Assert.assertEquals( parts.size(), maxPartNumber );
        for ( int i = 0; i < parts.size(); i++ ) {
            PartSummary partSumm = parts.get( i );
            Assert.assertEquals( partSumm.getETag(),
                    partETags.get( i ).getETag() );
            Assert.assertEquals( partSumm.getPartNumber(), i + 1 );
        }
        runSuccessNum++;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccessNum == expRunSuccessNum ) {
                s3Client.abortMultipartUpload( new AbortMultipartUploadRequest(
                        S3TestBase.bucketName, key, uploadId ) );
                s3Client.deleteObject( S3TestBase.bucketName, key );
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