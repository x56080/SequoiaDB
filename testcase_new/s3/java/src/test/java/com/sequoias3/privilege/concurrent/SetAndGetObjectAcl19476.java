package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
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
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @Description seqDB-19476: 并发配置和获取对象acl
 * @Author wangkexin
 * @Date 2019.09.25
 */
public class SetAndGetObjectAcl19476 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19476";
    private String keyName = "key19476";
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String ownerId;

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
        ownerId = s3Client.getS3AccountOwner().getId();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, keyName, file );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        Grant[] defaultGrant = { new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl ) };
        Grant[] expGrant = { new Grant( new CanonicalGrantee( ownerId ),
                Permission.ReadAcp ),
                new Grant( GroupGrantee.AuthenticatedUsers,
                        Permission.Write ) };

        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadSetObjectAcl( expGrant ) );
        threadExec
                .addWorker( new ThreadGetObjectAcl( defaultGrant, expGrant ) );
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

    private class ThreadSetObjectAcl {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private Grant[] grant;

        public ThreadSetObjectAcl( Grant[] grant ) {
            this.grant = grant;
        }

        @ExecuteOrder(step = 1)
        private void setObjectAcl() {
            try {
                PrivilegeUtils
                        .setObjectAclByBody( s3, bucketName, keyName, grant );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadGetObjectAcl {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private Grant[] defaultGrant;
        private Grant[] expGrant;
        private List<Grant> expGrantsList;

        public ThreadGetObjectAcl( Grant[] defaultGrant, Grant[] expGrant ) {
            this.defaultGrant = defaultGrant;
            this.expGrant = expGrant;
        }

        @ExecuteOrder(step = 1)
        private void getObjectAcl() {
            try {
                AccessControlList result = s3
                        .getObjectAcl( bucketName, keyName );
                List<Grant> actGrantsList = result.getGrantsAsList();
                if ( actGrantsList.size() == defaultGrant.length ) {
                    expGrantsList = new ArrayList<>(
                            Arrays.asList( defaultGrant ) );
                } else if ( actGrantsList.size() == expGrant.length ) {
                    expGrantsList = new ArrayList<>(
                            Arrays.asList( expGrant ) );
                } else {
                    Assert.fail(
                            "act object acl size is wrong : " + actGrantsList
                                    .toString() );
                }
                checkGrantList( actGrantsList, expGrantsList );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }

        private void checkGrantList( List<Grant> actGrantsList,
                List<Grant> expGrantsList ) {
            boolean isEqual = false;
            if ( actGrantsList.size() == expGrantsList.size() && actGrantsList
                    .containsAll( expGrantsList ) && expGrantsList
                    .containsAll( actGrantsList ) ) {
                isEqual = true;
            }
            if ( !isEqual ) {
                Assert.fail(
                        "object acl is wrong! exp grants = " + expGrantsList
                                .toString() + ", act grants = " + actGrantsList
                                .toString() );
            }
        }
    }
}
