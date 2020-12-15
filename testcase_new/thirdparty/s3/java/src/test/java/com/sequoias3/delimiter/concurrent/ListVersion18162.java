package com.sequoias3.delimiter.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18162: current delimiter status is not normal,List
 *              Versions of bucket.
 * @author wuyan
 * @Date 2019.4.11
 * @version 1.00
 */
public class ListVersion18162 extends S3TestBase {
    private boolean runSuccess = false;
    private String newDelimiter = "!";
    private String bucketName = "bucket18162";
    private int versionNum = 3;
    private String[] keyNames = { "/a!test0_18162", "/atest1_18162",
            "/atest2!18162.png", "/test@!3_18162", "/test@a4_18162",
            "/test@!5_18162", "/testa_test6_18162", "/testa_x7!_18162",
            "/y/test8!_18162" };
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < keyNames.length; i++ ) {
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] + "_version1" );
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] + "_version1" );
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] + "_version3" );
        }
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        UpdateDelimiter updateDelimiter = new UpdateDelimiter( newDelimiter );
        ListVersions listVersions = new ListVersions( newDelimiter );
        threadExec.addWorker( updateDelimiter );
        threadExec.addWorker( listVersions );
        threadExec.run();

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

    private class UpdateDelimiter {
        private String delimiter;

        private UpdateDelimiter( String delimiter ) {
            this.delimiter = delimiter;
        }

        @ExecuteOrder(step = 1)
        private void updateDelimiter() {
            DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        }

        @ExecuteOrder(step = 2)
        private void checkDelimiterResult() throws Exception {
            DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
        }
    }

    private class ListVersions {
        private String delimiter;
        private VersionListing vsList;

        private ListVersions( String delimiter ) {
            this.delimiter = delimiter;
        }

        @ExecuteOrder(step = 1)
        private void listVersions() {
            vsList = s3Client.listVersions( new ListVersionsRequest()
                    .withBucketName( bucketName ).withDelimiter( delimiter ) );

        }

        @ExecuteOrder(step = 2)
        private void checkListVersionsResult() {
            List< String > matchPrefixList = new ArrayList<>();
            matchPrefixList.add( "/a!" );
            matchPrefixList.add( "/atest2!" );
            matchPrefixList.add( "/test@!" );
            matchPrefixList.add( "/testa_x7!" );
            matchPrefixList.add( "/y/test8!" );
            MultiValueMap< String, String > expVersions = new LinkedMultiValueMap< String, String >();
            for ( int i = versionNum - 1; i >= 0; i-- ) {
                expVersions.add( keyNames[ 1 ], String.valueOf( i ) );
                expVersions.add( keyNames[ 4 ], String.valueOf( i ) );
                expVersions.add( keyNames[ 6 ], String.valueOf( i ) );
            }
            Assert.assertEquals( vsList.isTruncated(), false,
                    "list.isTruncated() is unexpected!" );
            ObjectUtils.checkListVSResults( vsList, matchPrefixList,
                    expVersions );
        }
    }
}
