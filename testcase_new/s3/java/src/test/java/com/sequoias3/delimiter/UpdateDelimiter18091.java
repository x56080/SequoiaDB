package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: 设置分隔符，指定桶不存在 testlink-case: seqDB-18091
 *
 * @author wangkexin
 * @Date 2019.04.12
 * @version 1.00
 */
public class UpdateDelimiter18091 extends S3TestBase {
    private String bucketName = "bucket18091";
    private String newDelimiter = "%";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    private void testUpdateDelimiter() throws Exception {
        try {
            DelimiterUtils.putBucketDelimiter( bucketName, newDelimiter );
            Assert.fail( "exp fail but found succ." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );

        }
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( s3Client != null ) {
            s3Client.shutdown();
        }
    }
}
