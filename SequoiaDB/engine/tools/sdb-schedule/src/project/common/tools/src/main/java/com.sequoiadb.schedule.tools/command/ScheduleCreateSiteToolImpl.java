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

   Source File Name = ScheduleCreateSiteToolImpl.java

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBVersion;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.schedule.common.Version;
import com.sequoiadb.schedule.common.crypto.PasswordMgr;
import com.sequoiadb.schedule.tools.common.ScheduleCommon;
import com.sequoiadb.schedule.tools.common.ScheduleHelpGenerator;
import com.sequoiadb.schedule.tools.common.ScheduleHelper;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.element.ScheduleNodeTypeEnum;
import com.sequoiadb.schedule.tools.element.ScheduleServerScriptEnum;
import com.sequoiadb.schedule.tools.element.RootSiteInfo;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import com.sequoiadb.schedule.tools.exec.ExecutorWrapper;
import com.sequoiadb.util.SdbDecrypt;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.util.List;

public class ScheduleCreateSiteToolImpl extends ScheduleTool{
    private static final Logger logger = LoggerFactory.getLogger(ScheduleCreateSiteToolImpl.class);
    private final String OPT_LONG_NAME = "name";
    private final String OPT_LONG_META_SOURCE_URL = "ms-url";
    private final String OPT_LONG_META_SOURCE_USER = "ms-user";
    private final String OPT_LONG_META_SOURCE_PASSWD = "ms-passwd";
    private final String OPT_LONG_DATASOURCE_NAME = "ds-name";
    private final String OPT_LONG_SYSTEM_CS_NAME = "systemCsName";

    private final Version MIN_DS_SITE_VERSION = new Version(5,8,6);

    private Options ops;
    private ScheduleHelpGenerator hp;

    private String systemCSName = "SDB_SCHEDULE_SYSTEM";

    public ScheduleCreateSiteToolImpl() throws ScheduleToolsException {
        super("createsite");
        ops = new Options();
        hp = new ScheduleHelpGenerator();
        ops.addOption(hp.createOpt(null, OPT_LONG_NAME, "site name", true, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_META_SOURCE_URL, "metasource source url", false, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_META_SOURCE_USER, "metasource source user", false, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_META_SOURCE_PASSWD, "metasource source password", false, true, true, false, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_DATASOURCE_NAME, "data source name in rootsite sdb", true, true, false));
        ops.addOption(hp.createOpt(null, OPT_LONG_SYSTEM_CS_NAME,
                "custom system cs name.default 'SDB_SCHEDULE_SYSTEM'", false, true, false));
    }

    @Override
    public void process(String[] args) throws ScheduleToolsException {
        CommandLine cl = ScheduleCommon.parseArgs(args, ops);
        String siteName = cl.getOptionValue(OPT_LONG_NAME);
        if (siteName == null || siteName.isEmpty()) {
            throw new IllegalArgumentException("site name is empty");
        }
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

        String dataSourceName = cl.getOptionValue(OPT_LONG_DATASOURCE_NAME);
        if (dataSourceName == null || dataSourceName.isEmpty()) {
            throw new IllegalArgumentException("data source name is empty");
        }

        Sequoiadb msSdb = null;
        try {
            msSdb = getSdbConnection(ScheduleHelper.getSdbUrlList(metaSourceUrl), metaSourceUser, metaSourcePasswd);
            DBCollection collection = msSdb.getCollectionSpace(systemCSName).getCollection("SITE");
            BSONObject object = collection.queryOne(new BasicBSONObject("name", siteName), null, null, null, 0);
            if (object != null) {
                Object datasource = object.get("datasource");
                if (datasource == null || !datasource.equals(dataSourceName)) {
                    throw new ScheduleToolsException("site " + siteName + " already exists, existSiteInfo=" + object, ScheduleBaseExitCode.ALREADY_EXIST_ERROR);
                }
                System.out.println("datasource site already exists！");
                return;
            }

            BSONObject obj = collection.queryOne(new BasicBSONObject("datasource", dataSourceName), null,
                    null, null, 0);
            if (obj != null) {
                throw new ScheduleToolsException("datasource already exists, existSite=" + obj, ScheduleBaseExitCode.ALREADY_EXIST_ERROR);
            }

            String dataSourceUrl;
            String dataSourceUser;
            String cipherText;
            try(DBCursor cursor = msSdb.getList(Sequoiadb.SDB_LIST_DATASOURCES, new BasicBSONObject("Name", dataSourceName),
                    null, null, null, 0, 1)) {
                if (!cursor.hasNext()) {
                    throw new IllegalArgumentException("datasource not found in rootsite sdb datasource list, dataSourceName=" + dataSourceName);
                }
                BSONObject next = cursor.getNext();
                dataSourceUrl = (String) next.get("Address");
                dataSourceUser = (String) next.get("User");
                cipherText = (String) next.get("CipherText");
            }

            if (dataSourceUrl == null) {
                throw new IllegalArgumentException("datasource url not found in rootsite sdb datasource list, datasource=" + dataSourceName);
            }

            try (Sequoiadb dsSdb = getSdbConnection(ScheduleHelper.getSdbUrlList(dataSourceUrl), dataSourceUser, new SdbDecrypt().decryptPasswd(cipherText))){
                DBVersion dbVersion = dsSdb.getDBVersion();
                Version dsVersion = new Version(dbVersion.getVersion(), dbVersion.getSubVersion(), dbVersion.getFixVersion());
                if (dsVersion.compareTo(MIN_DS_SITE_VERSION) < 0) {
                    throw new ScheduleToolsException("data source version is too low, min version is " + MIN_DS_SITE_VERSION + ", current version is " + dsVersion,
                            ScheduleBaseExitCode.INVALID_ARG);
                }
            }

            BSONObject siteObj = new BasicBSONObject();
            siteObj.put("name", siteName);
            siteObj.put("datasource", dataSourceName);
            collection.insertRecord(siteObj);
            System.out.println("create site [" + siteName + "] success");
        }
        catch (ScheduleToolsException e) {
            throw e;
        }
        catch (Exception e) {
            throw new ScheduleToolsException("create site failed", ScheduleBaseExitCode.SYSTEM_ERROR, e);
        }
        finally {
            if (msSdb != null) {
                msSdb.close();
            }
        }
    }

    private Sequoiadb getSdbConnection(List<String> urls, String user, String passwd) {
        return new Sequoiadb(urls, user, passwd, new ConfigOptions());
    }

    private static String readOptionValue(String optionName) {
        System.out.print(optionName + " value: ");
        return readPasswdFromStdIn();
    }

    public static String readPasswdFromStdIn() {
        return new String(System.console().readPassword());
    }

    @Override
    public void printHelp(boolean isFullHelp) throws ScheduleToolsException {
        hp.printHelp(isFullHelp);
    }
}
