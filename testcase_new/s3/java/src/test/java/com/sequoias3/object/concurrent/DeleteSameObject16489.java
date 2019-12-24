package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 并发删除同一对象 testlink-case: seqDB-16489
 *
 * @author wangkexin
 * @Date 2019.01.03
 * @version 1.00
 */
public class DeleteSameObject16489 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16489";
    private String bucketName = "bucket16489";
    private String keyName = "key16489";
    private String roleName = "normal";
    private String content = "testContent16489";
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
    public void testDeleteObject() throws Exception {
        DeleteObjectThread deleteSameObject = new DeleteObjectThread();
        deleteSameObject.start( 100 );
        Assert.assertTrue( deleteSameObject.isSuccess(),
                deleteSameObject.getErrorMsg() );
        Assert.assertFalse( s3Client.doesObjectExist( bucketName, keyName ) );
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

    private class DeleteObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                s3Client.deleteObject( bucketName, keyName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
