package com.sequoias3.region;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
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
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-20405:默认区域下，设置csRange
 * @author fanyu
 * @Date:2020年01月04日
 * @version 1.00
 */
@Test(groups = "datacsrange")
public class DefaultRegion20405 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket20405";
    private String objectNameBase = "object20405";
    private int dataCSRange = 10;
    private int objectNum = 20;
    private List< String > csNameList;
    private AtomicInteger counter = new AtomicInteger( objectNum );
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    @SuppressWarnings("deprecation")
    public void testRegion() throws Exception {
        //create bucket
        s3Client.createBucket( bucketName );
        // create object
        ThreadExecutor executor = new ThreadExecutor( 1000 * 60 * 5 );
        for ( int i = 0; i < objectNum; i++ ) {
            executor.addWorker( new CreateObject() );
        }
        executor.run();
        //check result
        checkResults();
        //delete data cs exclude "S3_" + regionName + "_Data_" + new Date()
        // .getYear() + "_1"
        csNameList.remove(
                "S3_SYS_Data_" + Calendar.getInstance().get( Calendar.YEAR ) +
                        "_1" );
        for ( String csName : csNameList ) {
            RegionUtils.dropCS( csName );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkResults() throws IOException {
        String prefix =
                "S3_SYS_Data_" + Calendar.getInstance().get( Calendar.YEAR );
        csNameList = RegionUtils.listCS( prefix );
        List< Integer > csNumList = new ArrayList<>();
        Assert.assertTrue( csNameList.size() > 0, csNameList.toString() );
        // get dataCS index
        for ( String csName : csNameList ) {
            csNumList.add( Integer.parseInt(
                    csName.substring( csName.lastIndexOf( "_" ) + 1,
                            csName.length() ) ) );
        }
        Collections.sort( csNumList );
        // 0 < csNum <= dataCSRange
        Assert.assertTrue( csNumList.get( 0 ) >= 0, csNameList.toString() );
        Assert.assertTrue( csNumList.get( csNumList.size() - 1 ) <= dataCSRange,
                csNameList.toString() );
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
