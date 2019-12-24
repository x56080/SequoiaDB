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
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @Description seqDB-18734:指定nextPartnumberMarker匹配记录不存在，查询分段列表
 * @Author wangkexin
 * @Date 2019.08.05
 */

public class ListParts18734 extends S3TestBase {
    private boolean runSuccess = false;
    private int partNumber = 5;
    private String bucketName = "bucket18734";
    private String keyName = "key18734";
    private AmazonS3 s3Client = null;
    private long fileSize = partNumber * PartUploadUtils.partLimitMinSize;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

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

    }

    @Test
    private void testListParts() throws Exception {
        List<Integer> expPartNumbersList = Arrays.asList( 1, 2, 3 );
        int maxParts = 3;
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyName );
        PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );
        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        request.setMaxParts( maxParts );
        PartListing listResult = s3Client.listParts( request );
        List<PartSummary> listParts = listResult.getParts();
        List<Integer> actPartNumbersList = new ArrayList<>();
        for ( PartSummary parts : listParts ) {
            int partNumber = parts.getPartNumber();
            actPartNumbersList.add( partNumber );
        }

        Assert.assertEquals( actPartNumbersList, expPartNumbersList );
        Assert.assertEquals( ( int ) listResult.getNextPartNumberMarker(),
                maxParts );

        // 再次查询指定PartNumberMarker匹配记录不存在，返回结果为空
        request.setPartNumberMarker( partNumber );
        listResult = s3Client.listParts( request );
        int actListSize = listResult.getParts().size();
        Assert.assertEquals( actListSize, 0 );
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
