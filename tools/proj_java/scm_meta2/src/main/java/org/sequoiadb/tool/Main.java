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

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import sun.misc.Signal;
import sun.misc.SignalHandler;

import java.util.ArrayList;
import java.util.List;

/**
 * Tool2 for fixing meta data of Scm
 */
public class Main {
    private static final Logger logger = LoggerFactory.getLogger(Main.class);

    private static SequoiadbDatasource createDatasource(Param param) {
        // create connection pool
        List<String> addrList = new ArrayList<>();
        addrList.add(param.getHost());
        // datasource option
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount(100);
        dsOpt.setMaxIdleCount(20);
        dsOpt.setMinIdleCount(10);
        dsOpt.setValidateConnection(true);
        // network option
        ConfigOptions nwOpt = new ConfigOptions();
        nwOpt.setConnectTimeout(200);
        nwOpt.setMaxAutoConnectRetryTime(0);
        SequoiadbDatasource datasource = new SequoiadbDatasource(addrList,
                param.getUserName(), param.getPassword(), nwOpt, dsOpt);
        return datasource;
    }

    public static void main(String[] args) {
        SequoiadbDatasource datasource = null;

        // handle SIGINT/SIGTERM/SIGHUP/SIGQUIT signal events
        SignalHandler signalHandler = new SignalHandler() {
            @Override
            public void handle(Signal signal) {
                logger.info("Receive signal: " + signal.getName() + ", going to stop");
                Controller.stop();
            }
        };
        Signal.handle(new Signal("TERM"), signalHandler);
        Signal.handle(new Signal("INT"), signalHandler);
        String osName = System.getProperty("os.name").toLowerCase();
        if (osName.contains("linux")) {
            Signal.handle(new Signal("HUP"), signalHandler);
            // Not support "QUIT" in linux
        }

        // parse parameters
        Param param = new Param(args);

        // run program
        try {
            TaskBase task = null;
            if (param.getAction().equals(Param.ACTION_EXPORTMETA)) {
                datasource = createDatasource(param);
                Common common = new Common(datasource);
                task = new ToolExportSCMMetaTask(param, common);
            } else if (param.getAction().equals(Param.ACTION_EXPORTLOBINFO)) {
                task = new ToolExportSCMLobInfoTask(param);
            } else if (param.getAction().equals(Param.ACTION_REPAIR)) {
                datasource = createDatasource(param);
                Common common = new Common(datasource);
                task = new ToolCheckAndRebuildMetaTask(param, common);
            } else if (param.getAction().equals(Param.ACTION_UPDATE)) {
                datasource = createDatasource(param);
                Common common = new Common(datasource);
                task = new ToolUpdateMetaTask(param, common);
            }
            task.init();
            task.doit();
        } catch (Exception e) {
            String errMsg = "Failed to run action: " + param.getAction();
            logger.error(errMsg + ", e: " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        } finally {
            try {
                if (datasource != null) {
                    datasource.close();
                }
            } catch (Exception e) {
                logger.error("Failed to close datasource");
            }
        }

        if (Controller.hasErr()) {
            System.out.println("Finish running, error happen, see the log file for more detail!");
            System.exit(-1);
        } else {
            System.out.println("Finish running!");
            System.exit(0);
        }

    }
}
