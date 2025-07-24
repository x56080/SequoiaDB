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

   Source File Name = TestCaseLog.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.util;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;

import org.dom4j.*;
import org.dom4j.io.*;

public class TestCaseLog {

	private String logFile;
	private Document document;
	private Element root;
	private Element currentElement;
	
	public TestCaseLog(String dir){
		String hostname;
	
		try {
			hostname = InetAddress.getLocalHost().getHostName();
		} catch (UnknownHostException e) {
			hostname = "localhost";
		}
		
		SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
		if(dir != null)
			logFile = dir + "/" + hostname + "-sql-" + formatter.format(new Date()) + ".xml";
		else
			logFile = hostname + "-sql-" + formatter.format(new Date()) + ".xml";
		
		
		initXml();
	}
	
	public void saveFile(){
		try{
			Writer fileWriter = new FileWriter(logFile);
			OutputFormat format = OutputFormat.createPrettyPrint();
			format.setEncoding("UTF-8");
			XMLWriter xmlWriter = new XMLWriter(fileWriter, format);
			xmlWriter.write(document);
			xmlWriter.close();
			fileWriter.close();
		}catch(IOException e){
			System.err.println(e.getLocalizedMessage());
			e.printStackTrace();
		}
	}
	
	public void addLogRecordError(String causeType, String sql, String message, String testcaseName){
		addTestCase("com.sequoiadb.test.SQLExec", testcaseName);
		addFailure(causeType, sql, message);
	}
	
	public void addLogRecordSucc(String sql, String message, String testcaseName){
		addTestCase("com.sequoiadb.test.SQLExec", testcaseName);
		addSucc(sql, message);
	}
	
	private void addTestCase(String classname, String name){
		currentElement = root.addElement("testcase");
		currentElement.addAttribute("classname", classname);
		currentElement.addAttribute("name", name);
		SimpleDateFormat formatter = new SimpleDateFormat("MM-dd HH:mm:ss");
		currentElement.addAttribute("time", formatter.format(new Date()));
	}
	
	private void addSucc(String sql, String message){
		currentElement.addCDATA("sql= \n" + sql + "\nmessage= \n" + message + "\n");
	}
	
	private void addFailure(String causeType, String sql, String message){
		currentElement = currentElement.addElement("failure");
		currentElement.addAttribute("type", causeType);
		currentElement.addCDATA("sql= \n" + sql + "\nmessage= \n" + message + "\n");
	}
	
	private void initXml(){
		document = DocumentHelper.createDocument();
		root = document.addElement("testsuite");
	}
}
