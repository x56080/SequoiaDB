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

import ch.qos.logback.classic.Level;
import ch.qos.logback.classic.LoggerContext;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.apache.commons.cli.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

public class Param {
    private static final Logger logger = LoggerFactory.getLogger(Param.class);
    public final static String ACTION_EXPORTMETA = "exportmeta";
    public final static String ACTION_EXPORTLOBINFO = "exportlobinfo";
    public final static String ACTION_REPAIR = "repair";
    public final static String ACTION_UPDATE = "update";
    private final int THREADNUM_DEFAULT = 10;
    private final int BATCHSIZE_DEFAULT = 10000;

    String[] args;
    private String host; // <hostname:port>
    private String userName;
    private String password;
    private String action;
    private String collectionName; // full name
    private String outputDir;
    private ArrayList<String> inputFiles = new ArrayList<>();
    private int threadNum = THREADNUM_DEFAULT;
    private int batchSize = BATCHSIZE_DEFAULT;
    private String csName;
    private String clName;

    public Param(String[] args) {
        this.args = args;
        init();
    }

    private void init() {
        LoggerContext loggerContext = (LoggerContext) LoggerFactory.getILoggerFactory();
        List<ch.qos.logback.classic.Logger> loggerList = loggerContext.getLoggerList();
        Level logLevel = Level.INFO;

        // 根据命令行参数定义Option对象，第1/2/3/4个参数分别是指命令行参数名缩写、参数名全称、是否有参数值、参数描述
        Option opt1 = Option.builder().longOpt("host").hasArg(true).required(false).desc("(可选)coord 节点地址，如：localhost:11810").build();
        Option opt2 = Option.builder("u").longOpt("user").hasArg(true).required(true).desc("(必选)用户名").build();
        Option opt3 = Option.builder("w").longOpt("password").hasArg(true).required(true).desc("(必选)用户密码").build();
        Option opt4 = Option.builder("a").longOpt("action").hasArg(true).required(true).desc("(必选)操作类型，取值为 exportmeta/exportlobinfo/repair/update，四选一").build();
        Option opt5 = Option.builder("c").longOpt("collection").hasArg(true).required(true).desc("(必选)指定 --action 参数需要处理的集合全名。仅支持单个集合").build();
        Option opt6 = Option.builder("o").longOpt("outputdir").hasArg(true).required(true).desc("(必选)指定 --action 参数执行后，结果文件存放的目录").build();
        Option opt7 = Option.builder("f").longOpt("inputfile").hasArg(true).required(false).desc("(可选)指定部分 --action 参数需要使用的输入文件。当输入多个文件时，使用逗号分割").build();
        Option opt8 = Option.builder("l").longOpt("logLevel").hasArg(true).required(false).desc("(可选)指定日志级别，可选取值为 OFF/DEBUG/INFO/WARN/ERROR，默认为 INFO").build();
        Option opt9 = Option.builder("j").longOpt("threads").hasArg(true).required(false).desc("(可选，repair 时生效)执行数据修复时，最大的并发线程数，默认值为 10").build();
        Option opt10 = Option.builder("b").longOpt("batch").hasArg(true).required(false).desc("(可选，repair 时生效)执行数据修复时，每线程每次修复的记录数，默认值为 10000").build();
        Option opt11 = Option.builder("h").longOpt("help").hasArg(false).required(false).desc("(可选)打印命令行参数说明").build();

        Options options = new Options();
        options.addOption(opt1);
        options.addOption(opt2);
        options.addOption(opt3);
        options.addOption(opt4);
        options.addOption(opt5);
        options.addOption(opt6);
        options.addOption(opt7);
        options.addOption(opt8);
        options.addOption(opt9);
        options.addOption(opt10);
        options.addOption(opt11);

        CommandLine cli = null;
        CommandLineParser cliParser = new DefaultParser();
        HelpFormatter helpFormatter = new HelpFormatter();

        try {
            cli = cliParser.parse(options, args);
        } catch (ParseException e) {
            System.err.println("Error parsing command line arguments: " + e.getMessage());
            // 解析失败是用 HelpFormatter 打印 帮助信息
            helpFormatter.printHelp("SCM 元数据修复工具(阶段二)运行参数", options);
            System.exit(1);
        }

        // 打印帮助信息
        if (cli.hasOption("h")) {
            helpFormatter.printHelp("SCM 元数据修复工具(阶段二)运行参数", options);
            System.exit(0);
        }
        // <hostname:port>
        if (cli.hasOption("host")) {
            host = cli.getOptionValue("host", "");
        }
        // user
        if (cli.hasOption("u")) {
            userName = cli.getOptionValue("u", "");
        }
        // password
        if (cli.hasOption("w")) {
            password = cli.getOptionValue("w", "");
        }
        // action
        if (cli.hasOption("a")) {
            this.action = cli.getOptionValue("a", "");
            if (!this.action.equals(ACTION_EXPORTMETA) &&
                    !this.action.equals(ACTION_EXPORTLOBINFO) &&
                    !this.action.equals(ACTION_REPAIR) &&
                    !this.action.equals(ACTION_UPDATE)) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "非法的操作参数: " + this.action);
            }
        }
        // collections
        if (cli.hasOption("c")) {
            // get and check collection full name
            this.collectionName = cli.getOptionValue("c", "");
            String[] array = this.collectionName.split("\\.");
            if (array == null || array.length != 2 || array[0].isEmpty() || array[1].isEmpty()) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "非法的集合全名: " + this.collectionName);
            }
            this.csName = array[0];
            this.clName = array[1];
        }
        // output directory
        if (cli.hasOption("o")) {
            this.outputDir = cli.getOptionValue("o", "");
            // check and the directory exist or not
            Common.checkOutputDir(this.getOutputDir());
        }
        // input file(s)
        if (cli.hasOption("f")) {
            String fileNames = cli.getOptionValue("f", "");
            String[] files = fileNames.split(",");
            for (String file : files) {
                if (file.isEmpty()) {
                    throw new BaseException(SDBError.SDB_INVALIDARG, "非法的输入文件名: " + fileNames);
                }
                if (!this.inputFiles.contains(file)) {
                    this.inputFiles.add(file);
                }
            }
        }
        // threads
        if (cli.hasOption("j")) {
            threadNum = Integer.parseInt(cli.getOptionValue("j", "10"));
            if (threadNum <= 0) {
                threadNum = THREADNUM_DEFAULT;
            } else if (threadNum > 100) {
                threadNum = 100;
            }
        }
        // batch size
        if (cli.hasOption("b")) {
            batchSize = Integer.parseInt(cli.getOptionValue("b", "10000"));
            if (batchSize <= 0) {
                batchSize = BATCHSIZE_DEFAULT;
            } else if (batchSize > 100000) {
                batchSize = 100000; // 10w
            }
        }
        // log
        if (cli.hasOption("l")) {
            String logStr = cli.getOptionValue("l", "");
            if (logStr.equals(Level.OFF.toString())) {
                logLevel = Level.OFF;
            } else if (logStr.equals(Level.DEBUG.toString())) {
                logLevel = Level.DEBUG;
            } else if (logStr.equals(Level.INFO.toString())) {
                logLevel = Level.INFO;
            } else if (logStr.equals(Level.WARN.toString())) {
                logLevel = Level.WARN;
            } else if (logStr.equals(Level.ERROR.toString())) {
                logLevel = Level.ERROR;
            } else {
                throw new BaseException(SDBError.SDB_INVALIDARG, "非法的日志级别: " + logStr);
            }
        }
        final Level level = logLevel;
        loggerList.forEach(logger -> {
            logger.setLevel(level);
        });
        // check
        if ((this.host == null || this.host.isEmpty()) && !this.action.equals(ACTION_EXPORTLOBINFO)) {
            // 当 action 为 exportlobinfo 时，地址信息通过文件输入，所以不需要 --host 参数；其它情况都需要
            throw new BaseException(SDBError.SDB_INVALIDARG, "需要指定 --host 参数");
        }
    }

    public String getHost() {
        return host;
    }

    public String getUserName() {
        return userName;
    }

    public String getPassword() {
        return password;
    }

    public String getAction() {
        return action;
    }

    public int getThreadNum() {
        return threadNum;
    }

    public int getBatchSize() {
        return batchSize;
    }

    public String getCollectionName() {
        return collectionName;
    }

    public String getCSName() {
        return csName;
    }

    public String getCLName() {
        return clName;
    }

    public String getOutputDir() {
        return outputDir;
    }

    public ArrayList<String> getInputFiles() {
        return inputFiles;
    }
}
