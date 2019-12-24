package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 设置分隔符，指定桶不属于自己 testlink-case: seqDB-18092
 *
 * @author wangkexin
 * @Date 2019.04.13
 * @version 1.00
 */
public class UpdateDelimiter18092 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18092";
    private String userNameA = "user18092a";
    private String userNameB = "user18092b";
    private String roleName = "normal";
    private String newDelimiter = "%";
    private AmazonS3 s3ClientA = null;
    private String[] accessKeysA = null;
    private String[] accessKeysB = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userNameA );
        CommLib.clearUser( userNameB );
        accessKeysA = UserUtils.createUser( userNameA, roleName );
        s3ClientA = CommLib.buildS3Client( accessKeysA[ 0 ], accessKeysA[ 1 ] );
        s3ClientA.createBucket( bucketName );

        accessKeysB = UserUtils.createUser( userNameB, roleName );
    }

    @Test
    private void testUpdateDelimiter() throws Exception {
        try {
            DelimiterUtils.putBucketDelimiter( bucketName, newDelimiter,
                    accessKeysB[ 0 ] );
            Assert.fail( "exp fail but found succ." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
            }
        } finally {
            if ( s3ClientA != null ) {
                s3ClientA.shutdown();
            }
        }
    }
}
