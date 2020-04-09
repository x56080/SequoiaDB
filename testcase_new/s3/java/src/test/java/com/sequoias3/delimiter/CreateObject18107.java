package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18107: multiple update delimiter , than put object with
 *              the same key.
 * @author wuyan
 * @Date 2019.04.09
 * @version 1.00
 */
public class CreateObject18107 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18107";
    private String keyName = "aa/bb/?cc?test_18107.png";
    private String firstDelimiter = "%";
    private String secondDelimiter = "?";
    private String defaultDelimiter = "/";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testUpdateDelimiter() throws Exception {
        // first update delimiter, than put object
        DelimiterUtils.putBucketDelimiter( bucketName, firstDelimiter );
        String firstContext = "testcreateObject18107";
        s3Client.putObject( bucketName, keyName, firstContext );
        List< String > expCommprefixList1 = new ArrayList<>();
        List< String > expContentList1 = new ArrayList<>();
        expContentList1.add( keyName );
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                firstDelimiter, expCommprefixList1, expContentList1 );

        // second update delimiter, than put object with the same key
        DelimiterUtils.updateDelimiterSuccessAgain( bucketName,
                secondDelimiter );
        String secondContext = "testcreateObjectsecond18107";
        s3Client.putObject( bucketName, keyName, secondContext );
        List< String > expCommprefixList2 = new ArrayList<>();
        expCommprefixList2.add( "aa/bb/?" );
        List< String > expContentList2 = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                secondDelimiter, expCommprefixList2, expContentList2 );

        // third update delimiter, than put object with the same key
        DelimiterUtils.updateDelimiterSuccessAgain( bucketName,
                defaultDelimiter );
        String thirdContext = "testcreateObjectthird18107";
        s3Client.putObject( bucketName, keyName, thirdContext );
        List< String > expCommprefixList3 = new ArrayList<>();
        expCommprefixList3.add( "aa/" );
        List< String > expContentList3 = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                defaultDelimiter, expCommprefixList3, expContentList3 );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

}
