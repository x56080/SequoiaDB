package com.sequoias3.region;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.util.Md5Utils;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-20403:创建区域使用自动创建模式，设置csRange
 * @author fanyu
 * @Date:2020年01月03日
 * @version 1.00
 */
public class CreateRegion20403 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket20403";
    private String objectNameBase = "object20403-";
    private int objectNum = 20;
    private int dataCSRange = 20;
    private AtomicInteger counter = new AtomicInteger( objectNum );
    private String regionName = "region20403";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    @SuppressWarnings("deprecation")
    public void testRegion() throws Exception {
        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withDataCSShardingType( "year" ).withDataCLShardingType( "year" )
                .withName( regionName ).withDataCSRange( dataCSRange );
        RegionUtils.putRegion( region );
        // create bucket
        s3Client.createBucket( bucketName, regionName );
        // create object on region
        ThreadExecutor executor = new ThreadExecutor( 1000 * 60 * 5 );
        for ( int i = 0; i < objectNum; i++ ) {
            executor.addWorker( new CreateObject() );
        }
        executor.run();
        //check result
        checkResults();
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
            s3Client.shutdown();
        }
    }

    private void checkResults() throws IOException {
        String prefix = RegionUtils
                .getDataCSName( regionName, "year", new Date() );
        List< String > csList = RegionUtils.listCS( prefix );
        List< Integer > csNumList = new ArrayList<>();
        Assert.assertTrue( csList.size() > 0, csList.toString() );
        // get dataCS index
        for ( String csName : csList ) {
            csNumList.add( Integer.parseInt(
                    csName.substring( csName.lastIndexOf( "_" ) + 1,
                            csName.length() ) ) );
        }
        Collections.sort( csNumList );
        // 0 < csNum <= dataCSRange
        Assert.assertTrue( csNumList.get( 0 ) >= 0, csList.toString() );
        Assert.assertTrue( csNumList.get( csNumList.size() - 1 ) <= dataCSRange,
                csList.toString() );
        //check object content
        for ( int i = 1; i <= objectNum; i++ ) {
            S3Object obj = s3Client.getObject( bucketName, objectNameBase + i );
            Assert.assertEquals( Md5Utils.md5AsBase64( obj.getObjectContent() ),
                    Md5Utils.md5AsBase64( String.valueOf( i ).getBytes() ),
                    "bucketName = " + bucketName + ",objectName = " +
                            objectNameBase + i );
        }
    }

    class CreateObject {
        @ExecuteOrder(step = 1)
        public void createObject() {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                int i = counter.getAndDecrement();
                s3Client.putObject( bucketName, objectNameBase + i,
                        String.valueOf( i ) );
            } finally {
                s3Client.shutdown();
            }
        }
    }
}
