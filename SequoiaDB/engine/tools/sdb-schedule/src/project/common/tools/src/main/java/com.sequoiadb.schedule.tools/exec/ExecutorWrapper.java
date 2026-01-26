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

   Source File Name = ExecutorWrapper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.tools.exec;
import com.sequoiadb.schedule.tools.common.PropertiesUtil;
import com.sequoiadb.schedule.tools.common.ScheduleCommon;
import com.sequoiadb.schedule.tools.common.ScheduleHelper;
import com.sequoiadb.schedule.tools.common.ScheduleToolsDefine;
import com.sequoiadb.schedule.tools.element.ScheduleNodeInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeProcessInfo;
import com.sequoiadb.schedule.tools.element.ScheduleNodeStatus;
import com.sequoiadb.schedule.tools.element.ScheduleNodeType;
import com.sequoiadb.schedule.tools.element.RootSiteInfo;
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

public class ExecutorWrapper {
    private String servicePath;
    private Executor executor;
    private Map<Integer, ScheduleNodeInfo> node2Conf;
    private static final Logger logger = LoggerFactory.getLogger(ExecutorWrapper.class);
    private ScheduleNodeType nodeType;

    public ExecutorWrapper(ScheduleNodeType nodeType, String installPath) throws ScheduleToolsException {
        this.nodeType = nodeType;
        this.servicePath = installPath + File.separator + nodeType.getServiceDirName();
        if (ScheduleCommon.isLinux()) {
            executor = new LinuxExecutorImpl();
        }
        else {
            throw new ScheduleToolsException("Unsupport platform", ScheduleBaseExitCode.SYSTEM_ERROR);
        }
    }

    public String getNodeConfPathCheck(int port) throws ScheduleToolsException {
        String confOfPort = getNodeConfPath(port);
        if (confOfPort == null) {
            throw new ScheduleToolsException("Can't find conf path of " + port + " node",
                    ScheduleBaseExitCode.FILE_NOT_FIND);
        }
        return confOfPort;
    }

    public String getNodeConfPath(int port) throws ScheduleToolsException {
        scanConfDir();
        String confOfPort = node2Conf.get(port).getConfPath();
        return confOfPort;
    }

    public void startNode(ScheduleNodeInfo node) throws ScheduleToolsException {
        String nodeConfPath = node.getConfPath();
        try {
            String springConfigLocation = nodeConfPath + File.separator
                    + ScheduleToolsDefine.FILE_NAME.APP_PROPS;
            String loggingConfig = nodeConfPath + File.separator + ScheduleToolsDefine.FILE_NAME.LOGBACK;

            String jarPath = ScheduleHelper.getJarPathByType(node.getNodeType(),
                    servicePath);
            String logPath = servicePath
                    + File.separator + ScheduleToolsDefine.FILE_NAME.LOG
                    + File.separator + node.getNodeType().getName() + File.separator
                    + node.getPort();
            String errorLogPath = logPath + File.separator + ScheduleToolsDefine.FILE_NAME.ERROR_OUT;
            ScheduleHelper.createDir(logPath);

            Properties sysPro = PropertiesUtil.loadProperties(springConfigLocation);
            String options = sysPro.getProperty(ScheduleToolsDefine.SYSTEM_JVM_OPTIONS, "");

            executor.startNode(jarPath, springConfigLocation, loggingConfig, errorLogPath, options,
                    servicePath);
        }
        catch (ScheduleToolsException e) {
            throw new ScheduleToolsException(
                    "Failed:sdb-schedule(" + node.getPort() + ") failed to start:" + e.getMessage(),
                    e.getExitCode());
        }
    }

    public RootSiteInfo getMetaSdbInfo() {
        scanConfDir();
        Collection<ScheduleNodeInfo> localNodes = node2Conf.values();
        for (ScheduleNodeInfo node : localNodes) {
            String confPath = node.getConfPath() + File.separator + "application.properties";
            try {
                Properties prop = PropertiesUtil.loadProperties(confPath);
                String sdb = prop.getProperty("system.store.sequoiadb.urls");
                if (sdb != null && sdb.length() != 0) {
                    String sdbUser = prop.getProperty("system.store.sequoiadb.username", "");
                    String sdbPasswd = prop.getProperty("system.store.sequoiadb.password", "");
                    String systemCSName = prop.getProperty("system.metasource.collectionSpace", "SDB_SCHEDULE_SYSTEM");
                    return new RootSiteInfo(sdb, sdbUser, sdbPasswd, systemCSName);
                }
                logger.warn("server's conf file have no root site url:" + confPath);
                continue;
            }
            catch (ScheduleToolsException e) {
                logger.warn("failed to analyze server's conf file:" + confPath,
                        e);
                continue;
            }
        }
        return null;
    }

    public void stopNode(int port, boolean isForce) throws ScheduleToolsException {
        int pid = getNodePid(port);
        if (pid < 0) {
            return;
        }
        try {
            executor.stopNode(pid, isForce);
        }
        catch (ScheduleToolsException e) {
            throw new ScheduleToolsException("Failed to stop " + port + " node:" + e.getMessage(),
                    e.getExitCode());
        }
    }

    public int getNodePid(int port) throws ScheduleToolsException {
        String confOfPort = getNodeCheck(port).getConfPath();
        Map<String, ScheduleNodeProcessInfo> confNodeProcess;
        try {
            ScheduleNodeStatus psRes = executor.getNodeStatus(nodeType);
            confNodeProcess = psRes.getStatusMap();
        }
        catch (ScheduleToolsException e) {
            throw new ScheduleToolsException(
                    "Failed to check " + port + " node status:" + e.getMessage(), e.getExitCode());
        }
        if (confNodeProcess.containsKey(confOfPort)) {
            return confNodeProcess.get(confOfPort).getPid();
        }
        else {
            return -1;
        }
    }

    public Map<String, ScheduleNodeProcessInfo> getNodeStatus() throws ScheduleToolsException {
        ScheduleNodeStatus psRes = executor.getNodeStatus(nodeType);
        return psRes.getStatusMap();
    }

    public ScheduleNodeInfo getNodeCheck(int port) throws ScheduleToolsException {
        ScheduleNodeInfo node = getNode(port);
        if (node == null) {
            throw new ScheduleToolsException("Can't find conf path of " + port + " node",
                    ScheduleBaseExitCode.FILE_NOT_FIND);
        }
        return node;
    }

    public ScheduleNodeInfo getNode(int port) throws ScheduleToolsException {
        scanConfDir();
        ScheduleNodeInfo node = node2Conf.get(port);
        return node;
    }

    public Map<Integer, ScheduleNodeInfo> getAllNode() {
        scanConfDir();
        return node2Conf;
    }

    private void scanConfDir() {

        if (node2Conf != null) {
            return;
        }
        node2Conf = new HashMap<>();

        String confPath = servicePath + File.separator
                + ScheduleToolsDefine.FILE_NAME.CONF + File.separator + nodeType.getName();
        scanNode(confPath);

    }

    private void scanNode(String confPath) {
        File confFile = new File(confPath);
        if (!confFile.isDirectory()) {
            return;
        }

        File[] files = confFile.listFiles();
        for (File f : files) {
            if (f.isDirectory() && !f.getName().equals("samples")) {
                String applicationProp = confPath + File.separator + f.getName() + File.separator
                        + ScheduleToolsDefine.FILE_NAME.APP_PROPS;
                String confPort;
                try {
                    Properties prop = PropertiesUtil.loadProperties(applicationProp);
                    confPort = prop.getProperty(ScheduleToolsDefine.PROPERTIES.SERVER_PORT);
                }
                catch (ScheduleToolsException e1) {
                    logger.warn("scan conf dir have some incomplete node's conf file", e1);
                    continue;
                }
                if (confPort != null && !confPort.equals("")) {
                    try {
                        int port = Integer.valueOf(confPort);

                        this.node2Conf.put(port, new ScheduleNodeInfo(
                                confPath + File.separator + f.getName(), nodeType, port));
                    }
                    catch (Exception e) {
                        logger.error(
                                "error occurs when parsing 'server.port' in config file, path={}, server.port='{}'.",
                                applicationProp, confPort, e);
                    }
                }
                else {
                    logger.warn(
                            "scan conf dir have some incomplete node's conf file:Can't find server.port in conf file:"
                                    + applicationProp);
                }
            }
        }
    }
}
