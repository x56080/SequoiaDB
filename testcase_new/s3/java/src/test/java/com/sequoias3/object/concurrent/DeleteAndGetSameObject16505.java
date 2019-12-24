package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
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
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * test content: 开启版本控制，并发删除和获取对象（不指定版本） testlink-case: seqDB-16505
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class DeleteAndGetSameObject16505 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16505";
    private String bucketName = "bucket16505";
    private String keyName = "key16505";
    private String roleName = "normal";
    private String content = "testContent16505";
    private Map<String, String> versionId2Etg = new HashMap<String, String>();
    private File localPath = null;
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, content + "v1" );
        s3Client.putObject( bucketName, keyName, content + "v2" );

        versionId2Etg
                .put( "1", TestTools.getMD5( ( content + "v2" ).getBytes() ) );
        versionId2Etg
                .put( "0", TestTools.getMD5( ( content + "v1" ).getBytes() ) );
    }

    @Test
    public void testDeleteAndGetObject() throws Exception {
        GetObjectThread getObject = new GetObjectThread();
        DeleteObjectThread deleteObject = new DeleteObjectThread();
        getObject.start();
        deleteObject.start();

        Assert.assertTrue( deleteObject.isSuccess(),
                deleteObject.getErrorMsg() );
        Assert.assertTrue( getObject.isSuccess(), getObject.getErrorMsg() );
        Assert.assertFalse( s3Client.doesObjectExist( bucketName, keyName ) );

        checkDeleteResult();
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

    private void checkDeleteResult() {
        Map<String, String> actVersionId2Etg = new HashMap<String, String>();
        VersionListing verList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List<S3VersionSummary> objectVersionList = verList
                .getVersionSummaries();
        Assert.assertEquals( objectVersionList.size(), 3 );
        for ( S3VersionSummary obj : objectVersionList ) {
            Assert.assertEquals( obj.getBucketName(), bucketName,
                    "bucketName is wrong!" );
            Assert.assertEquals( obj.getKey(), keyName, "keyName is wrong!" );
            if ( obj.isDeleteMarker() ) {
                Assert.assertEquals( obj.getVersionId(), "2" );
            } else {
                actVersionId2Etg.put( obj.getVersionId(), obj.getETag() );
            }
        }

        Assert.assertEquals( actVersionId2Etg, versionId2Etg );
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

    private class GetObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                S3Object object = s3Client.getObject( bucketName, keyName );
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile
                        .initDownloadPath( localPath, TestTools.getMethodName(),
                                Thread.currentThread().getId() );
                ObjectUtils.inputStream2File( s3is, downloadPath );
                s3is.close();
                String getObjectMd5 = TestTools.getMD5( downloadPath );
                Assert.assertEquals( getObjectMd5,
                        TestTools.getMD5( ( content + "v2" ).getBytes() ),
                        "md5 is wrong!" );
                Assert.assertEquals( object.getObjectMetadata().getVersionId(),
                        "1" );
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
