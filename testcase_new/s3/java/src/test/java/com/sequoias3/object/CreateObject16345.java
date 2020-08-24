package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description: 开启版本控制，增加多个同名对象 testlink-case: seqDB-16345
 *
 * @author wangkexin
 * @Date 2018.11.12
 * @version 1.00
 */
public class CreateObject16345 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16345";
    private String keyName = "object16345";
    private int countNum = 20;
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String expContent = "object_file16345";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testPutObject() throws Exception {
        List< String > contentList = new ArrayList<>();
        for ( int i = 0; i < countNum; i++ ) {
            String currentExpContent = expContent + "." + i;
            s3Client.putObject( bucketName, keyName, currentExpContent );
            contentList.add( currentExpContent );
        }
        checkPutObjectResult( contentList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
    }

    private void checkPutObjectResult( List< String > contentList )
            throws Exception {
        // Objects in the version list are stored in reverse order by versionId
        Collections.reverse( contentList );
        VersionListing listing = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List< S3VersionSummary > list = listing.getVersionSummaries();
        Assert.assertEquals( list.size(), countNum );

        // check object content by md5
        for ( int i = 0; i < list.size(); i++ ) {
            Assert.assertEquals(
                    Integer.parseInt( list.get( i ).getVersionId() ),
                    ( list.size() - 1 ) - i, "versionid is wrong!" );
            String actMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                    bucketName, keyName, list.get( i ).getVersionId() );
            Assert.assertEquals( actMd5,
                    TestTools.getMD5( contentList.get( i ).getBytes() ),
                    "md5 is different!" );
            Assert.assertEquals( list.get( i ).getSize(),
                    ( long ) contentList.get( i ).length(), "size is wrong!" );
        }
    }
}
