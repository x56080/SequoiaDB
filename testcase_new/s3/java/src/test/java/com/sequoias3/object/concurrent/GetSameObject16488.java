package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * test content: 并发获取同一对象 testlink-case: seqDB-16488
 *
 * @author wangkexin
 * @Date 2019.01.03
 * @version 1.00
 */
public class GetSameObject16488 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16488";
    private String bucketName = "bucket16488";
    private String keyName = "key16488";
    private String roleName = "normal";
    private String content = "testContent16488";
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, content );
    }

    @Test
    public void testGetObject() throws Exception {
        GetObjectThread getSameObject = new GetObjectThread();
        getSameObject.start( 100 );
        Assert.assertTrue( getSameObject.isSuccess(),
                getSameObject.getErrorMsg() );
        runSuccess = true;
    }

    @AfterClass
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

    private void checkGetObjectResult( ObjectMetadata metadata )
            throws IOException {
        Assert.assertEquals( metadata.getETag(),
                TestTools.getMD5( content.getBytes() ), "md5 is wrong!" );
        Assert.assertEquals( metadata.getVersionId(), "null" );
        Assert.assertEquals( metadata.getContentLength(), content.length() );
    }

    private class GetObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                S3Object object = s3Client.getObject( bucketName, keyName );
                ObjectMetadata metadata = object.getObjectMetadata();
                checkGetObjectResult( metadata );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
