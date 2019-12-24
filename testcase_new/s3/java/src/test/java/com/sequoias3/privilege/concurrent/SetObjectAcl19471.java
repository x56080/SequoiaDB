package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.EmailAddressGrantee;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Grantee;
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
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

/**
 * @Description seqDB-19471: 并发配置相同桶不同对象acl，权限不同
 * @Author wangkexin
 * @Date 2019.09.24
 */
public class SetObjectAcl19471 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19471";
    private String keyName_base = "key19471";
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private int threadNum = 100;
    private String ownerId;
    private List<Grantee> granteeList = new ArrayList<>();

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
        prepareGranteeList();

    }

    @Test
    private void testSetObjectAcl() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();

        Map<String, Grant> grants = new HashMap<String, Grant>();
        Random random = new Random();
        for ( int i = 0; i < threadNum; i++ ) {
            String keyName = keyName_base + "_" + i;
            s3Client.putObject( bucketName, keyName, file );

            int randomGranteeIndex = random.nextInt( granteeList.size() );
            int randomPermissionIndex = random
                    .nextInt( Permission.values().length );
            Permission permission = Permission
                    .values()[ randomPermissionIndex ];
            Grantee grantee = granteeList.get( randomGranteeIndex );
            Grant grant = new Grant( grantee, permission );

            threadExec.addWorker( new ThreadSetObjectAcl( keyName, grant ) );

            grants.put( keyName, grant );
        }
        threadExec.run();

        for ( Map.Entry<String, Grant> entry : grants.entrySet() ) {
            PrivilegeUtils.checkSetObjectAclResult( s3Client, bucketName,
                    entry.getKey(), entry.getValue() );
        }

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

    private void prepareGranteeList() {
        // granteeList : include owner id, uri(predefined group) and emailAdress
        granteeList.add( new CanonicalGrantee( ownerId ) );
        granteeList.add( GroupGrantee.AllUsers );
        granteeList.add( GroupGrantee.AuthenticatedUsers );
        granteeList.add( GroupGrantee.LogDelivery );
        granteeList.add( new EmailAddressGrantee( "test email adress 19471" ) );
    }

    private class ThreadSetObjectAcl {
        private AmazonS3 s3;
        private String keyName;
        private Grant grant;

        public ThreadSetObjectAcl( String keyName, Grant grant ) {
            s3 = CommLib.buildS3Client();
            this.keyName = keyName;
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
}
