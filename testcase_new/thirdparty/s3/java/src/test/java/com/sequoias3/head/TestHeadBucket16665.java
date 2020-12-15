package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.HeadBucketRequest;
import com.amazonaws.services.s3.model.HeadBucketResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: headBucket查询指定返回region信息 testlink-case: seqDB-16665
 *
 * @author wangkexin
 * @Date 2018.12.14
 * @version 1.00
 */

public class TestHeadBucket16665 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16665";
    private String userName = "user16665";
    private String roleName = "normal";
    private String region = "us-east-1";
    private AmazonS3 s3Client = null;

    @BeforeClass(enabled = false)
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test(enabled = false)
    private void testDoesBucketExist() throws Exception {
        s3Client.createBucket( new CreateBucketRequest( bucketName, region ) );
        HeadBucketResult bucketResult = s3Client
                .headBucket( new HeadBucketRequest( bucketName ) );
        Assert.assertEquals( bucketResult.getBucketRegion(), region );
        runSuccess = true;
    }

    @AfterClass(enabled = false)
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
