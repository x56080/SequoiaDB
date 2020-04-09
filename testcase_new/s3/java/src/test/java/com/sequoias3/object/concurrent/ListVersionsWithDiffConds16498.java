package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * @Description seqDB-16498: enabling bucket versioning,concurrent different
 *              condition query object version list.
 * @author wuyan
 * @Date 2019.1.8
 * @version 1.00
 */
public class ListVersionsWithDiffConds16498 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16498";
    private String roleName = "normal";
    private String[] acessKeys = null;
    private String bucketName = "bucket16498";
    private String key = "%dir-1%bb%object16498";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 200;
    private int updateSize = 1024 * 100;
    private int objectNums = 40;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
    private String delimiter = "_";
    private String prefix = "/dir-1/prefix/test16498";
    private List< String > matchPrefixKeyList = new ArrayList<>();
    private List< String > keyList = new ArrayList<>();

    @BeforeClass
    private void setUp() throws IOException {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );

        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        putObjects( s3Client );
    }

    @Test
    public void testObject() throws Exception {
        ListVersionNoMatchConds listNoMatchConds = new ListVersionNoMatchConds();
        ListVersionWithPrefix listWithPrefix = new ListVersionWithPrefix();
        ListVersionWithPrefixAndDelimiter listWithPrefixAndDelimiter = new ListVersionWithPrefixAndDelimiter();
        listNoMatchConds.start();
        listWithPrefix.start( 10 );
        listWithPrefixAndDelimiter.start( 10 );

        Assert.assertTrue( listNoMatchConds.isSuccess(),
                listNoMatchConds.getErrorMsg() );
        Assert.assertTrue( listWithPrefix.isSuccess(),
                listWithPrefix.getErrorMsg() );
        Assert.assertTrue( listWithPrefixAndDelimiter.isSuccess(),
                listWithPrefixAndDelimiter.getErrorMsg() );

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

    private List< String > listVersionsWithConds( AmazonS3 s3Client,
            List< String > actVersionKeys, String prefix, String delimiter ) {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ).withDelimiter( delimiter ) );
        List< String > commPrefixes = versionList.getCommonPrefixes();
        while ( true ) {
            Iterator< S3VersionSummary > versionIter = versionList
                    .getVersionSummaries().iterator();

            while ( versionIter.hasNext() ) {
                S3VersionSummary vs = versionIter.next();
                String getKey = vs.getKey();
                actVersionKeys.add( getKey );
            }
            if ( versionList.isTruncated() ) {
                versionList = s3Client.listNextBatchOfVersions( versionList );
            } else {
                break;
            }
        }
        return commPrefixes;
    }

    private void putObjects( AmazonS3 s3Client ) {
        int matchprefix = 10;
        String keyName;
        for ( int i = 0; i < objectNums; i++ ) {
            if ( i < matchprefix ) {
                keyName = prefix + "_" + i + TestTools.getRandomString( i );
                matchPrefixKeyList.add( keyName );
                matchPrefixKeyList.add( keyName );
            } else {
                keyName = key + "_" + i;
            }
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
            keyList.add( keyName );
            s3Client.putObject( bucketName, keyName, new File( updatePath ) );
            keyList.add( keyName );
        }
        Collections.sort( keyList );
    }

    private void checkListVersionResult( AmazonS3 s3Client,
            List< String > actVersionKeys, List< String > expVersionKeys,
            List< String > commPrefixes, int commPrefixNums,
            String commPrefix ) {
        Assert.assertEquals( commPrefixes.size(), commPrefixNums );
        // if commonPrefiexs is null,mismatch does not check commonPrefixes
        if ( commPrefixes.size() != 0 ) {
            Assert.assertEquals( commPrefixes.get( 0 ), commPrefix );
        }
        // check the keyName
        Collections.sort( actVersionKeys );
        Assert.assertEquals( actVersionKeys, expVersionKeys );
    }

    private class ListVersionNoMatchConds extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            List< String > actVersionKeys = new ArrayList<>();
            ;
            try {
                // no matching prefix and delimiter
                List< String > commPrefixes = listVersionsWithConds( s3Client,
                        actVersionKeys, null, null );
                checkListVersionResult( s3Client, actVersionKeys, keyList,
                        commPrefixes, 0, null );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListVersionWithPrefix extends S3ThreadBase {

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            List< String > actVersionKeys = new ArrayList<>();
            try {
                List< String > commPrefixes = listVersionsWithConds( s3Client,
                        actVersionKeys, prefix, null );
                // no matching delimiter
                checkListVersionResult( s3Client, actVersionKeys,
                        matchPrefixKeyList, commPrefixes, 0, null );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListVersionWithPrefixAndDelimiter extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            List< String > actVersionKeys = new ArrayList<>();
            try {
                List< String > commPrefixes = listVersionsWithConds( s3Client,
                        actVersionKeys, prefix, delimiter );
                // matching delimiter displays only 1 record
                List< String > expKeys = new ArrayList<>();
                checkListVersionResult( s3Client, actVersionKeys, expKeys,
                        commPrefixes, 1, prefix + delimiter );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
