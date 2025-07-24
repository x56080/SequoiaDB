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

   Source File Name = ConfigMeta.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.config;

import java.io.File;

import org.dom4j.*;
import org.dom4j.io.SAXReader;

public class ConfigMeta {
	
	//test enviroment variable
	public String testDriver;
	public String testURL;
	public String testDatabase;
	public String testUid;
	public String testPwd;
	
	public String verifyDriver;
	public String verifyURL;
	public String verifyDatabase;
	public String verifyUid;
	public String verifyPwd;
	
	public boolean verifyEnable;
	
	public boolean caseRandom;
	
	//
	private Document document;
	private Element root;
	
	public ConfigMeta(String configFile){
		readConfigFile(configFile);
		initConfigMetadata();
		showParas();
	}
	
	private void readConfigFile(String configFile){
		try{
			SAXReader saxReader = new SAXReader();
			document 	= 	saxReader.read(new File(configFile));
			root		=	document.getRootElement();
		}
		catch(Exception e){
			errorAndExit(e);
		}
	}
	
	private void initConfigMetadata(){
		getTestnfo();
		getVerifyInfo();
		getCaseRandom();
	}
	
	private void errorAndExit(Exception e){
		e.printStackTrace();
		System.exit(1);
	}
	
	private void getTestnfo(){
		Element testInfo = root.element("test-source");
		try{
			testDriver = testInfo.element("driver").getTextTrim();
			testURL = testInfo.element("url").getTextTrim();
			testDatabase = testInfo.element("database").getTextTrim();
			testUid = testInfo.element("uid").getTextTrim();
			if(testUid.trim().length() == 0)
				testUid = null;
			testPwd = testInfo.element("pwd").getTextTrim();
			if(testPwd.trim().length() == 0)
				testPwd = null;
		}
		catch(Exception e){
			errorAndExit(e);
		}
	}
	
	private void getVerifyInfo(){
		Element verifyInfo = root.element("verify-source");
		try{
			if(verifyInfo.element("enable").getTextTrim().equalsIgnoreCase("yes") == true)
			{
				verifyEnable = true;
			}
			else
			{
				verifyEnable = false;
				return;
			}
			
			verifyDriver = verifyInfo.element("driver").getTextTrim();
			verifyURL = verifyInfo.element("url").getTextTrim();
			verifyDatabase = verifyInfo.element("database").getTextTrim();
			verifyUid = verifyInfo.element("uid").getTextTrim();
			if(verifyUid.trim().length() == 0)
				verifyUid = null;
			verifyPwd = verifyInfo.element("pwd").getTextTrim();
			if(verifyPwd.trim().length() == 0)
				verifyPwd = null;
		}
		catch(Exception e){
			errorAndExit(e);
		}
	}
	
	private void getCaseRandom(){
		try{
			Element caserandom = root.element("caserandom");
			if(caserandom.getTextTrim().equalsIgnoreCase("yes"))
				caseRandom = true;
			else
				caseRandom = false;
		}catch(Exception e){
			caseRandom = false;
		}
	}
	
	private void showParas(){
		System.out.println("test config:");
		System.out.println("*****************");
		System.out.println("test driver:\t " + testDriver);
		System.out.println("test url:\t " + testURL);
		System.out.println("test database:\t " + testDatabase);
		System.out.println("test uid:\t " + testUid);
		System.out.println("test pwd:\t " + testPwd);
		
		System.out.println("verify-db-enable:\t " + verifyEnable);
		
		if(verifyEnable){
			System.out.println("verify driver:\t " + verifyDriver);
			System.out.println("verify url:\t " + verifyURL);
			System.out.println("verify database:\t " + verifyDatabase);
			System.out.println("verify uid:\t " + verifyUid);
			System.out.println("verify pwd:\t " + verifyPwd);
		}
		
		System.out.println("exec testcase randomly:\t " + caseRandom);
		
		System.out.println("*****************");
		System.out.println("");
	}
}
