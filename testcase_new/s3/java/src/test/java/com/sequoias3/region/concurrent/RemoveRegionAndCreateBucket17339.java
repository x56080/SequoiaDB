package com.sequoias3.region.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;

/**
 * @Description seqDB-17339: concurrent remove region and create bucket on
 *              region.
 * @author wuyan
 * @Date 2019.1.30
 * @version 1.00
 */
public class RemoveRegionAndCreateBucket17339 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17339";
    private String key = "key17339";
    private String regionName = "region17339";
    private String userName = "user17339";
    private String roleName = "normal";
    private String[] accessKeys;
    private AmazonS3 s3Client = null;
    private List<String> bucketNames = new ArrayList<>();
    private int fileSize = 1024 * 10;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearUser( userName );
        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );
    }

    @Test
    public void testRegion() throws Exception {
        CreateBuckets createBuckets = new CreateBuckets();
        RemoveRegion removeRegion = new RemoveRegion();
        createBuckets.start();
        removeRegion.start();
        if ( removeRegion.isSuccess() && !createBuckets.isSuccess() ) {
            AmazonS3Exception e = ( AmazonS3Exception ) ( createBuckets
                    .getExceptions().get( 0 ) );
            // 404, "NoSuchRegion"
            if ( e.getStatusCode() != 404 ) {
                Assert.fail(
                        "createBucket fail:" + e.getErrorMessage() + "/n e:" + e
                                .getStatusCode() );
            }
            checkRemoveRegionResult( false );
        } else if ( createBuckets.isSuccess() && !removeRegion.isSuccess() ) {
            AmazonS3Exception e = ( AmazonS3Exception ) ( removeRegion
                    .getExceptions().get( 0 ) );
            // -312, "RegionNotEmpty"
            if ( e.getStatusCode() != 409 ) {
                Assert.fail(
                        "remove Region fail:" + e.getErrorMessage() + "/n e:"
                                + e.getStatusCode() );
            }
            checkCreateBucketResult();
            checkRemoveRegionResult( true );
        } else {
            Assert.fail( "unexpected results!" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearUser( userName );
                if ( RegionUtils.headRegion( regionName ) ) {
                    RegionUtils.deleteRegion( regionName );
                }
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkRemoveRegionResult( boolean doesExistRegion )
            throws Exception {
        boolean curDoesExistRegion = RegionUtils.headRegion( regionName );
        // check that the auto create cs have been deleted
        String metaCSName = RegionUtils.getMetaCSName( regionName );
        String dataCSName =
                RegionUtils.getDataCSName( regionName, "year", new Date() )
                        + "_1";
        if ( doesExistRegion ) {
            Assert.assertTrue( curDoesExistRegion );
            Assert.assertTrue( RegionUtils.doesCSExist( metaCSName ),
                    metaCSName );
            Assert.assertTrue( RegionUtils.doesCSExist( dataCSName ),
                    dataCSName );
        } else {
            Assert.assertFalse( curDoesExistRegion );
            Assert.assertFalse( RegionUtils.doesCSExist( metaCSName ),
                    metaCSName );
            Assert.assertFalse( RegionUtils.doesCSExist( dataCSName ),
                    dataCSName );
        }
    }

    private void checkCreateBucketResult() throws Exception {
        // check bucket numst
        AmazonS3 s3Client = CommLib
                .buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        try {
            List<Bucket> buckets = s3Client.listBuckets();
            List<String> actbucketNameLists = new ArrayList<>();
            for ( Bucket bucket : buckets ) {
                String actBucketName = bucket.getName();
                String actRegion = s3Client.getBucketLocation( actBucketName );
                if ( actRegion.equals( regionName ) ) {
                    actbucketNameLists.add( actBucketName );
                }
            }
            Collections.sort( actbucketNameLists );
            Collections.sort( bucketNames );
            Assert.assertEquals( actbucketNameLists, bucketNames );
            // select one bucket check put object.
            createObjectAndCheckResult( s3Client, bucketNames.get( 0 ) );
        } finally {
            s3Client.shutdown();
        }
    }

    private void createObjectAndCheckResult( AmazonS3 s3Client,
            String bucketName ) throws Exception {
        s3Client.putObject( bucketName, key, new File( filePath ) );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private class CreateBuckets extends S3ThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws Exception {
            accessKeys = UserUtils.createUser( userName, roleName );
            AmazonS3 s3Client = CommLib
                    .buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
            try {
                for ( int i = 0; i < 100; i++ ) {
                    String curBucketName = bucketName + "." + i;
                    s3Client.createBucket( curBucketName, regionName );
                    bucketNames.add( curBucketName );
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class RemoveRegion extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            // random waiting time to remove region,impersionate remove and
            // create bucket different time periods.
            int random = ( int ) ( Math.random() * 80 );
            Thread.sleep( random );
            RegionUtils.deleteRegion( regionName );
        }
    }
}
