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

   Source File Name = ScheduleDeleteNodeToolImpl.java

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

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.schedule.common.crypto.PasswordMgr;
import com.sequoiadb.schedule.tools.common.ScheduleCommandUtil;
import com.sequoiadb.schedule.tools.common.ScheduleCommon;
import com.sequoiadb.schedule.tools.common.ScheduleHelpGenerator;
import com.sequoiadb.schedule.tools.common.ScheduleHelper;
import com.sequoiadb.schedule.tools.element.RootSiteInfo;
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

import java.io.File;
import java.net.InetAddress;

public class ScheduleDeleteNodeToolImpl extends ScheduleTool{
    private static final Logger logger = LoggerFactory.getLogger(ScheduleDeleteNodeToolImpl.class);
    private final String OPT_LONG_PORT = "port";
    private final String OPT_LONG_META_SOURCE_URL = "ms-url";
    private final String OPT_LONG_META_SOURCE_USER = "ms-user";
    private final String OPT_LONG_META_SOURCE_PASSWD = "ms-passwd";
    private final String OPT_LONG_SYSTEM_CS_NAME = "systemCsName";

    private final String OPT_SHORT_PORT = "p";

    private ScheduleHelpGenerator hp;
    private Options ops;
    private String systemCSName = "SDB_SCHEDULE_SYSTEM";
    public ScheduleDeleteNodeToolImpl() throws ScheduleToolsException {
        super("deletenode");
        hp = new ScheduleHelpGenerator();
        ops = new Options();
        ops.addOption(hp.createOpt(OPT_SHORT_PORT, OPT_LONG_PORT, "schedule node port.", true, true,
                false));
        ops.addOption(hp.createOpt(null, OPT_LONG_META_SOURCE_URL,
                "metasource to sdb url.", false, true, false));
        ops.addOption(
                hp.createOpt(null, OPT_LONG_META_SOURCE_USER, "metasource to sdb user.", false, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_META_SOURCE_PASSWD, "metasource to sdb passwd.", false, true,
                true, false, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_SYSTEM_CS_NAME,
                "custom system cs name.default 'SDB_SCHEDULE_SYSTEM'", false, true, false));
    }

    @Override
    public void process(String[] args) throws ScheduleToolsException {
        CommandLine cl = ScheduleCommandUtil.parseArgs(args, ops);

        String portStr = cl.getOptionValue(OPT_LONG_PORT);
        int port = ScheduleCommon.convertStrToInt(portStr);

        String metaSourceUrl;
        String metaSourceUser;
        String metaSourcePasswd;
        if (cl.hasOption(OPT_LONG_META_SOURCE_URL)) {
            metaSourceUrl = cl.getOptionValue(OPT_LONG_META_SOURCE_URL);
            metaSourceUser = cl.getOptionValue(OPT_LONG_META_SOURCE_USER);
            metaSourcePasswd = cl.getOptionValue(OPT_LONG_META_SOURCE_PASSWD);
            if (metaSourcePasswd == null) {
                metaSourcePasswd = readOptionValue("meta source password");
            }
        }
        else {
            String installPath = ScheduleHelper
                    .getAbsolutePathFromTool(ScheduleHelper.getPwd() + File.separator + "..");
            ExecutorWrapper exe = new ExecutorWrapper(
                    new ScheduleNodeType(ScheduleNodeTypeEnum.SCHEDULESERVER,
                            ScheduleServerScriptEnum.SCHEDULESERVER),
                    installPath);
            RootSiteInfo rootSiteSdbInfo = exe.getMetaSdbInfo();
            if (rootSiteSdbInfo == null) {
                throw new ScheduleToolsException("not found metasource sdb info in local schedule server node config",
                        ScheduleBaseExitCode.FILE_NOT_FIND);
            }
            metaSourceUrl = rootSiteSdbInfo.getSdbUrl();
            metaSourceUser = rootSiteSdbInfo.getSdbUser();
            metaSourcePasswd = PasswordMgr.getInstance().decrypt(1, rootSiteSdbInfo.getSdbPasswd());
            systemCSName = rootSiteSdbInfo.getSystemCSName();
        }

        if (cl.hasOption(OPT_LONG_SYSTEM_CS_NAME)) {
            systemCSName = cl.getOptionValue(OPT_LONG_SYSTEM_CS_NAME);
        }

        Sequoiadb db = null;
        try {
            db = new Sequoiadb(ScheduleHelper.getSdbUrlList(metaSourceUrl), metaSourceUser, metaSourcePasswd, new ConfigOptions());
            stopNode(port);
            String hostName = InetAddress.getLocalHost().getHostName();
            String hostAddress = InetAddress.getLocalHost().getHostAddress();
            deleteNodeFromMeta(db, hostName, hostAddress, port);
            clearNodeFile(port);
            System.out
                    .println("delete schedule node success:hostName=" + hostName + ", port=" + port);
            logger.info("delete schedule node success:hostName={}, port={}", hostName, port);
        }
        catch (Exception e) {
            logger.error("delete schedule node failed: port={}", port, e);
            ScheduleCommon.throwToolException("delete schedule node failed", e);
        }
    }

    private void clearNodeFile(int port) throws ScheduleToolsException {
        String confPath = ScheduleCommon.getScheduleConfAbsolutePath() + port;
        File confDir = new File(confPath);
        try {
            deleteFile(confDir);
        }
        catch (Exception e) {
            throw new ScheduleToolsException(
                    "delete node conf directory failed: confDir=" + confDir.getPath(),
                    ScheduleBaseExitCode.SYSTEM_ERROR, e);
        }

        String logPath = "." + File.separator + "log" + File.separator
                + ScheduleCommon.SCHEDULE_LOG_DIR_NAME + File.separator + port;
        File logDir = new File(logPath);
        try {
            deleteFile(logDir);
        }
        catch (Exception e) {
            throw new ScheduleToolsException(
                    "delete node log directory failed: logDir=" + logDir.getPath(),
                    ScheduleBaseExitCode.SYSTEM_ERROR, e);
        }
    }

    private void deleteFile(File dirFile) throws ScheduleToolsException {
        if (dirFile.exists()) {
            if (!dirFile.isFile()) {
                for (File subfile : dirFile.listFiles()) {
                    deleteFile(subfile);
                }
            }
            dirFile.delete();
        }
    }

    private void deleteNodeFromMeta(Sequoiadb db, String hostName, String hostAddress, int port) throws ScheduleToolsException {
        BasicBSONList andList = new BasicBSONList();
        BSONObject hostNameCond = new BasicBSONObject();
        hostNameCond.put("host_name", hostName);
        hostNameCond.put("port", port);
        BSONObject hostAddressCond = new BasicBSONObject();
        hostAddressCond.put("ip_Addr", hostAddress);
        hostAddressCond.put("port", port);
        andList.add(hostNameCond);
        andList.add(hostAddressCond);
        DBCollection collection = db.getCollectionSpace(systemCSName).getCollection("SERVER_NODE");
        collection.deleteRecords(new BasicBSONObject("$and", andList));
    }

    private void stopNode(int port) throws ScheduleToolsException {
        String installPath = ScheduleHelper.getAbsolutePathFromTool(ScheduleHelper.getPwd() + File.separator + "..");
        ExecutorWrapper executor = new ExecutorWrapper(new ScheduleNodeType(ScheduleNodeTypeEnum.SCHEDULESERVER, ScheduleServerScriptEnum.SCHEDULESERVER), installPath);
        executor.stopNode(port, true);
    }

    @Override
    public void printHelp(boolean isFullHelp) throws ScheduleToolsException {
        hp.printHelp(isFullHelp);
    }

    private static String readOptionValue(String optionName) {
        System.out.print(optionName + " value: ");
        return readPasswdFromStdIn();
    }

    public static String readPasswdFromStdIn() {
        return new String(System.console().readPassword());
    }
}
