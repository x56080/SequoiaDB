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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ToolExportSCMLobInfoTask implements TaskBase {
    private static final Logger logger = LoggerFactory.getLogger(ToolExportSCMLobInfoTask.class);
    private static final String OUTPUT_FILE_SUFFIX = ".lobinfo.json";
    private static final String INPUT_FILE_SCM_SITES = "scm.sites";
    private static final String INPUT_FILE_LOBTABLES_SUFFIX = "lob_tables";
    private static final String INPUT_FILE_LOBTABLES_MAINSITE_BEGIN = "# Main-Site-Begin";
    private static final String INPUT_FILE_LOBTABLES_MAINSITE_END = "# Main-Site-End";
    private static final String INPUT_FILE_LOBTABLES_SUBSITE_BEGIN = "# Sub-Site-Begin";
    private static final String INPUT_FILE_LOBTABLES_SUBSITE_END = "# Sub-Site-End";

    private Param param;
    private String outputFileName;
    private String inputSitesFile;
    private String inputLobTablesFile;
    private Map<Integer, String> siteInfoMap = new HashMap<>();
    private List<String> mainSiteTables = new ArrayList<>();
    private List<String> subSiteTables = new ArrayList<>();

    public ToolExportSCMLobInfoTask(Param param) {
        this.param = param;
    }

    @Override
    public void init() {
        // going to build the output file
        this.outputFileName = Common.createOutputFile(param.getOutputDir(),
                param.getCollectionName() + OUTPUT_FILE_SUFFIX);

        // init input files
        List<String> inputFiles = param.getInputFiles();
        for (String filePath : inputFiles) {
            String fileName = Common.getFileName(filePath);
            if (fileName.equals(INPUT_FILE_SCM_SITES) && (this.inputSitesFile == null || this.inputSitesFile.isEmpty())) {
                this.inputSitesFile = filePath;
            }
            if (fileName.contains(INPUT_FILE_LOBTABLES_SUFFIX) && (this.inputLobTablesFile == null || this.inputLobTablesFile.isEmpty())) {
                this.inputLobTablesFile = filePath;
            }
        }
        if (this.inputSitesFile == null || this.inputSitesFile.isEmpty() ||
                this.inputLobTablesFile == null || this.inputLobTablesFile.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid input files for exporting lob info: " + inputFiles);
        }

        // parse the input files
        List<String> siteInfos = null;
        try {
            siteInfos = FileLoader.getAllLines(this.inputSitesFile);
        } catch (Exception e) {
            String errMsg = "Failed to load lines from file: " + this.inputSitesFile;
            logger.error(errMsg + ", e: " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
        List<String> tableInfos = null;
        try {
            tableInfos = FileLoader.getAllLines(this.inputLobTablesFile);
        } catch (Exception e) {
            String errMsg = "Failed to load lines from file: " + this.inputLobTablesFile;
            logger.error(errMsg + ", e: " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }

        // init site map
        for (String info : siteInfos) {
            if (info.isEmpty()) {
                continue;
            }
            String[] array = info.split(",");
            if (array.length != 2) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid site info[" + info
                        + "] in file: " + this.inputSitesFile);
            }
            int siteId = Integer.parseInt(array[0]);
            if (!this.siteInfoMap.containsKey(siteId)) {
                this.siteInfoMap.put(siteId, array[1]);
            }
        }
        for (Map.Entry<Integer, String> entry : this.siteInfoMap.entrySet()) {
            logger.info("Site info is: id: " + entry.getKey() + ", address: " + entry.getValue());
        }
        if (!this.siteInfoMap.containsKey(1)) {
            String errMsg = "No main site info in file: " + this.inputSitesFile;
            logger.error(errMsg);
            throw new BaseException(SDBError.SDB_INVALIDARG, errMsg);
        }
        // check the site addresses are ok or not
        for (Map.Entry<Integer, String> entry : this.siteInfoMap.entrySet()) {
            String address = entry.getValue();
            if (!Common.isAddressUsable(address, param.getUserName(), param.getPassword())) {
                String errMsg = "Failed to connect to address: " + address;
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
        }

        // init table list, we scan two rounds
        // 第一轮，扫描获取主中心的 lob 表名
        boolean hasBegin = false;
        for (String info : tableInfos) {
            info = info.trim();
            // 跳过所有内容，直到遇到 "# Main-Site-Begin" 才开始处理内容
            if (!hasBegin && !isMainSiteBegin(info)) {
                continue;
            }
            if (isMainSiteBegin(info)) { // 第一次遇到 "# Main-Site-Begin"，标记准备开始解析行内容
                hasBegin = true;
                continue;
            }
            if (isMainSiteEnd(info)) { // 如果遇到了 "# Main-Site-End", 就结束扫描
                break;
            }
            // 跳过空行
            if (info.isEmpty()) {
                continue;
            }
            // 判断集合名字是否为合法的 cs.cl
            if (!Common.isValidCollectionName(info)) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid table name[" + info
                        + "] in file: " + this.inputLobTablesFile);
            }
            // 把主站点的 lob表 放到 mainSiteTables 中
            if (!mainSiteTables.contains(info)) {
                mainSiteTables.add(info);
            }
        }
        // 第二轮，扫描获取分中心的 lob 表名
        hasBegin = false;
        for (String info : tableInfos) {
            info = info.trim();
            // 跳过所有内容，直到遇到 "# Sub-Site-Begin" 才开始处理内容
            if (!hasBegin && !isSubSiteBegin(info)) {
                continue;
            }
            if (isSubSiteBegin(info)) { // 第一次遇到 "# Sub-Site-Begin"，标记准备开始解析行内容
                hasBegin = true;
                continue;
            }
            if (isSubSiteEnd(info)) { // 如果遇到了 "# Sub-Site-End", 就结束扫描
                break;
            }
            // 跳过空行
            if (info.isEmpty()) {
                continue;
            }
            // 判断集合名字是否为合法的 cs.cl
            if (!Common.isValidCollectionName(info)) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid table name[" + info
                        + "] in file: " + this.inputLobTablesFile);
            }
            // 把分站点的 lob表 放到 subSiteTables 中
            if (!subSiteTables.contains(info)) {
                subSiteTables.add(info);
            }
        }
        logger.info("The lob tables in main site are: " + mainSiteTables);
        logger.info("The lob tables in sub sites are: " + subSiteTables);
    }

    private boolean isMainSiteBegin(String line) {
        if (line.equals(INPUT_FILE_LOBTABLES_MAINSITE_BEGIN)) {
            return true;
        }
        return false;
    }

    private boolean isMainSiteEnd(String line) {
        if (line.equals(INPUT_FILE_LOBTABLES_MAINSITE_END)) {
            return true;
        }
        return false;
    }

    private boolean isSubSiteBegin(String line) {
        if (line.equals(INPUT_FILE_LOBTABLES_SUBSITE_BEGIN)) {
            return true;
        }
        return false;
    }

    private boolean isSubSiteEnd(String line) {
        if (line.equals(INPUT_FILE_LOBTABLES_SUBSITE_END)) {
            return true;
        }
        return false;
    }

    @Override
    public void doit() {
        ConcurrentFileWriter fileWriter =
                new ConcurrentFileWriter(this.outputFileName, Common.CACHE_SIZE_2MB, Common.CACHE_SIZE_2MB);
        try {
            getMainSiteLobInfo(fileWriter);
            getSubSiteLobInfo(fileWriter);
        } finally {
            fileWriter.closeFile();
        }
    }

    private void getMainSiteLobInfo(ConcurrentFileWriter writer) {
        // get main site address, main site id is 1
        String address = siteInfoMap.get(1);
        for (String collectionName : mainSiteTables) {
            getSiteLobInfo(1, collectionName, address, param.getUserName(), param.getPassword(), writer);
        }
    }

    private void getSubSiteLobInfo(ConcurrentFileWriter writer) {
        for (Map.Entry<Integer, String> entry : siteInfoMap.entrySet()) {
            // skip main site address, main site id is 1
            if (entry.getKey() == 1) {
                continue;
            }
            int siteId = entry.getKey();
            String address = entry.getValue();
            // 每个分站点都需要对所有 分站点的lob表 进行一次 listLob()。部分分站点可能会缺少部分表，
            // 在这种情况下，会报 -34/-23 错误，忽略这些错误即可
            for (String collectionName : subSiteTables) {
                try {
                    getSiteLobInfo(siteId, collectionName, address, param.getUserName(), param.getPassword(), writer);
                } catch (BaseException e) {
                    if (e.getErrorType().equals(SDBError.SDB_DMS_NOTEXIST.getErrorType()) || // -23 error
                            e.getErrorType().equals(SDBError.SDB_DMS_CS_NOTEXIST.getErrorType())) {  // -34 error
                        logger.warn("No collection[" + collectionName +
                                "] in site[id: " + siteId + ", address: " + address + "]");
                        continue;
                    } else {
                        String errMsg = "Failed to export lob info from collection[" + collectionName +
                                "] in site[id: " + siteId + ", address: " + address + "]";
                        logger.error(errMsg);
                        throw e;
                    }
                } catch (Exception e2) {
                    String errMsg = "Failed to export lob info from collection[" + collectionName +
                            "] in site[id: " + siteId + ", address: " + address + "]";
                    logger.error(errMsg + ", e: " + e2.getMessage());
                    throw new BaseException(SDBError.SDB_SYS, errMsg, e2);
                }
            }
        }
    }

    private void getSiteLobInfo(int siteId, String collectionName,
                                String address, String userName, String password,
                                ConcurrentFileWriter writer) {
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        int iterCount = 0;
        try {
            sdb = Common.createSdb(address, userName, password);
            DBCollection cl = Common.getCL(sdb, collectionName);
            cursor = cl.listLobs();
            while (cursor.hasNext()) {
                if (iterCount++ % 200 == 0 && !Controller.isRunning()) {
                    logger.info("Interrupted export lob info");
                    return;
                }
                BSONObject record = cursor.getNext();
                // record is:
                // { "Size" : 351884 ,
                // "Oid" : { "$oid" : "5fc496c1e2d025a3e0427041" } ,
                // "CreateTime" : { "$ts" : 1683690743 , "$inc" : 152000 } ,
                // "ModificationTime" : { "$ts" : 1683690743 , "$inc" : 615000 } ,
                // "Available" : true ,
                // }
                long createTime = Common.timestampToLong((BSONTimestamp) record.get("CreateTime"));
                long modificationTime = Common.timestampToLong((BSONTimestamp) record.get("ModificationTime"));
                // 导入工具无法识别 "$ts" "$inc" 标识的 timestamp 字符串，所以这里提前将其转为 long 类型（单位是毫秒）
                record.removeField("CreateTime");
                record.removeField("ModificationTime");

                // 构建新的 lobinfo，写入 lobinfo 表
                BSONObject newRecord = new BasicBSONObject();
                newRecord.putAll(record);
                newRecord.put("SiteId", siteId);
                newRecord.put("Collection", collectionName);
                newRecord.put("CreateTime", createTime);
                newRecord.put("ModificationTime", modificationTime);

                // write to file
                writer.writeToFile(newRecord.toString());
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            Common.closeSdb(sdb);
        }
        logger.info("Finish export lob info from collection[" + collectionName +
                "] in site[id: " + siteId + ", address: " + address + "], total lob info count is: " + iterCount);
    }
}
