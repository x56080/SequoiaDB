package com.sequoias3.partupload.concurrent;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;

/**
 * @Description seqDB-18686: upload multiple parts concurrently,the length of the parts is different
 *              and there is partNum of 1.
 * @author wuyan
 * @Date 2019.07.27
 * @version 1.00
 */
@Test(groups = "partlistinuseoff")
public class UploadPart18686 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18686";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String filePath = null;
    private int fileSize = 1024 * 580;
    private List<PartETag> partEtags = Collections.synchronizedList(new ArrayList<PartETag>());

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File(S3TestBase.workDir + File.separator + TestTools.getClassName());
        filePath = localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile(localPath);
        TestTools.LocalFile.createDir(localPath.toString());
        TestTools.LocalFile.createFile(filePath, fileSize);
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void uploadParts() throws Exception {
        File file = new File(filePath);
        String uploadId = PartUploadUtils.initPartUpload(s3Client, S3TestBase.bucketName, keyName);

        ThreadExecutor threadExec = new ThreadExecutor();
        int partNums = 6;
        int[] partSizes = { 1024 * 100, 1024 * 200, 1024 * 150, 1024 * 10, 1024 * 20, 1024 * 100 };
        int[] offSets = { 0, 1024 * 100, 1024 * 300, 1024 * 450, 1024 * 460, 1024 * 480 };
        for (int i = 0; i < partNums; i++) {
            int partNum = i + 1;
            int partSize = partSizes[i];
            int offSet = offSets[i];
            threadExec.addWorker(new PartUpload(partNum, partSize, offSet, file, uploadId));
        }
        threadExec.run();

        PartUploadUtils.completeMultipartUpload(s3Client, bucketName, keyName, uploadId, partEtags);

        // check the upload file
        String downfileMd5 = ObjectUtils.getMd5OfObject(s3Client, localPath, bucketName, keyName);
        Assert.assertEquals(downfileMd5, TestTools.getMD5(filePath));
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if (runSuccess) {
                s3Client.deleteObject(S3TestBase.bucketName, keyName);
                TestTools.LocalFile.removeFile(localPath);
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private class PartUpload {
        private int partNum;
        private int partSize;
        private int filePosition;
        private File file;
        private String uploadId;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        private PartUpload(int partNum, int partSize, int filePosition, File file, String uploadId) {
            this.partNum = partNum;
            this.partSize = partSize;
            this.filePosition = filePosition;
            this.file = file;
            this.uploadId = uploadId;
        }

        @ExecuteOrder(step = 1)
        private void partUpload() {
            try {
                UploadPartRequest partRequest = new UploadPartRequest().withFile(file).withFileOffset(filePosition)
                        .withPartNumber(partNum).withPartSize(partSize).withBucketName(bucketName).withKey(keyName)
                        .withUploadId(uploadId);
                UploadPartResult uploadPartResult = s3Client1.uploadPart(partRequest);
                partEtags.add(uploadPartResult.getPartETag());
            } finally {
                if (s3Client1 != null) {
                    s3Client1.shutdown();
                }
            }
        }
    }
}
