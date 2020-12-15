package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;

/**
 * @Description seqDB-19470: 并发配置相同对象acl，权限不同
 * @Author wangkexin
 * @Date 2019.09.24
 */
public class SetObjectAcl19470 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19470";
    private String keyName = "key19470";
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private int threadNum = 100;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, keyName, file );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        Set< Grant > grants = new HashSet< Grant >();
        Random random = new Random();
        for ( int i = 0; i < threadNum; i++ ) {
            int randomIndex = random.nextInt( Permission.values().length );
            Permission permission = Permission.values()[ randomIndex ];
            Grant grant = new Grant( GroupGrantee.AllUsers, permission );
            threadExec.addWorker( new ThreadSetObjectAcl( grant ) );
            grants.add( grant );
        }
        threadExec.run();

        // get object acl list size is 1 and included in grants
        AccessControlList result = s3Client.getObjectAcl( bucketName, keyName );
        List< Grant > actGrants = result.getGrantsAsList();
        Assert.assertEquals( actGrants.size(), 1,
                "exp grants = " + grants.toString() + " , act grants = "
                        + actGrants.toString() );
        Assert.assertTrue( grants.contains( actGrants.get( 0 ) ),
                "exp grants = " + grants.toString() + " , act grants = "
                        + actGrants.toString() );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private class ThreadSetObjectAcl {
        private AmazonS3 s3;
        private Grant grant;

        public ThreadSetObjectAcl( Grant grant ) {
            s3 = CommLib.buildS3Client();
            this.grant = grant;
        }

        @ExecuteOrder(step = 1)
        private void setObjectAcl() {
            try {
                PrivilegeUtils.setObjectAclByBody( s3, bucketName, keyName,
                        grant );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
