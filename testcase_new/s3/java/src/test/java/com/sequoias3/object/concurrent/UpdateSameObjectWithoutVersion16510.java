package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
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

import java.util.List;

/**
 * test content: 禁用版本控制，对象已存在，并发更新相同对象 testlink-case: seqDB-16510
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class UpdateSameObjectWithoutVersion16510 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16510";
    private String bucketName = "bucket16510";
    private String keyName = "key16510";
    private String roleName = "normal";
    private String oldContent = "oldcontent16510";
    private String newContent = "newcontent16510";
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        s3Client.putObject( bucketName, keyName, oldContent );
    }

    @Test
    public void testUpdateObject() throws Exception {
        UpdateObjectThread updateSameObject = new UpdateObjectThread();
        updateSameObject.start( 50 );

        Assert.assertTrue( updateSameObject.isSuccess(),
                updateSameObject.getErrorMsg() );

        checkUpdateObjectResult();
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

    private void checkUpdateObjectResult() {
        S3Object obj = s3Client.getObject( bucketName, keyName );
        ObjectMetadata metadata = obj.getObjectMetadata();
        Assert.assertEquals( metadata.getETag(),
                TestTools.getMD5( newContent.getBytes() ) );
        Assert.assertEquals( metadata.getVersionId(), "null" );

        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName );
        VersionListing versionList = s3Client.listVersions( req );
        List< S3VersionSummary > objectVersionList = versionList
                .getVersionSummaries();
        Assert.assertEquals( objectVersionList.size(), 1,
                "the number of object version is incorrect!" );
    }

    private class UpdateObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                s3Client.putObject( bucketName, keyName, newContent );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
