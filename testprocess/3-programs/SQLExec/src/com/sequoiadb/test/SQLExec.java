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

   Source File Name = SQLExec.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test;

import com.sequoiadb.testsql.*;
import com.sequoiadb.config.*;
import com.sequoiadb.util.*;

import java.sql.Connection;
import java.util.ArrayList;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import org.apache.log4j.*;

public class SQLExec {

	/**
	 * @param args
	 * arg1: test case file or dir
	 * arg2: filename of test config file. if not exists, then use test.xml
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		
		if(args.length < 1)
		{
			SQLExec.helpInfo();
			System.exit(1);
		}
		
		String configFile = "test.xml";
		if(args.length > 1)
			configFile = args[1];
		
		String logDir = null;
		if(args.length > 2)
			logDir = args[2];
		
		String log4jConfig = "log4j.properties";
		if(args.length > 3)
			log4jConfig = args[3];
		
		String start = "1";
		if(args.length > 4)
			start = args[4];
		
		SQLExec test = new SQLExec(args[0], configFile, logDir, log4jConfig, start);
		test.work();
		
		System.exit(0);
	}
	
	private ConfigMeta testMeta;
	private Connection testConn;
	private Connection verifyConn;
	private String logDir;
	private String startFile;
	private int startSqlNumber;
	private boolean testFilter;
	
	//logger
	private Logger logger;
	private ArrayList<TestSqlExec> testSqlExecs;
	
	//testcase log
	private TestCaseLog testLog;
	
	//testcase file
	private FileList flist;
	
	public SQLExec(String testcase, String configFile, String logDir, String log4jConfig, String start){
		initLogger(log4jConfig);
		testMeta = new ConfigMeta(configFile);
		try{
			flist = new FileList(testcase, testMeta.caseRandom);
		}
		catch(IOException e){
			e.printStackTrace();
			System.exit(2);
		}
		setStart(start);
		this.logDir = logDir;
		testLog = new TestCaseLog(this.logDir);
		initEnv();
	}

	public static void helpInfo(){
		System.out.println("SQLExecTest help:");
		System.out.println("SQLExecTest testcase [test.xml] [logdir] [log4j.properties] [start-sql-number]");
		System.out.println("**************************");
		System.out.println("\ttestcase: 	sqlfile or sqlfile dir");
		System.out.println("\t[test.xml]:	test configuration file. default is test.xml");
		System.out.println("\t[logdir]:		the dir which xml-log is generated at. default is current path");
		System.out.println("\t[log4j.properties]:		log4j config file. default is log4j.properties");
		System.out.println("\t[start-sql-number]:		the number of sql start to be executed. SqlStatements before the sql will not be execute.default value is 1.");
	}
	
	public void work(){
		
		File file;
		
		while((file=flist.getNextFile()) != null)
		{
			if(testFilter && startFile != null)
			{
				if(!file.getName().equalsIgnoreCase(startFile)){	// the files before the startFile will be not executed 
					continue;
				}
			}
			workLow(file);
			testFilter = false;
		}
		
		testEnd();
	}
	
	private void workLow(File file){
		int i = 0;
		SqlFileAnalyse fileAnalyse = new SqlFileAnalyse();
		
		fileAnalyse.setFile(file);
		fileAnalyse.startRead();
		String sql;
		
		int sqlLine = 0;
		String lineString = "";


		while((sql = fileAnalyse.getNextSql()) != null)
		{
			sqlLine++;
			if(testFilter && sqlLine < startSqlNumber)	//sqls before the startSqlNumber sql will not be executed;
				continue;
			
			lineString = "sqlLine:" + sqlLine;
			
			ArrayList<Thread> testThreads = new ArrayList<Thread>();
			for(i=0; i<testSqlExecs.size(); i++){
				testThreads.add(new Thread(testSqlExecs.get(i)));
			}
			
			testSqlExecs.get(0).setExpectedFlag(fileAnalyse.getExpectedFlag());
			
			//exec sql
			for(i=0; i<testSqlExecs.size(); i++){
				if(i==0)
					testSqlExecs.get(i).preExecSql(sql, true);		//SequoiaSql supports object
				else
					testSqlExecs.get(i).preExecSql(sql.replace("[", "_").replace("]",""), true);	//other databases don't support object
				testThreads.get(i).start();
			}
			
			//wait
			for(i=0; i<testThreads.size(); i++){
				if(testThreads.get(i).isAlive())
				{
					i--;
					try{
						Thread.sleep(10);
					}
					catch(Exception e){
						e.printStackTrace();
					}
				}
			}
			
			//check result
			for(i=0; i<testSqlExecs.size(); i++){
				TestSqlExec tmp = testSqlExecs.get(i);
				if(tmp.getExecFlag())
				{
					//succ log
					logger.info(sql + "\n--exec succ!\n");
				}
				else
				{
					//fail log,
					logger.debug(sql);
					logger.debug("\n" + "--exec failed!\n--ErrorCode:" + tmp.getErrorCode() + "\n--ErrorMessage:\n" + tmp.getSqlExceptionMessage() + "\n");
					if(isSocketError(tmp.getSqlExceptionMessage()))
					{
						testLog.addLogRecordError("SocketError", 
												sql, 
												"--ErrorCode:" + tmp.getErrorCode() + 
												"\n--ErrorMessage:\n" + tmp.getSqlExceptionMessage() + "\n",
												file.getName()+ "." + sqlLine);
						logger.error(lineString + "\n" + sql + "\n--SocketError\nErrorMessage:" + tmp.getSqlExceptionMessage());
						
						testEnd();
						
						saveTeminateNumber(file.getName(), sqlLine);
						
						System.exit(2);
					}
				}
			}
			
			//compare result
			//conditions:
			//1.exec sql on two databases
			//2.sql exec succ
			if(testSqlExecs.size() == 2 && testSqlExecs.get(0).getExecFlag() && testSqlExecs.get(1).getExecFlag() )
			{
				compreResult(testSqlExecs.get(0), testSqlExecs.get(1), sql, file.getName()+ "." + sqlLine);
			}
			//only sequoiasql exec
			else {
				noCompareResultLog(testSqlExecs.get(0), sql, file.getName()+ "." + sqlLine);
			}
			
			//close Statement
			for(i=0; i<testSqlExecs.size(); i++){
				testSqlExecs.get(i).closedStmt();
			}
			
			//if connection is closed, reconnect
			reConn(file.getName(), sqlLine);
		}
		fileAnalyse.completeRead();
	}
	
	private void compreResult(TestSqlExec test1, TestSqlExec test2, String sql, String testcaseName){
		if(test1.getRowCount() != test2.getRowCount())
		{
			logger.error(sql + "\n--row count not match!" 
							+ "\tsequoiadb:" + test1.getRowCount() 
							+ "\tthirdpart:" + test2.getRowCount() + "\n");
			testLog.addLogRecordError("RowCountNotMatch", 
									sql, 
									"\tsequoiadb:" + test1.getRowCount() + 
									"\tthirdpart:" + test2.getRowCount(),
									testcaseName);
		}
		else if(test1.getRowCount() != 0 )
		{
			//compare result low
			ResultSetCompare comp = new ResultSetCompare(test1.getResultSet(), 
														test2.getResultSet(), 
														test2.getResultSetMetaData(),
														isSqlOrderBy(sql));
			
			if(!comp.compareWork())
			{
				logger.error(sql + "\n--result compare-fail!\n" + comp.getMessage());
				testLog.addLogRecordError("ResultsetCompareFail", 
						sql, 
						comp.getMessage(),
						testcaseName);
			}
			else
			{
				logger.info("--result compare-succ!\n");
				testLog.addLogRecordSucc(sql, "--result compare-succ!", testcaseName);
			}
		}
		else
		{
			logger.info("--empty-result, compare-succ!\n");
			testLog.addLogRecordSucc(sql, "--empty-result, result compare-succ!", testcaseName);
		}
	}
	
	private void noCompareResultLog(TestSqlExec test1, String sql, String testcaseName){
		switch(test1.getExpectedFlag()){
			case IGNORE:
				testLog.addLogRecordSucc(sql, "--expected ignore and exec " + (test1.getExecFlag()?"succ":"fail"), testcaseName);
				break;
				
			case FAIL:
				if(test1.getExecFlag()){
					testLog.addLogRecordError("ExecNotEqualExpected", sql, "--expected fail but exec succ", testcaseName);
				}
				else {
					testLog.addLogRecordSucc(sql, "--expected fail and exec fail", testcaseName);
				}
				break;
				
			case SUCCESS:
				if(test1.getExecFlag()){
					testLog.addLogRecordSucc(sql, "--expected succ and exec succ", testcaseName);
				}
				else {
					testLog.addLogRecordError("ExecNotEqualExpected", sql, 
												"--expected succ but exec fail!\nErrorCode:" + test1.getErrorCode() + "\nExceptionMessage:" + test1.getSqlExceptionMessage(), 
												testcaseName);
				}
				break;
				
			default:
				testLog.addLogRecordError("UnknownExpected", sql, "--unknown expected result", testcaseName);
				break;
		}
	}
	
	private void initEnv(){
		try{
			Class.forName(testMeta.testDriver);
		}catch(ClassNotFoundException e){
			e.printStackTrace();
		}
		testConn = OperateConnection.initConnection(testMeta.testURL, testMeta.testUid, testMeta.testPwd);
		if(testConn == null){
			System.out.println("connect to test-source failed");
			System.exit(1);
		}
		
		if(testMeta.verifyEnable){
			try{
				if(testMeta.testDriver.compareTo(testMeta.verifyDriver) != 0)
					Class.forName(testMeta.verifyDriver);
			}catch(ClassNotFoundException e){
				e.printStackTrace();
			}
			verifyConn = OperateConnection.initConnection(testMeta.verifyURL, testMeta.verifyUid, testMeta.verifyPwd);
			if(verifyConn == null){
				System.out.println("connect to verify-source failed");
				System.exit(1);
			}
		}
		else
		{
			verifyConn = null;
		}
		
		testSqlExecs = new ArrayList<TestSqlExec>();
		testSqlExecs.add(new TestSqlExec(testConn, testMeta.testDatabase));
		
		if(testMeta.verifyEnable)
		{
			testSqlExecs.add(new TestSqlExec(verifyConn, testMeta.verifyDatabase));
		}
	}
	
	private void initLogger(String log4jConfig){
		logger = Logger.getLogger(SQLExec.class.getName());
		PropertyConfigurator.configure(log4jConfig);
	}
	
	private boolean isSocketError(String exceptionMessage){
		if(exceptionMessage!=null && exceptionMessage.indexOf("socket write error") > -1)
			return true;
		else if(exceptionMessage!=null && exceptionMessage.indexOf("Broken pipe") > -1)
			return true;
		else
			return false;
	}
	
	private boolean isSqlOrderBy(String sql){
		int i = sql.lastIndexOf(")");
		
		if(sql.toUpperCase().lastIndexOf(" ORDER BY ", i!=-1?i:0) >-1)
			return true;
		
		return false;
	}
	
	private void testEnd(){
		testLog.saveFile();
	}
	
	private void saveTeminateNumber(String fileName, int sqlNumber){
		try{
			String temiFileName = "teminate-sql-num.txt";
			if(logDir != null)
				temiFileName = logDir + "/" + temiFileName;
			File file = new File(temiFileName);
			FileWriter fw = new FileWriter(file);
			fw.write("errornumber=" + fileName + ":" + String.valueOf(sqlNumber));
			fw.close();
		}catch(Exception e){
			e.printStackTrace();
		}
	}
	
	private void setStart(String start){
		if(start == null || start.trim().length() == 0){
			testFilter = false;
			return;
		}
		
		String words[] = start.split(":");
		if(words.length == 1){		//just contains line-number, eg. 1000
			testFilter = true;
			startFile = null;
			try{
				startSqlNumber = Integer.valueOf(words[0]);
			}catch(Exception e){
				startSqlNumber = 1;
			}
			return;
		}
		else if(words.length == 2){	//contains sqlfile and line-number, eg. sqlfile1.sql:1000  
			testFilter = true;
			startFile = words[0];
			try{
				startSqlNumber = Integer.valueOf(words[1]);
			}catch(Exception e){
				startSqlNumber = 1;
			}
			return;
		}
		else{						//invalid input
			testFilter = false;
			System.err.println("start filter is invalid:" + start + "can't be analyzed.The correct format is sqlfile:line-number");
		}
	}
	
	private void reConn(String fileName, int sqlNumber){
		try{
			if(testConn == null || testConn.isClosed()){
				testConn = OperateConnection.initConnection(testMeta.testURL, testMeta.testUid, testMeta.testPwd);
				if(testConn == null){
					testEnd();
					saveTeminateNumber(fileName, sqlNumber);
					System.out.println("connect to test-source failed");
					System.exit(2);
				}
			}
			if(testMeta.verifyEnable){
				verifyConn = OperateConnection.initConnection(testMeta.verifyURL, testMeta.verifyUid, testMeta.verifyPwd);
				if(verifyConn == null){
					System.out.println("connect to verify-source failed");
					testMeta.verifyEnable = false;
					testSqlExecs.remove(1);
				}
			}
		}catch(Exception e){
			testEnd();
			saveTeminateNumber(fileName, sqlNumber);
			e.printStackTrace();
			System.exit(2);
		}
	}
}
