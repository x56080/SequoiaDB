/**
 * Copyright (C) 2023 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.sequoiadb.tool;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.options.UpdateOption;
import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;

public class ToolUpdateMetaTask implements TaskBase {
    private static final Logger logger = LoggerFactory.getLogger(ToolUpdateMetaTask.class);
    private static final String OUTPUT_FILE_UPDATE_FAIL_SUFFIX = ".update.update_fail";
    private static final String OUTPUT_FILE_UPDATE_SUCC_SUFFIX = ".update.update_succ";
    private static final String INPUT_FILE_SUFFIX = ".update.json";

    private String outputUpdateFail;
    private String outputUpdateSucc;
    private String inputFileName;
    private Param param;
    private Common common;

    public ToolUpdateMetaTask(Param param, Common common) {
        this.param = param;
        this.common = common;
    }

    @Override
    public void init() {
        // going to build the output file, if the output file has existed, throw error
        try {
            this.outputUpdateFail = Common.createOutputFile(param.getOutputDir(),
                    param.getCollectionName() + OUTPUT_FILE_UPDATE_FAIL_SUFFIX, false);
            this.outputUpdateSucc = Common.createOutputFile(param.getOutputDir(),
                    param.getCollectionName() + OUTPUT_FILE_UPDATE_SUCC_SUFFIX, false);
        } catch (Exception e) {
            String errMsg = "Failed to init update.update_fail and update.update_succ files, " +
                    "if they exist, please move and backup it fist";
            logger.error(errMsg + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }

        // init input file
        List<String> inputFiles = param.getInputFiles();
        for (String filePath : inputFiles) {
            String fileName = Common.getFileName(filePath);
            if (fileName.contains(INPUT_FILE_SUFFIX) && (this.inputFileName == null || this.inputFileName.isEmpty())) {
                this.inputFileName = filePath;
            }
        }
        if (this.inputFileName == null || this.inputFileName.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid input file for updating meta info: " + inputFiles);
        }
    }

    @Override
    public void doit() {
        ResultFile updateFailFile = new ResultFile(this.outputUpdateFail);
        ResultFile updateSuccFile = new ResultFile(this.outputUpdateSucc);
        try {
            FileLoader loader = new FileLoader(this.inputFileName, 10000);
            doit(loader, updateFailFile, updateSuccFile);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_SYS, e);
        } finally {
            updateFailFile.close();
            updateSuccFile.close();
        }
    }

    private void doit(FileLoader loader, ResultFile updateFailFile, ResultFile updateSuccFile) throws IOException {
        Sequoiadb sdb = common.getSdb();
        long startTM = System.currentTimeMillis();
        long lastTM = startTM;
        int iterCount = 0;
        int totalSucc = 0;

        try {
            String line = null;
            DBCollection cl = sdb.getCollectionSpace(param.getCSName()).getCollection(param.getCLName());
            while ((line = loader.getLine()) != null) {
                if (iterCount++ % 200 == 0 && !Controller.isRunning()) {
                    logger.info("Interrupted update meta");
                    return;
                }
                if (line.isEmpty()) {
                    // skip empty line
                    continue;
                }
                // build bson
                BasicBSONObject record = (BasicBSONObject) JSON.parse(line);
                // update meta
                boolean isSucc = false;
                try {
                    isSucc = updateMeta(cl, record);
                } catch (Exception e) {
                    logger.warn("Failed to update in collection: " + cl.getFullName() + ", e: " + e.getMessage());
                    isSucc = false;
                }
                ObjectId objectId = (ObjectId) record.get("_id");
                if (!isSucc) {
                    if (sdb.isValid()) {
                        /// 更新失败且连接还有效，将结果 Oid 写入 update_fail 文件
                        updateFailFile.write(objectId);
                    } else {
                        /// 否则，让进程异常退出
                        String errMsg = "Connection is broken for task, so fail to update record with oid: " + objectId.toString();
                        logger.error(errMsg);
                        throw new BaseException(SDBError.SDB_SYS, errMsg);
                    }
                } else {
                    totalSucc++;
                    updateSuccFile.write(objectId);
                }
                // 尝试打印统计信息。每隔 5 min 打印一次
                lastTM = Common.printStatistics("Update", startTM, lastTM, iterCount);
            }
        } finally {
            common.releaseSdb(sdb);
            logger.info("Total update: " + iterCount + " meta records, success: " + totalSucc);
            Common.printStatistics("Update", startTM, lastTM, iterCount, 0);
        }
    }

    public boolean updateMeta(DBCollection coll, BSONObject record) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject updater = new BasicBSONObject();
        UpdateOption option = new UpdateOption().setHint(new BasicBSONObject("", Common.IDX_DEFAULT_ID));

        // matcher is: { $and:[ {"_id" : oid}, {"site_list": bsonList} ] }
        // updater is: { $set: {"site_list": newBsonList} }

        // build matcher
        ObjectId oid = (ObjectId) record.get("_id");
        BasicBSONList origSiteList = (BasicBSONList) record.get("orig_site_list");
        /// build $and:[xxx]
        BasicBSONList andList = new BasicBSONList();
        andList.put(0, new BasicBSONObject("_id", oid));
        andList.put(1, new BasicBSONObject("site_list", origSiteList));
        /// build matcher
        matcher.put("$and", andList);

        // build updater
        BasicBSONList newSiteList = (BasicBSONList) record.get("new_site_list");
        updater.put("$set", new BasicBSONObject("site_list", newSiteList));

        // update
        logger.debug("Update record, matcher: " + matcher.toString() + ", updater: "
                + updater.toString() + ", option: " + option.toString() + " in cl: " + coll.getFullName());
        try {
            UpdateResult updateResult = coll.updateRecords(matcher, updater, option);
            if (updateResult.getModifiedNum() != 1) {
                logger.warn("Failed to update record, matcher: " + matcher.toString() + ", updater: "
                        + updater.toString() + ", option: " + option.toString()
                        + " in cl: " + coll.getFullName() + ", modify num is: " + updateResult.getModifiedNum());
                return false;
            }
        } catch (Exception e) {
            logger.error("Failed to update record, matcher: " + matcher.toString() + ", updater: "
                    + updater.toString() + ", option: " + option.toString() + " in cl: " + coll.getFullName()
                    + ", error: " + e.getMessage());
            return false;
        }

        return true;
    }
}
