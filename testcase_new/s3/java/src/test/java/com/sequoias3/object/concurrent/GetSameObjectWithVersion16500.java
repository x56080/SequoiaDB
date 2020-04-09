package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 开启版本控制，并发获取同一对象 testlink-case: seqDB-16500
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class GetSameObjectWithVersion16500 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16500";
    private String bucketName = "bucket16500";
    private String keyName = "key16500";
    private String roleName = "normal";
    private String content = "testContent16500";
    private List< String > etagList = new ArrayList<>();
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;
    private File localPath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put three versions of the object
        for ( int i = 0; i < 3; i++ ) {
            String currentContent = content + ObjectUtils.getRandomString( i );
            s3Client.putObject( bucketName, keyName, currentContent );
            etagList.add( TestTools.getMD5( currentContent.getBytes() ) );
        }
    }

    @Test
    public void testGetObject() throws Exception {
        // test a : Getting object without specified versions
        GetObjectThread getObject = new GetObjectThread();
        getObject.start( 100 );
        Assert.assertTrue( getObject.isSuccess(), getObject.getErrorMsg() );

        // test b : Get the same version of the object
        GetSameObjectThread getSameObject = new GetSameObjectThread();
        getSameObject.start( 100 );
        Assert.assertTrue( getSameObject.isSuccess(),
                getSameObject.getErrorMsg() );

        // test c : Getting different versions of objects
        List< GetDifferentObjectThread > getDiffVerObjects = new ArrayList<>();
        for ( int i = 0; i < 3; i++ ) {
            getDiffVerObjects
                    .add( new GetDifferentObjectThread( String.valueOf( i ) ) );
        }

        for ( GetDifferentObjectThread getDiffVerObject : getDiffVerObjects ) {
            getDiffVerObject.start();
        }

        for ( GetDifferentObjectThread getDiffVerObject : getDiffVerObjects ) {
            Assert.assertTrue( getDiffVerObject.isSuccess(),
                    getDiffVerObject.getErrorMsg() );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class GetObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                S3Object object = s3Client.getObject( bucketName, keyName );
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile.initDownloadPath(
                        localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
                ObjectUtils.inputStream2File( s3is, downloadPath );
                s3is.close();
                String getObjectMd5 = TestTools.getMD5( downloadPath );
                Assert.assertEquals( getObjectMd5, etagList.get( 2 ),
                        "md5 is wrong!" );
                ObjectMetadata metadata = object.getObjectMetadata();
                Assert.assertEquals( metadata.getVersionId(), "2" );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class GetSameObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                S3Object object = s3Client.getObject(
                        new GetObjectRequest( bucketName, keyName, "1" ) );
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile.initDownloadPath(
                        localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
                ObjectUtils.inputStream2File( s3is, downloadPath );
                s3is.close();
                String getObjectMd5 = TestTools.getMD5( downloadPath );
                Assert.assertEquals( getObjectMd5, etagList.get( 1 ),
                        "md5 is wrong!" );
                ObjectMetadata metadata = object.getObjectMetadata();
                Assert.assertEquals( metadata.getVersionId(), "1" );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class GetDifferentObjectThread extends S3ThreadBase {
        String versionid;

        public GetDifferentObjectThread( String versionid ) {
            this.versionid = versionid;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                S3Object object = s3Client.getObject( new GetObjectRequest(
                        bucketName, keyName, versionid ) );
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile.initDownloadPath(
                        localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
                ObjectUtils.inputStream2File( s3is, downloadPath );
                s3is.close();
                String getObjectMd5 = TestTools.getMD5( downloadPath );
                Assert.assertEquals( getObjectMd5,
                        etagList.get( Integer.parseInt( versionid ) ),
                        "md5 is wrong!" );
                ObjectMetadata metadata = object.getObjectMetadata();
                Assert.assertEquals( metadata.getVersionId(), versionid );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
