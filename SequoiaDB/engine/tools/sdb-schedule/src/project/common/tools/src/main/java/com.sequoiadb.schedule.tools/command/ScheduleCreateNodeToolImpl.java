/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ScheduleCreateNodeToolImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.command;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.schedule.common.crypto.PasswordMgr;
import com.sequoiadb.schedule.tools.SchCtl;
import com.sequoiadb.schedule.tools.common.PropertiesUtil;
import com.sequoiadb.schedule.tools.common.ScheduleCommon;
import com.sequoiadb.schedule.tools.common.ScheduleHelpGenerator;
import com.sequoiadb.schedule.tools.common.ScheduleHelper;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.element.ScheduleNodeTypeEnum;
import com.sequoiadb.schedule.tools.element.ScheduleServerScriptEnum;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import com.sequoiadb.schedule.tools.exec.ExecutorWrapper;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.UUID;

public class ScheduleCreateNodeToolImpl extends ScheduleTool {
    private final String OPT_LONG_PORT = "port";
    private final String OPT_SHORT_PORT = "p";
    private final String OPT_LONG_META_SOURCE_URL = "ms-url";
    private final String OPT_LONG_META_SOURCE_USER = "ms-user";
    private final String OPT_LONG_META_SOURCE_PASSWD = "ms-passwd";
    private final String OPT_LONG_SYSTEM_CS_NAME = "systemCsName";
    private final String OPT_LONG_SYSTEM_CS_STORE_DOMAIN = "systemCsStoreDomain";

    private Options ops;
    private String sysConfPath;
    private String log4jConfPath;
    private boolean isNeedRollBackDir = false;
    private boolean isNeedRollBackSysConf = false;
    private boolean isNeedRollBackLog4jConf = false;
    private ScheduleHelpGenerator hp;
    private static final Logger logger = LoggerFactory
            .getLogger(ScheduleCreateNodeToolImpl.class.getName());

    public ScheduleCreateNodeToolImpl() throws ScheduleToolsException {
        super("createnode");
        ops = new Options();
        hp = new ScheduleHelpGenerator();
        ops.addOption(
                hp.createOpt(OPT_SHORT_PORT, OPT_LONG_PORT, "new node port.", true, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_META_SOURCE_URL, "metasource sdb url.", false, true, false));
        ops.addOption(
                hp.createOpt(null, OPT_LONG_META_SOURCE_USER, "metasource to sdb user.", false, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_META_SOURCE_PASSWD, "metasource to sdb passwd.", false, true,
                true, false, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_SYSTEM_CS_NAME,
                "custom system cs name in sdb.default 'SDB_SCHEDULE_SYSTEM'", false, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_SYSTEM_CS_STORE_DOMAIN,
                "custom system cs store domain name.", false, true, false));
    }

    @Override
    public void printHelp(boolean isHelpFull) {
        hp.printHelp(isHelpFull);
    }

    @Override
    public void process(String[] args) throws ScheduleToolsException {
        CommandLine cl = ScheduleCommon.parseArgs(args, ops);
        String sdbUrl = cl.getOptionValue(OPT_LONG_META_SOURCE_URL);
        String sdbUser = cl.getOptionValue(OPT_LONG_META_SOURCE_USER);
        String sdbPasswd = cl.getOptionValue(OPT_LONG_META_SOURCE_PASSWD);

        String portStr = cl.getOptionValue(OPT_LONG_PORT);
        int port = Integer.parseInt(portStr);

        if (port > 65535 || port < 0) {
            throw new ScheduleToolsException("port out of range:" + port,
                    ScheduleBaseExitCode.INVALID_ARG);
        }

        String installPath = ScheduleHelper
                .getAbsolutePathFromTool(ScheduleHelper.getPwd() + File.separator + "..");
        ExecutorWrapper exe = new ExecutorWrapper(
                new ScheduleNodeType(ScheduleNodeTypeEnum.SCHEDULESERVER,
                        ScheduleServerScriptEnum.SCHEDULESERVER),
                installPath);
        if (exe.getAllNode().containsKey(port)) {
            throw new ScheduleToolsException(
                    "The port is already occupied,port:" + port + ",conf path:"
                            + exe.getNodeConfPathCheck(port),
                    ScheduleBaseExitCode.SCHEDULE_ALREADY_EXIST_ERROR);
        }

        sysConfPath = ScheduleCommon.getScheduleConfAbsolutePath() + port + File.separator
                + ScheduleCommon.APPLICATION_PROPERTIES;
        log4jConfPath = ScheduleCommon.getScheduleConfAbsolutePath() + port + File.separator
                + ScheduleCommon.LOGCONF_NAME;
        Sequoiadb db = null;
        try {
            db = new Sequoiadb(ScheduleHelper.getSdbUrlList(sdbUrl), sdbUser, sdbPasswd,
                    new ConfigOptions());
        }
        catch (Exception e) {
            throw new ScheduleToolsException(
                    "Failed to connect to SequoiaDB, please check the sdb url,user or passwd",
                    ScheduleBaseExitCode.INVALID_ARG, e);
        }

        String customSystemCsName = cl.getOptionValue(OPT_LONG_SYSTEM_CS_NAME);
        if (customSystemCsName == null || customSystemCsName.isEmpty()) {
            customSystemCsName = "SDB_SCHEDULE_SYSTEM";
        }
        String customStoreDomain = cl.getOptionValue(OPT_LONG_SYSTEM_CS_STORE_DOMAIN);

        try {
            System.out.print("Init metasource...");
            initMetaSource(db, sdbUrl, sdbUser, sdbPasswd, customSystemCsName, customStoreDomain);
            System.out.println("success");
        }
        catch (Exception e) {
            logger.error("Failed to init meta source,error:{}", e.getMessage(), e);
            ScheduleCommon.throwToolException("Failed to init meta source", e);
        }
        finally {
            db.close();
        }

        try {
            createConfFile(sysConfPath);
            isNeedRollBackSysConf = true;
            createConfFile(log4jConfPath);
            isNeedRollBackLog4jConf = true;

            createDefaultConf(sdbUrl, sdbUser, sdbPasswd, customSystemCsName, port, sysConfPath,
                    log4jConfPath);
        }
        catch (Exception e) {
            rollBackFile();
            logger.info("Failed to create node,error:{}", e.getMessage(), e);
            ScheduleCommon.throwToolException("Failed to create node", e);
        }

        System.out.println("Create node success");
    }

    private void initMetaSource(Sequoiadb db, String sdbUrl, String sdbUser, String sdbPasswd, String customSystemCsName, String customStoreDomain) {
        if (!db.isCollectionSpaceExist(customSystemCsName)) {
            logger.info("Create system collectionSpace:{}, domain:{}", customSystemCsName, customStoreDomain);
            if (customStoreDomain != null && !customStoreDomain.isEmpty()) {
                db.createCollectionSpace(customSystemCsName,
                        new BasicBSONObject("Domain", customStoreDomain));
            }
            else {
                db.createCollectionSpace(customSystemCsName);
            }
        }

        initRootSiteInfo(db, sdbUrl, sdbUser, sdbPasswd, customSystemCsName);

        if (needInitDefaultSchedule(db, customSystemCsName)) {
            logger.info("Init default schedule info:{}", customSystemCsName);
            CollectionSpace collectionSpace = db.getCollectionSpace(customSystemCsName);
            if (!collectionSpace.isCollectionExist("SCHEDULE")) {
                collectionSpace.createCollection("SCHEDULE");
            }
            BSONObject defaultScheduleInfo = getDefaultScheduleInfo();
            collectionSpace.getCollection("SCHEDULE").insertRecord(defaultScheduleInfo);
        }
    }

    private void initRootSiteInfo(Sequoiadb db, String sdbUrl, String sdbUser, String sdbPasswd, String customSystemCsName) {
        CollectionSpace collectionSpace = db.getCollectionSpace(customSystemCsName);
        if (!collectionSpace.isCollectionExist("SITE")) {
            collectionSpace.createCollection("SITE");
        }

        DBCollection collection = collectionSpace.getCollection("SITE");
        try(DBCursor cursor = collection.query(new BasicBSONObject("name", "rootsite"), null, null, null)) {
            if (cursor.hasNext()) {
                return;
            }
        }

        BSONObject siteObj = new BasicBSONObject();
        siteObj.put("name", "rootsite");
        siteObj.put("datasource", null);
        siteObj.put("urls", ScheduleHelper.getSdbUrlList(sdbUrl));
        siteObj.put("user", sdbUser);
        String encryptedPasswd = PasswordMgr.getInstance().encrypt(1, sdbPasswd);
        siteObj.put("password", encryptedPasswd);
        logger.info("Insert root site info:{}", siteObj);
        collection.insertRecord(siteObj);
    }

    private BSONObject getDefaultScheduleInfo(){
        long timeStamp = System.currentTimeMillis();
        BSONObject scheduleInfo = new BasicBSONObject();
        scheduleInfo.put("id", UUID.randomUUID().toString());
        scheduleInfo.put("name", "系统内置清理任务");
        scheduleInfo.put("type", "clean");
        scheduleInfo.put("desc", "系统内置的清理任务，每天零点执行一次");
        scheduleInfo.put("cron", "0 0 0 * * ?");
        scheduleInfo.put("enable", true);
        BSONObject content = new BasicBSONObject();
        content.put("max_exec_time", 3600000);
        BSONObject cleanRange = new BasicBSONObject();
        cleanRange.put("max_retention_days", 30);
        cleanRange.put("clean_site", null);
        cleanRange.put("clean_site_regex", ".*");
        cleanRange.put("cl_list", new BasicBSONList());
        cleanRange.put("cs_list", new BasicBSONList());
        BasicBSONList csRegexList = new BasicBSONList();
        csRegexList.add(".*");
        cleanRange.put("cs_regex", csRegexList);
        BasicBSONList clRegexList = new BasicBSONList();
        clRegexList.add(".*");
        cleanRange.put("cl_regex", clRegexList);
        BasicBSONList cleanRangeList = new BasicBSONList();
        cleanRangeList.add(cleanRange);
        content.put("clean_range", cleanRangeList);
        scheduleInfo.put("create_time", timeStamp);
        scheduleInfo.put("update_time", timeStamp);
        scheduleInfo.put("content", content);
        return scheduleInfo;
    }

    private boolean needInitDefaultSchedule(Sequoiadb db, String customSystemCsName) {
        BSONObject clSnapshot = null;
        try (DBCursor cursor = db.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONS,
                new BasicBSONObject("Name", customSystemCsName + ".SCHEDULE"), null, null, null, 0, 1)){
            if (cursor.hasNext()) {
                clSnapshot = cursor.getNext();
            }
        }

        if (clSnapshot == null) {
            return true;
        }

        BasicBSONList details = (BasicBSONList) clSnapshot.get("Details");
        for (Object o : details) {
            BSONObject detail = (BSONObject) o;
            BasicBSONList groups = (BasicBSONList) detail.get("Group");
            for (Object groupObj : groups) {
                BSONObject group = (BSONObject) groupObj;
                Long totalInsert = (Long) group.get("TotalInsert");
                if (totalInsert > 0) {
                    return false;
                }
            }
        }
        return true;
    }

    private void rollBackFile() {
        File sysConfFile = new File(sysConfPath);
        File log4jConfFile = new File(log4jConfPath);
        if (isNeedRollBackSysConf) {
            try {
                if (sysConfFile.exists()) {
                    sysConfFile.delete();
                }
            }
            catch (Exception e1) {
                System.out.println("Failed to rollback,failed to delete file:" + sysConfPath
                        + ",errorMsg:" + e1.getMessage());
            }
        }
        if (isNeedRollBackLog4jConf) {
            try {
                if (log4jConfFile.exists()) {
                    log4jConfFile.delete();
                }
            }
            catch (Exception e2) {
                System.out.println("Failed to rollback,failed to delete file:" + log4jConfPath
                        + ",errorMsg:" + e2.getMessage());
            }
        }
        if (isNeedRollBackDir) {
            try {
                if (sysConfFile.getParentFile().exists()) {
                    sysConfFile.getParentFile().delete();
                }
            }
            catch (Exception e3) {
                System.out.println("Failed to rollback,failed to delete dir:"
                        + sysConfFile.getParentFile().getPath() + ",errorMsg:" + e3.getMessage());
            }
        }
    }

    private void createConfFile(String filePath) throws ScheduleToolsException {
        File file = new File(filePath);
        if (ScheduleCommon.isFileExists(filePath)) {
            throw new ScheduleToolsException("Failed to create " + file.getName() + ","
                    + file.getName() + " already exist:" + file.toString(),
                    ScheduleBaseExitCode.FILE_ALREADY_EXIST);
        }
        if (!ScheduleCommon.isFileExists(file.getParentFile().getPath())) {
            isNeedRollBackDir = true;
        }
        ScheduleCommon.createFile(filePath);
    }

    private void createDefaultConf(String sdbUrl, String sdbUser, String sdbPasswd,
            String systemCsName, int port, String sysConfPath, String log4jConfPath)
            throws ScheduleToolsException {

        Map<String, String> modifier = new HashMap<>();

        // sysconf.properties
        modifier.put("server.port", port + "");
        modifier.put("system.store.sequoiadb.urls", sdbUrl);
        modifier.put("system.store.sequoiadb.username", sdbUser);
        String encryptedPasswd = PasswordMgr.getInstance().encrypt(1, sdbPasswd);
        modifier.put("system.store.sequoiadb.password", encryptedPasswd);
        modifier.put("system.metasource.collectionSpace", systemCsName);

        // save
        writeDefaultConf(ScheduleCommon.SAMPLE_SYS_CONF_NAME, sysConfPath, modifier);

        // clear
        modifier.clear();

        // log4j.properties
        String logOutputPath = "." + File.separator + "log" + File.separator
                + ScheduleCommon.SCHEDULE_LOG_DIR_NAME + File.separator + port;
        modifier.put(PropertiesUtil.SAMPLE_VALUE_SCHEDULE_LOG_PATH, logOutputPath);
        // save
        writeDefaultConf(ScheduleCommon.SAMPLE_LOG_CONF_NAME, log4jConfPath, modifier);
    }

    private void writeDefaultConf(String sampleResFile, String outputPath,
            Map<String, String> modifier) throws ScheduleToolsException {
        InputStream is = SchCtl.class.getClassLoader().getResourceAsStream(sampleResFile);
        if (is == null) {
            logger.error("missing resource file:" + sampleResFile);
            throw new ScheduleToolsException("missing resource file:" + sampleResFile,
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        BufferedReader br = null;
        BufferedWriter bw = null;
        try {
            br = new BufferedReader(new InputStreamReader(is));
            bw = new BufferedWriter(new FileWriter(outputPath));
            if (sampleResFile.equals(ScheduleCommon.SAMPLE_SYS_CONF_NAME)) {
                modifyForAppConf(modifier, br, bw);
            }
            else {
                modifyForLogConf(modifier, br, bw);
            }

        }
        catch (IOException e) {
            logger.error("write config to " + outputPath + " occur error", e);
            throw new ScheduleToolsException(
                    "write config to " + outputPath + " occur error:" + e.getMessage(),
                    ScheduleBaseExitCode.SYSTEM_ERROR);
        }
        finally {
            close(bw);
            close(br);
        }
    }

    private void modifyForAppConf(Map<String, String> modifier, BufferedReader br,
            BufferedWriter bw) throws IOException {
        String line = null;
        while ((line = br.readLine()) != null) {
            Iterator<Map.Entry<String, String>> modifierIt = modifier.entrySet().iterator();
            while (modifierIt.hasNext()) {
                String modifyKey = modifierIt.next().getKey();
                if (line.contains(modifyKey)) {
                    String[] arr = line.split("=");
                    if (arr[0].trim().equals(modifyKey)) {
                        line = modifyKey + "=" + modifier.get(modifyKey);
                        modifierIt.remove();
                        break;
                    }
                }
            }
            bw.write(line);
            bw.newLine();
        }

        if (!modifier.isEmpty()) {
            bw.newLine();
            bw.write("#custom properties");
            bw.newLine();
            for (Map.Entry<String, String> prop : modifier.entrySet()) {
                bw.write(prop.getKey() + "=" + prop.getValue());
                bw.newLine();
            }
        }
    }

    private void modifyForLogConf(Map<String, String> modifier, BufferedReader br,
            BufferedWriter bw) throws IOException {
        String line = null;
        while ((line = br.readLine()) != null) {
            for (Map.Entry<String, String> entry : modifier.entrySet()) {
                if (line.contains(entry.getKey())) {
                    line = line.replace(entry.getKey(), entry.getValue());
                    break;
                }
            }
            bw.write(line);
            bw.newLine();
        }
    }

    private void close(Closeable c) {
        try {
            if (c != null) {
                c.close();
            }
        }
        catch (Exception e) {
            logger.warn("close file occur error", e);
        }
    }

}
