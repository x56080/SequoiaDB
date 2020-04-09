package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ObjectMetadata;
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

/**
 * test content: 开启版本控制，并发增加和删除相同对象 testlink-case: seqDB-16503
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class CreateAndDelectSameObject16503 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16503";
    private String bucketName = "bucket16503";
    private String keyName = "key16503";
    private String content = "testContent16503";
    private String roleName = "normal";
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testCreateAndDeleteObject() throws Exception {
        CreateObjectThread createObject = new CreateObjectThread( keyName );
        DeleteObjectThread deleteObject = new DeleteObjectThread( keyName );
        createObject.start();
        deleteObject.start();

        Assert.assertTrue( createObject.isSuccess(),
                createObject.getErrorMsg() );
        Assert.assertTrue( deleteObject.isSuccess(),
                createObject.getErrorMsg() );
        if ( s3Client.doesObjectExist( bucketName, keyName ) ) {
            ObjectMetadata metadata = s3Client.getObjectMetadata( bucketName,
                    keyName );
            Assert.assertEquals( metadata.getETag(),
                    TestTools.getMD5( content.getBytes() ) );
            Assert.assertEquals( metadata.getVersionId(), "1" );
        }
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

    private class CreateObjectThread extends S3ThreadBase {
        String keyName;

        public CreateObjectThread( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                s3Client.putObject( bucketName, keyName, content );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class DeleteObjectThread extends S3ThreadBase {
        String keyName;

        public DeleteObjectThread( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
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
