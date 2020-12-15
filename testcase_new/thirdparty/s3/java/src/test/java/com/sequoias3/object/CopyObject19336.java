package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
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
import java.io.IOException;
import java.util.Date;

/**
 *
 * @Description seqDB-19336:指定ifUnModifiedSince和ifModifiedSince条件匹配源对象复制
 * @author wuyan
 * @Date 2019.09.19
 * @version 1.00
 */
public class CopyObject19336 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user19336";
    private String roleName = "normal";
    private String bucketName = "bucket19336";
    private String srcKeyName = "/src/bb%/object19336";
    private String destKeyName = "/dest/object19336";
    private AmazonS3 s3Client = null;
    private int fileSize1 = 1024 * 2;
    private int fileSize2 = 1;
    private File localPath = null;
    private String hisVersionFilePath = null;
    private String curVersionFilePath = null;
    private long lastModifiedTime = 0;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        hisVersionFilePath = localPath + File.separator + "localFile_"
                + fileSize1 + ".txt";
        curVersionFilePath = localPath + File.separator + "localFile_"
                + fileSize1 + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( hisVersionFilePath, fileSize1 );
        TestTools.LocalFile.createFile( curVersionFilePath, fileSize2 );

        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, srcKeyName,
                new File( hisVersionFilePath ) );
        s3Client.putObject( bucketName, srcKeyName,
                new File( curVersionFilePath ) );
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                bucketName, srcKeyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        Date lastModifiedDate = objMetadata.getLastModified();
        lastModifiedTime = lastModifiedDate.getTime();
    }

    @Test
    public void testCopyObject() throws Exception {
        // set date 2 minutes early at the lastModified time
        long beforeTimestamp = lastModifiedTime - 2 * 60 * 1000l;
        Date beforeDate = new Date( beforeTimestamp );

        // set date 2 minutes later than lastModified time
        long afterTimestamp = lastModifiedTime + 2 * 60 * 1000l;
        Date afterDate = new Date( afterTimestamp );

        // copyObject
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, destKeyName );
        request.withUnmodifiedSinceConstraint( afterDate )
                .withModifiedSinceConstraint( beforeDate );
        s3Client.copyObject( request );

        // check the content of destObject
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, destKeyName );
        Assert.assertEquals( downfileMd5,
                TestTools.getMD5( curVersionFilePath ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
