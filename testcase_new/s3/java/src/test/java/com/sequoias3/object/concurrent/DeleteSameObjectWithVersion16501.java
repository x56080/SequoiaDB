package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 开启版本控制，并发删除同一对象 testlink-case: seqDB-16501
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class DeleteSameObjectWithVersion16501 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16501";
    private String bucketName = "bucket16501";
    private String keyName = "key16501";
    private String roleName = "normal";
    private String content = "testContent16501";
    private String deleteVersionId = "1";
    private List<String> expEtag = new ArrayList<>();
    private List<String> expVersionId = new ArrayList<>();
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put three versions of the object
        for ( int i = 0; i < 3; i++ ) {
            String currentContent = content + ObjectUtils.getRandomString( i );
            PutObjectResult result = s3Client
                    .putObject( bucketName, keyName, currentContent );
            expVersionId.add( result.getVersionId() );
            expEtag.add( TestTools.getMD5( currentContent.getBytes() ) );
        }

        // version id : 0-102
        for ( int i = 3; i < 103; i++ ) {
            expVersionId.add( String.valueOf( i ) );
        }

        expEtag.remove( Integer.parseInt( deleteVersionId ) );
        expVersionId.remove( deleteVersionId );
    }

    @Test
    public void testDeleteObject() throws Exception {
        // test a : Delete the same object without specifying version (version
        // id is : 0,2 )
        DeleteObjectThread deleteSameObject = new DeleteObjectThread();
        deleteSameObject.start( 100 );
        Assert.assertTrue( deleteSameObject.isSuccess(),
                deleteSameObject.getErrorMsg() );

        // test b : Delete the same object with the specified version (version
        // id is : 0,1,2,3,4,5,...,102)
        DeleteObjectWithVersionThread deleteObjectWithVersion = new DeleteObjectWithVersionThread(
                deleteVersionId );
        deleteObjectWithVersion.start( 100 );
        Assert.assertTrue( deleteObjectWithVersion.isSuccess(),
                deleteObjectWithVersion.getErrorMsg() );

        // check
        Assert.assertFalse( s3Client.doesObjectExist( bucketName, keyName ) );
        checkVersionResult();
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

    private void checkVersionResult() {
        List<String> actEtg = new ArrayList<>();
        List<String> actVersionId = new ArrayList<>();
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List<S3VersionSummary> objectVersionList = versionList
                .getVersionSummaries();
        Assert.assertEquals( objectVersionList.size(), 102,
                "the number of results returned is incorrect!" );
        for ( int i = 0; i < objectVersionList.size(); i++ ) {
            if ( i < 2 ) {
                Assert.assertFalse(
                        objectVersionList.get( i ).isDeleteMarker() );
                actEtg.add( objectVersionList.get( i ).getETag() );
            } else {
                Assert.assertTrue(
                        objectVersionList.get( i ).isDeleteMarker() );
                Assert.assertEquals( objectVersionList.get( i ).getETag(),
                        null );
            }
            actVersionId.add( objectVersionList.get( i ).getVersionId() );
        }
        Collections.sort( actVersionId );
        Collections.sort( expVersionId );
        Assert.assertEquals( actVersionId, expVersionId );

        Collections.reverse( expEtag );
        Assert.assertEquals( actEtg, expEtag );
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

    private class DeleteObjectWithVersionThread extends S3ThreadBase {
        String versionId;

        public DeleteObjectWithVersionThread( String versionId ) {
            this.versionId = versionId;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                s3Client.deleteVersion( bucketName, keyName, versionId );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
