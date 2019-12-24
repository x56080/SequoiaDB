package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-19320:桶内复制对象，目标对象和删除标记对象同名
 * @author wuyan
 * @Date 2019.09.18
 * @version 1.00
 */
public class CopyObject19320 extends S3TestBase {

    private boolean runSuccess = false;
    private String srcKeyName = "/srcObject19320";
    private String destKeyName = "/dest/object19320";
    private String bucketName = "bucket19320";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 2;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
        // put a deleteTag object
        s3Client.deleteObject( bucketName, destKeyName );
    }

    @Test
    public void testCopyObject() throws Exception {
        s3Client.copyObject( bucketName, srcKeyName, bucketName, destKeyName );

        checkObjectAttributeInfo( bucketName );
        checkObjectContent( bucketName, destKeyName );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        // down file
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttributeInfo( String bucketName )
            throws IOException {
        String currentVersionId = "1";
        String hisVersionId = "0";
        List<String> expVersionIds = new ArrayList<>();
        expVersionIds.add( hisVersionId );
        expVersionIds.add( currentVersionId );

        List<String> actVersionIds = new ArrayList<>();
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( "/dest" ) );
        List<S3VersionSummary> verList = versionList.getVersionSummaries();
        for ( S3VersionSummary versionSummary : verList ) {
            String versionId = versionSummary.getVersionId();
            if ( versionId.equals( currentVersionId ) ) {
                Assert.assertEquals( versionSummary.getETag(),
                        TestTools.getMD5( filePath ) );
                Assert.assertEquals( versionSummary.getSize(), fileSize );
                Assert.assertFalse( versionSummary.isDeleteMarker() );
            } else {
                // the object of history version is deleteTag
                Assert.assertEquals( versionId, hisVersionId );
                Assert.assertTrue( versionSummary.isDeleteMarker() );
            }
            actVersionIds.add( versionId );
        }

        Collections.sort( expVersionIds );
        Collections.sort( actVersionIds );
        Assert.assertEquals( actVersionIds, expVersionIds );
    }
}
