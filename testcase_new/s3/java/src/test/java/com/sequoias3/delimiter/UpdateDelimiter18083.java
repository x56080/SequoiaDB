package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 空桶更新分隔符 testlink-case: seqDB-18083
 *
 * @author wangkexin
 * @Date 2019.04.12
 * @version 1.00
 */
public class UpdateDelimiter18083 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18083";
    private String userName = "user18083";
    private String roleName = "normal";
    private String delimiter = "%";
    private AmazonS3 s3Client = null;
    private String[] accessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
    }

    @Test
    private void testUpdateDelimiter() throws Exception {
        DelimiterUtils
                .putBucketDelimiter( bucketName, delimiter, accessKeys[ 0 ] );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter,
                accessKeys[ 0 ] );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
