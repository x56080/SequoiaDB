package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: listMultipartUploads接口参数校验 testlink-case: seqDB-18810
 *
 * @author wangkexin
 * @Date 2019.8.7
 * @version 1.00
 */
public class ListMultipartUploadsRequest18810 extends S3TestBase {
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void testIllegalParameter() throws Exception {
        // a.接口参数取值合法---已在功能测试中验证
        // b.接口参数取值非法校验，取值为null
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                null );
        try {
            s3Client.listMultipartUploads( request );
            Assert.fail( "when bucketName is null, it should fail." );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when listing multipart uploads" );
        }
    }

    @AfterClass
    private void tearDown() {
        if ( s3Client != null ) {
            s3Client.shutdown();
        }
    }
}