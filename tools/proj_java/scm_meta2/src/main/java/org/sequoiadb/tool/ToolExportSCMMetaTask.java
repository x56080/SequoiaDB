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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;

public class ToolExportSCMMetaTask implements TaskBase {
    private static final Logger logger = LoggerFactory.getLogger(ToolExportSCMMetaTask.class);
    private static final String OUTPUT_FILE_SUFFIX = ".meta.json";
    private static final int LOAD_LINES = 100000; // load 10w lines from oid file each time, about 1.14MB
    private Param param;
    private Common common;
    private String outputFileName;

    public ToolExportSCMMetaTask(Param param, Common common) {
        this.param = param;
        this.common = common;
    }

    @Override
    public void init() {
        // going to build the output file
        String fileName = param.getCollectionName() + OUTPUT_FILE_SUFFIX;
        this.outputFileName = Common.createOutputFile(param.getOutputDir(), fileName);
    }

    @Override
    public void doit() {
        ArrayList<String> inputFiles = param.getInputFiles();
        for (String inputFile : inputFiles) {
            try {
                doit(inputFile);
            } catch (Exception e) {
                throw new BaseException(SDBError.SDB_SYS, e);
            }
        }
    }

    private void doit(String oidFileName) throws IOException {
        FileLoader loader = new FileLoader(oidFileName, LOAD_LINES);
        ConcurrentFileWriter writer =
                new ConcurrentFileWriter(this.outputFileName, Common.CACHE_SIZE_2MB, Common.CACHE_SIZE_2MB);
        Sequoiadb sdb = common.getSdb();
        String line = null;
        int iterCount = 0;
        int skipCount = 0;
        int succCount = 0;

        try {
            DBCollection cl = sdb.getCollectionSpace(param.getCSName()).getCollection(param.getCLName());
            writer.openFile();
            while ((line = loader.getLine()) != null) {
                if (iterCount++ % 200 == 0 && !Controller.isRunning()) {
                    logger.info("Interrupted export meta");
                    return;
                }
                if (line.isEmpty() || line.contains(Common.EXCLUDE_LINE_PREFIX)) {
                    // skip empty line and the line start with "# Begin at" in oid file
                    skipCount++;
                    continue;
                }
                // build oid for query
                ObjectId objectId = new ObjectId(line);
                // query meta record from SCM meta table
                BSONObject record = queryMetaRecord(cl, objectId);
                // 正常情况下，不会出现 record 为 null。如果真的有（说明数据 update_succ 文件的 oid 与 真实的元数据不匹配），
                // 已在 readme 要求检查日志是否 "No record return for oid" 报错，并联系开发分析
                if (record != null) {
                    BasicBSONList bsonList = (BasicBSONList) record.get("site_list");
                    record.put("orig_site_list", bsonList);
                    record.removeField("site_list");
                    // write record out to file in json format
                    writer.writeToFile(record.toString());
                    succCount++;
                }
            }
        } finally {
            writer.closeFile();
            common.releaseSdb(sdb);
            logger.info("The number of exported successfully meta record by using file[" + oidFileName + "] is: " + succCount);
        }

        if (iterCount != (skipCount + succCount)) {
            Controller.setHasErr(true);
            logger.warn("Skip count + successfully query record count is not equal to iteration count");
        }
    }

    private BSONObject queryMetaRecord(DBCollection cl, ObjectId objectId) {
        BSONObject matcher = new BasicBSONObject("_id", objectId);
        BSONObject selector = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject("", Common.IDX_DEFAULT_ID);

        // 为了减少 json 文件的大小，通过 selector 来获取有用的字段
        selector.put("_id", "");
        selector.put("data_create_time", "");
        selector.put("data_id", "");
        selector.put("site_list", "");

        BSONObject record = cl.queryOne(matcher, selector, null, hint, 0);
        if (record == null) {
            logger.error("No record return for oid: " + objectId.toString());
        }
        return record;
    }
}
