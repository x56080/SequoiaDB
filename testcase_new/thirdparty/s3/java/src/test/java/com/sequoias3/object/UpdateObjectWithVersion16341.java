package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-16341: object existence of delete tag, than create an
 *              object of the same name.
 * @author wuyan
 * @Date 2018.11.13
 * @version 1.00
 */
public class UpdateObjectWithVersion16341 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String key = "aa%bb%object16341";
    private String bucketName = "bucket16341";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 100;
    private File localPath = null;
    private String filePath = null;

    @DataProvider(name = "objectParameterProvider")
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : versionStatus and objectVersion
                // the versioning is enabled,the objectVersion is 1
                new Object[] { "Enabled", "1" },
                // the versioning is suspended,the objectVersion is null
                new Object[] { "Suspended", "null" } };
    }

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "objectParameterProvider")
    public void test( String versionStatus, String versionId )
            throws Exception {
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, versionStatus );
        s3Client.deleteObject( bucketName, key );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        checkCreateObjectReslut( bucketName, versionId );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generatePageSize().length ) {
                CommLib.deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkCreateObjectReslut( String bucketName,
            String expVersionId ) throws Exception {
        // get the new object content is the create content
        S3Object object = s3Client.getObject( bucketName, key );
        String versionId = object.getObjectMetadata().getVersionId();
        Assert.assertEquals( versionId, expVersionId );

        // check the content of the create object
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key, versionId );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}
