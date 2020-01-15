package com.sequoias3.region.serial;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.Ssh;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

/**
 * @Description: seqDB-17299 ::创建区域配置DataCSShardingType策略小于CLShardingType策略
 *               此用例时间跳变会影响到db日志，所以不上ci
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateRegion17299 extends S3TestBase {
    private String regionName = "region17299";
    private String bucketName = "bucket17299";
    private String objectName = "object17299";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 200;
    private File localPath = null;
    private List<String> filePathList = new ArrayList<String>();
    private List<Date> dateList = new ArrayList<Date>();
    private int fileNum = 3;
    private boolean runSuccess = false;

    /**
     * set the system time for the host
     *
     * @param host
     * @param dateStr,
     *            e.g: yyyyMMdd
     * @throws Exception
     */
    private static void setSystemTime( String host, String dateStr )
            throws Exception {
        Ssh ssh = null;
        try {
            ssh = new Ssh( host, "root", "jenkins" );

            // set date
            String cmd = "date -s \"" + dateStr + "\"";
            ssh.exec( cmd );

            // print local date after set date
            ssh.exec( "date" );
            String localDate = ssh.getStdout().split( "\n" )[ 0 ];
            System.out.println( "host = " + host + ", localDate = " + localDate
                    + ", after set system time" );
        } finally {
            if ( null != ssh ) {
                ssh.disconnect();
            }
        }
    }

    /**
     * restore the system time for the host
     *
     * @param host
     * @throws Exception
     */
    private static void restoreSystemTime( String host ) throws Exception {
        Ssh ssh = null;
        try {
            // hostname sshuser sshpasswd
            ssh = new Ssh( host, "root", "jenkins" );
            String cmd = "ntpdate " + "\"192.168.20.11\"";

            // in case of time server not usable, retry in 1 min
            int times = 60;
            int intervalSec = 1;
            boolean restoreOk = false;
            Exception lastException = null;
            for ( int i = 0; i < times; ++i ) {
                try {
                    ssh.exec( cmd );
                    restoreOk = true;
                    break;
                } catch ( Exception e ) {
                    lastException = e;
                    Thread.sleep( intervalSec );
                }
            }

            // print local date after set date
            ssh.exec( "date" );
            String localDate = ssh.getStdout().split( "\n" )[ 0 ];
            System.out.println( "host = " + host + ", localDate = " + localDate
                    + ", after restore system time" );

            if ( !restoreOk ) {
                throw lastException;
            }
        } finally {
            if ( null != ssh ) {
                ssh.disconnect();
            }
        }
    }

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        String filePath = null;
        for ( int i = 0; i < fileNum; i++ ) {
            filePath =
                    localPath + File.separator + "localFile_" + ( fileSize + i )
                            + ".txt";
            TestTools.LocalFile.createFile( filePath, fileSize + i );
            filePathList.add( filePath );
        }
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    private void test() throws Exception {
        // create region
        Region region = new Region();
        region.withDataCSShardingType( "month" )
                .withDataCLShardingType( "year" ).withName( regionName );
        RegionUtils.putRegion( region );

        // create bucket
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
        Calendar cal = Calendar.getInstance();
        cal.set( cal.get( Calendar.YEAR ) - 1, 0, 10 );

        // create object with time
        for ( int i = 0; i < fileNum; i++ ) {
            setSystemTime( S3TestBase.s3HostName, cal.getTime().toString() );
            dateList.add( cal.getTime() );
            s3Client.putObject( bucketName, objectName + "_" + i,
                    new File( filePathList.get( i ) ) );
            cal.set( Calendar.MONTH, cal.get( Calendar.MONTH ) + 1 );
        }

        // check cs and cl
        for ( int i = 0; i < dateList.size(); i++ ) {
            String csName = RegionUtils
                    .getDataCSName( regionName, "month", dateList.get( i ) );
            String clName = RegionUtils
                    .getDataCLName( "year", dateList.get( i ) );
            Assert.assertTrue( RegionUtils.clInCS( csName, clName ),
                    "csName = " + csName + ",clName = " + clName );
        }

        // get object for check
        for ( int i = 0; i < fileNum; i++ ) {
            S3Object s3Object = s3Client
                    .getObject( bucketName, objectName + "_" + i );
            checkObjectMetaAndData( s3Object, filePathList.get( i ),
                    dateList.get( i ) );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                RegionUtils.deleteRegion( regionName );
            }
        } finally {
            restoreSystemTime( S3TestBase.s3HostName );
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkObjectMetaAndData( S3Object object, String filePath,
            Date date ) throws Exception {
        ObjectMetadata metadata = object.getObjectMetadata();
        Assert.assertEquals( metadata.getVersionId(), "null" );
        Assert.assertEquals( metadata.getETag(), TestTools.getMD5( filePath ) );
        Assert.assertTrue( metadata.getLastModified().getTime() / 1000
                        >= date.getTime() / 1000,
                "metadata.getLastModified = " + metadata.getLastModified()
                        .getTime() + ",expDate = " + date.getTime() );
        String downloadPath = TestTools.LocalFile
                .initDownloadPath( localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( object.getObjectContent(), downloadPath );
        Assert.assertEquals( TestTools.getMD5( downloadPath ),
                TestTools.getMD5( filePath ), "filePath = " + filePath );
    }
}
