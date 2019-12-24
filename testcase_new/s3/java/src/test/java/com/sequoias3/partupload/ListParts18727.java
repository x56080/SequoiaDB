package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.List;

/**
 * @Description seqDB-18727:初始化分段上传后查询分段列表
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class ListParts18727 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client;
    private String key = "/aa/bb/obj18727";
    private String uploadId;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    private void test() throws Exception {
        uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, key );
        ListPartsRequest request = new ListPartsRequest( S3TestBase.bucketName,
                key, uploadId );
        PartListing partList = s3Client.listParts( request );
        List<PartSummary> parts = partList.getParts();
        Assert.assertEquals( parts.size(), 0 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.abortMultipartUpload(
                        new AbortMultipartUploadRequest( S3TestBase.bucketName,
                                key, uploadId ) );
                s3Client.deleteObject( S3TestBase.bucketName, key );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}