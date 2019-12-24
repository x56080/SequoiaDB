package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18733:带partnumberMarker查询分段列表
 * @Author wangkexin
 * @Date 2019.08.05
 */

public class ListParts18733 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private int partNumber = 5;
    private String bucketName = "bucket18733";
    private String keyName = "key18733";
    private AmazonS3 s3Client = null;
    private long fileSize = partNumber * PartUploadUtils.partLimitMinSize;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId = "";

    @DataProvider(name = "partNumberMarkerProvider")
    public Object[][] generateObjectNumber() {
        // parameter : partNumberMarker, expPartNumbersList
        return new Object[][] {
                // test a : 指定第一个partnumber
                new Object[] { 1, Arrays.asList( 2, 3, 4, 5 ) },
                // test b : 指定中间位置的分段号
                new Object[] { partNumber / 2, Arrays.asList( 3, 4, 5 ) },
                // test c : 指定最后一个分段号
                new Object[] { partNumber, new ArrayList<>() },
                // test d : 指定倒数第二个分段号
                new Object[] { partNumber - 1, Arrays.asList( 5 ) },
                // test e : 指定大于所有分段号的值
                new Object[] { partNumber + 1, new ArrayList<>() } };
    }

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );
    }

    @Test(dataProvider = "partNumberMarkerProvider")
    private void testListParts( Integer partNumberMarker,
            List<Integer> expPartNumbersList ) throws Exception {
        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        request.setPartNumberMarker( partNumberMarker );
        PartListing listResult = s3Client.listParts( request );
        List<PartSummary> listParts = listResult.getParts();
        List<Integer> actPartNumbersList = new ArrayList<>();
        for ( PartSummary parts : listParts ) {
            int partNumber = parts.getPartNumber();
            actPartNumbersList.add( partNumber );
        }

        // check the keyName
        Assert.assertEquals( actPartNumbersList, expPartNumbersList );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateObjectNumber().length ) {
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
