package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * test content: lobPageSize和replSize配置校验 testlink-case: seqDB-18596
 *
 * @author wangkexin
 * @Date 2019.06.26
 * @version 1.00
 */
public class TestPutObject18596 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18596";
    private String userName = "user18596";
    private String[] accessKeys = null;
    private String keyName = "key18596";
    private String content = "content18596";
    private File localPath = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, UserCommDefind.normal );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testReputBacket() throws Exception {
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, content );
        String expEtag = TestTools.getMD5( content.getBytes() );
        String actEtag = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( actEtag, expEtag );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
