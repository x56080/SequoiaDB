package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.springframework.web.client.HttpServerErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.util.Date;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: 创建区域，LobPageSize和ReplSize参数校验 testlink-case: seqDB-18611
 *
 * @author wangkexin
 * @Date 2019.06.27
 * @version 1.00
 */
public class CreateRegion18611 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket18611";
    private String key = "key18611";
    private String regionName = "region18611";
    private String invalid_regionName = "invalidregion18611";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private File localPath = null;
    private String filePath = null;

    @DataProvider(name = "LobPageSizeAndReplSizeProvider")
    public Object[][] LobPageSizeAndReplSize() {
        return new Object[][] { new Object[] { "", "", "262144", "-1" },
                new Object[] { "0", "-1", "262144", "-1" },
                new Object[] { "4096", "0", "4096", "7" },
                new Object[] { "8192", "1", "8192", "1" },
                new Object[] { "16384", "2", "16384", "2" },
                new Object[] { "32768", "3", "32768", "3" },
                new Object[] { "65536", "4", "65536", "4" },
                new Object[] { "131072", "5", "131072", "5" },
                new Object[] { "262144", "6", "262144", "6" },
                new Object[] { "524288", "7", "524288", "7" } };
    }

    @DataProvider(name = "invalidLobPageSizeAndReplSizeProvider")
    public Object[][] invalidLobPageSizeAndReplSize() {
        return new Object[][] {
                new Object[] { "123", "-1", "InvalidLobPageSize" },
                new Object[] { "4096", "456", "InvalidReplSize" },
                new Object[] { "111", "222", "InvalidLobPageSize" },
                new Object[] { "abc", "1", "500" },
                new Object[] { "16384", "test", "500" },
                new Object[] { "@%$&*", "3", "500" },
                new Object[] { "65536", "$^%*%^", "500" } };
    }

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
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
        RegionUtils.clearRegion( invalid_regionName );
    }

    @Test(dataProvider = "LobPageSizeAndReplSizeProvider")
    public void testRegion( String dataLobPageSize, String dataReplSize,
            String expDataLobPageSize, String expDataReplSize )
            throws Exception {
        Region region = new Region();
        region.withName( regionName ).withDataLobPageSize( dataLobPageSize )
                .withDataReplSize( dataReplSize );
        RegionUtils.putRegion( region );

        // create object on region
        createObjectAndCheckResult();

        // get region and check region info
        checkRegion( expDataLobPageSize, expDataReplSize );
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.deleteRegion( regionName );
        actSuccessTests.getAndIncrement();
    }

    @Test(dataProvider = "invalidLobPageSizeAndReplSizeProvider")
    public void testRegionWithInvalidValue( String dataLobPageSize,
            String dataReplSize, String errorCode ) throws Exception {
        Region region = new Region();
        region.withName( invalid_regionName )
                .withDataLobPageSize( dataLobPageSize )
                .withDataReplSize( dataReplSize );
        if ( errorCode.equals( "500" ) ) {
            try {
                RegionUtils.putRegion( region );
                Assert.fail( "exp failed but found succeed." );
            } catch ( HttpServerErrorException e ) {
                Assert.assertEquals( e.getStatusCode().toString(), errorCode );
            }
        } else {
            try {
                RegionUtils.putRegion( region );
                Assert.fail( "exp failed but found succeed." );
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(), errorCode );
            }
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == ( LobPageSizeAndReplSize().length
                    + invalidLobPageSizeAndReplSize().length ) ) {
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkRegion( String expDataLobPageSize,
            String expDataReplSize ) throws Exception {
        String datacsName =
                RegionUtils.getDataCSName( regionName, "year", new Date() )
                        + "_1";
        String dataclName = RegionUtils.getDataCLName( "quarter", new Date() );
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ) {
            DBCursor csCursor = sdb
                    .getSnapshot( 5, "{'Name':'" + datacsName + "'}",
                            "{'LobPageSize':{'$include':1}}", null );
            String actLobPageSize = csCursor.getNext().get( "LobPageSize" )
                    .toString();
            Assert.assertEquals( actLobPageSize, expDataLobPageSize );

            DBCursor clCursor = sdb.getSnapshot( 8,
                    "{'Name':'" + datacsName + "." + dataclName + "'}",
                    "{'ReplSize':{'$include':1}}", null );
            String actReplSize = clCursor.getNext().get( "ReplSize" )
                    .toString();
            Assert.assertEquals( actReplSize, expDataReplSize );
        }
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult() throws Exception {
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

}
