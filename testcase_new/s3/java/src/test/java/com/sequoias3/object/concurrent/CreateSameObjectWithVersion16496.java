package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
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
 * test content: 开启版本控制，并发增加相同对象 testlink-case: seqDB-16496
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class CreateSameObjectWithVersion16496 extends S3TestBase {
    private final int defaultNums = 100;
    private boolean runSuccess = false;
    private String userName = "user16496";
    private String bucketName = "bucket16496";
    private String content = "content16496";
    private String keyName = "key16496";
    private String roleName = "normal";
    private List<String> expEtags = new ArrayList<>();
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
    public void testCreateObject() throws Exception {
        List<CreateObjectThread> createObjects = new ArrayList<>( 20 );
        for ( int i = 0; i < defaultNums; i++ ) {
            String currContent =
                    content + "." + ObjectUtils.getRandomString( i );
            createObjects.add( new CreateObjectThread( currContent ) );
            expEtags.add( TestTools.getMD5( currContent.getBytes() ) );
        }

        for ( CreateObjectThread createObject : createObjects ) {
            createObject.start();
        }

        for ( CreateObjectThread createObject : createObjects ) {
            Assert.assertTrue( createObject.isSuccess(),
                    createObject.getErrorMsg() );
        }

        checkCreateObjectResult();
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

    private void checkCreateObjectResult() {
        List<String> actEtags = new ArrayList<>();
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List<S3VersionSummary> objectVersionList = versionList
                .getVersionSummaries();
        Assert.assertEquals( objectVersionList.size(), defaultNums );
        for ( S3VersionSummary obj : objectVersionList ) {
            Assert.assertEquals( obj.getBucketName(), bucketName,
                    "bucketName is wrong!" );
            Assert.assertEquals( obj.getKey(), keyName, "keyName is wrong!" );
            actEtags.add( obj.getETag() );
        }
        Collections.sort( expEtags );
        Collections.sort( actEtags );
        Assert.assertEquals( actEtags, expEtags,
                "etag is wrong! , the act etag is :" + actEtags.toString()
                        + ", exp etag is : " + expEtags.toString() );
    }

    private class CreateObjectThread extends S3ThreadBase {
        String content;

        public CreateObjectThread( String content ) {
            this.content = content;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                s3Client.putObject( bucketName, keyName, content );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
