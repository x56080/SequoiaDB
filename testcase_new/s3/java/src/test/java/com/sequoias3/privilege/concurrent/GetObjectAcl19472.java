package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
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
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19472: 并发查询对象acl
 * @Author wangkexin
 * @Date 2019.09.24
 */
public class GetObjectAcl19472 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19472";
    private String keyName = "key19472";
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String ownerId;
    private Grant expGrant[] = new Grant[ 2 ];
    private int threadNum = 100;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        ownerId = s3Client.getS3AccountOwner().getId();

        s3Client.putObject( bucketName, keyName, file );

        s3Client.setObjectAcl( bucketName, keyName,
                CannedAccessControlList.PublicRead );
        expGrant[ 0 ] = new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl );
        expGrant[ 1 ] = new Grant( GroupGrantee.AllUsers, Permission.Read );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            threadExec.addWorker( new ThreadGetObjectAcl() );
        }
        threadExec.run();
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

    private class ThreadGetObjectAcl {
        private AmazonS3 s3 = CommLib.buildS3Client();
        ;

        @ExecuteOrder(step = 1)
        private void getObjectAcl() {
            try {
                PrivilegeUtils.checkSetObjectAclResult( s3, bucketName, keyName,
                        expGrant );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
