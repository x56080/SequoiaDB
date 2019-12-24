package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-19449:桶不存在，配置和获取桶acl
 * @Author huangxiaoni
 * @Date 2019.09.24
 */

public class SetBucketAcl19449 extends S3TestBase {
    private String tcId = "19449";
    private AmazonS3 adminS3 = null;
    private String bucketName = "bucket" + tcId;

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLib.buildS3Client();
    }

    @Test
    private void test_setBucketAcl() throws Exception {
        try {
            adminS3.setBucketAcl( bucketName, CannedAccessControlList.Private );
            Assert.fail( "expect fail, but actual success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }
    }

    @Test
    private void test_getBucketAcl() throws Exception {
        try {
            adminS3.getBucketAcl( bucketName );
            Assert.fail( "expect fail, but actual success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }
    }

    @AfterClass
    private void tearDown() {
        adminS3.shutdown();
    }
}
