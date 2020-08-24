package com.sequoias3.delimiter;

import java.io.File;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-18105: create Region by bucket, create objects,the object
 *              name include delimiter.
 * @author wuyan
 * @Date 2019.04.12
 * @version 1.00
 */
public class CreateObject18105 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18105";
    private String regionName = "region18105";
    private String keyName = "/AA/object18105";
    private int keyNum = 20;
    private String delimiter = "TEST";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024;
    private File localPath = null;
    private String filePath = null;
    private SequoiaS3 regionClient = null;

    @SuppressWarnings("deprecation")
    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        regionClient.createRegion( regionName );
        s3Client.createBucket( bucketName, regionName );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testCreateObject() throws Exception {
        List< String > matchDelimiterKeyList = new ArrayList< >();
        List< String > keyNameList = new ArrayList< >();
        for ( int i = 0; i < keyNum; i++ ) {
            String subKeyName = keyName + "_" + i + "_" + delimiter
                    + "/test.png";
            s3Client.putObject( bucketName, subKeyName, new File( filePath ) );
            keyNameList.add( subKeyName );

            String matchDelimiterKey = keyName + "_" + i + "_" + delimiter;
            matchDelimiterKeyList.add( matchDelimiterKey );
        }

        List< String > expContentList = new ArrayList< >();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                delimiter, matchDelimiterKeyList, expContentList );

        deleteObjectAndBucket( keyNameList );
        deleteRegionAndCheckResult();

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void deleteObjectAndBucket( List< String > keyNameList ) {
        for ( String key : keyNameList ) {
            s3Client.deleteObject( bucketName, key );
        }
        s3Client.deleteBucket( bucketName );
    }

    private void deleteRegionAndCheckResult() throws Exception {
        regionClient.deleteRegion( regionName );

        // check cs clear result
        Date date = Calendar.getInstance().getTime();
        String datacsName = RegionUtils.getDataCSName( regionName,
                DataShardingType.YEAR, date ) + "_1";
        String metacsName = RegionUtils.getMetaCSName( regionName );
        Assert.assertFalse( RegionUtils.doesCSExist( datacsName ) );
        Assert.assertFalse( RegionUtils.doesCSExist( metacsName ) );
    }
}
