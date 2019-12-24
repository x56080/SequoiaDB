package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-15905:maximum number of buckets to be created *
 * @author wuyan
 * @Date 2018.09.30
 * @version 1.00
 */
public class CreateBucket15905 extends S3TestBase {
    private final int defaultMaxNums = 100;
    private boolean runSuccess = false;
    private String bucketName = "bucket15905";
    private String key = "key15905";
    private String userName = "user15905";
    private String roleName = "normal";
    private List<String> bucketNameList = new ArrayList<String>();
    private File localPath = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        clearBucket( s3Client );
    }

    @Test
    public void testCreateBucket() throws Exception {
        createBucketbyMaxNums();

        // create buckets over maximum number
        try {
            s3Client.createBucket( new CreateBucketRequest( bucketName ) );
            Assert.fail( "create bucket maxnum is 100 by default!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "TooManyBuckets" );
        }

        checkCreateBucketResult( s3Client );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void createBucketbyMaxNums() {
        for ( int i = 0; i < defaultMaxNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            s3Client.createBucket( subBucketName );
            s3Client.putObject( subBucketName, key, "test" + subBucketName );
            bucketNameList.add( subBucketName );
        }
    }

    private void checkCreateBucketResult( AmazonS3 s3Client ) throws Exception {
        // check bucket nums
        List<Bucket> buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), defaultMaxNums );

        List<String> actbucketNameLists = new ArrayList<>();
        for ( int i = 0; i < buckets.size(); i++ ) {
            Bucket bucket = buckets.get( i );
            String actBucketName = bucket.getName();
            String actOwner = bucket.getOwner().getDisplayName();
            actbucketNameLists.add( actBucketName );
            Assert.assertEquals( actOwner, userName );
            // check the object in the bucket
            getObjectAndCheckContent( actBucketName );
        }

        Collections.sort( actbucketNameLists );
        Collections.sort( bucketNameList );
        Assert.assertEquals( actbucketNameLists, bucketNameList );
    }

    private void getObjectAndCheckContent( String bucketName )
            throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        String content = "test" + bucketName;
        String expEtag = TestTools.getMD5( content.getBytes() );
        Assert.assertEquals( downfileMd5, expEtag );
    }

    @SuppressWarnings("deprecation")
    private void clearBucket( AmazonS3 s3Client ) {
        for ( int i = 0; i < defaultMaxNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            if ( s3Client.doesBucketExist( subBucketName ) ) {
                if ( s3Client.doesObjectExist( subBucketName, key ) ) {
                    s3Client.deleteObject( subBucketName, key );
                }
                s3Client.deleteBucket( subBucketName );
            }
        }
    }

}
