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

   Source File Name = XmlConfigReader.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.xml2web;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

import org.dom4j.Document;  
import org.dom4j.DocumentException;  
import org.dom4j.Element;  
import org.dom4j.io.OutputFormat;
import org.dom4j.io.SAXReader;
import org.dom4j.io.XMLWriter;
import org.w3c.dom.Node;
import org.xml.sax.InputSource;

public class XmlConfigReader {
	private static String confFile = "../../../doc/toc.xml";
//	private static XmlConfigReader instance = new XmlConfigReader(confFile);  
    private XmlConfig xmlconfig = new XmlConfig();
	private static String rootDir = new File(confFile).getParent();
	private static String tocFile = rootDir + "/toc.xml";
	private static String tocFile_bak = rootDir + "/toc_bak.xml";
    /**
     * initial configuration file
     * @param path
     */
    XmlConfigReader(String path,String language) {    
        try {
        	SAXReader toc_reader = new SAXReader();
        	Document toc_doc = toc_reader.read(new File(tocFile));
        	Element tocRootElement = toc_doc.getRootElement();
        	Boolean valid2 = changeToc(tocRootElement,language,toc_doc);
        	// conversion str .html to .xml
            convHtml();
        	// initialize reader
        	SAXReader reader = new SAXReader();
        	// create document from file path
        	Document doc = reader.read(new File(path));
        	
        	// get the root element from document
        	Element rootElement = doc.getRootElement();
        	
        	// change document element
        	Boolean valid = changeDoc(rootElement,language,doc);
        	
        	// process the document
        	runProcessDoc(rootElement,language,doc);
        	// if the document is not valid, we need to show error message
//        	if(valid == false){ 
//        		System.err.println("ERROR : The document is not valid.");
//        	}
        	// delete objlist tag
        	delObjlist();
        } catch (DocumentException e) {  
            e.printStackTrace();  
        }       
    }
    
    private Boolean changeToc(Element element, String language, Document doc) {
		Boolean valid = true;
		// make sure the document name is "doc"
        if(!element.getName().equals("doc")){
        	System.err.println("ERROR : The element : " + element.getName() + "is not root element.");
        	return false;
        }
        // get version information
        String major_release = element.attributeValue("major_release") ;
        String minor_release = element.attributeValue("minor_release") ;
        // release information sanity check
        if ( null == major_release || major_release.compareTo("") == 0 ||
        	 null == minor_release || minor_release.compareTo("") == 0 ) {
        	System.err.println("ERROR : Invalid release information" ) ;
        	return false ;
        }
        // get objlist from the root element
        Element docElement = element.element("objlist");

        // make sure the element exist
        if(null == docElement){
        	System.err.println ( "ERROR : objlist does not exist in doc") ;
        	return false ;
        }

		try {
//	    	@SuppressWarnings("unchecked")
//	        List<Element> rootObjElements = docElement.elements("obj");
//	    	for(Element rootObjElement:rootObjElements){
//	    		rootObjElement.setName("chapter");
//			}
	    	valid = changeObjlist(docElement,1,language);
			OutputFormat format = OutputFormat.createPrettyPrint();
	    	format.setEncoding("UTF-8");
	    	XMLWriter write = new XMLWriter(new FileWriter(tocFile),format);
			write.write(doc);
			write.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
        // 1 is always the root pid
        return valid;
	}

	private Boolean changeDoc(Element element, String language, Document doc) {
		Boolean valid = true;
		// make sure the document name is "doc"
        if(!element.getName().equals("doc")){
        	System.err.println("ERROR : The element : " + element.getName() + "is not root element.");
        	return false;
        }
        // get version information
        String major_release = element.attributeValue("major_release") ;
        String minor_release = element.attributeValue("minor_release") ;
        // release information sanity check
        if ( null == major_release || major_release.compareTo("") == 0 ||
        	 null == minor_release || minor_release.compareTo("") == 0 ) {
        	System.err.println("ERROR : Invalid release information" ) ;
        	return false ;
        }
        // get objlist from the root element
        Element docElement = element.element("objlist");

        // make sure the element exist
        if(null == docElement){
        	System.err.println ( "ERROR : objlist does not exist in doc") ;
        	return false ;
        }

		try {
	    	@SuppressWarnings("unchecked")
	        List<Element> rootObjElements = docElement.elements("obj");
	    	for(Element rootObjElement:rootObjElements){
	    		rootObjElement.setName("chapter");
			}
	    	valid = changeObjlist(docElement,1,language);
			OutputFormat format = OutputFormat.createPrettyPrint();
	    	format.setEncoding("UTF-8");
	    	XMLWriter write = new XMLWriter(new FileWriter(confFile),format);
			write.write(doc);
			write.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
        // 1 is always the root pid
//		System.out.println("########################valid : " + valid + "########################");
        return valid;
	}

	private Boolean changeObjlist(Element element,int pid,String language) {
		boolean valid = false;
    	@SuppressWarnings("unchecked")
		List<Element> objs = element.elements("obj");
    	int order = 0;
    	for(Element obj:objs){
    		String prefix = "ch" + (pid < 10 ? "0" : "") + pid;
    		boolean validObj = changeObj(obj,prefix,order+1,language); 
//    		System.out.println("^^^^^^^^^^^^^^^^^^^^^^^validObj : " + validObj + "^^^^^^^^^^^^^^^^^^^^^^^2");
    		
    		valid = Boolean.valueOf(validObj || false);
//    		System.out.println("^^^^^^^^^^^^^^^^^^^^^^^valid : " + valid + "|" + obj.element("cnname").getTextTrim() + "^^^^^^^^^^^^^^^^^^^^^^^2");
    		pid++;
		}
//    	System.out.println("@@@@@@@@@@@@@@@@@@@@valid : " + valid + "@@@@@@@@@@@@@@@@@@@@@@@@@@2");
    	return valid;
	}
	
	public Boolean changeObjlist2(Element element,String pid,int level_order,String language){
    	boolean valid = false;	
    	@SuppressWarnings("unchecked")
		List<Element> objs = element.elements("obj");
    	int order = 0;
    	
    	for(Element obj:objs){
    		String temp_pid = "";
    		String suffix = "s" + (level_order < 10 ? "0" : "") + level_order;
    		temp_pid = pid + suffix;
    		boolean validObj = changeObj(obj,temp_pid,order+1,language); 
    		valid = Boolean.valueOf(validObj || false);
    		level_order++;
		}
//    	System.out.println("$$$$$$$$$$$$$$$$$$$$$$$$valid : " + valid + "$$$$$$$$$$$$$$$$$$$$$$$$$");
    	return valid;
    }
	
    private boolean changeObj(Element element, String pid, int order, String language) {
//    	SDBDocument doc = new SDBDocument () ;
    	boolean valid = true;
    	// initialize from element
		Integer id = Integer.parseInt(getString(element,"id"));
		String cnname = getString(element,"cnname");
		String enname = getString(element,"enname");
		String cnpath = getString(element,"cnpath");
		String enpath = getString(element,"enpath");
		String enabled = getString(element,"enabled");
		// if the docuemnt contains objlist, that means the current element is a directory
		Element objlist = element.element("objlist") ;
		boolean isDirectory = objlist != null ;
		// initial enabled element 
		if ( enabled.compareToIgnoreCase("false") == 0 ||
			 enabled.compareToIgnoreCase("f") == 0 ||
			 enabled.compareToIgnoreCase("0") == 0 ) {
			valid = Boolean.valueOf(false);
		}

		// checkObj is able to check validity of the element
		// 1) make sure the id does not exist in xmlconfig
		// 2) make sure either cnname or enname exist or both )
		// 3) make sure cnname exist if cnpath appears
		// 4) make sure enname exist if enpath appears
		// 5) make sure cnpath and enpath does not exist if isDirectory = true
		// 6) make sure cnpath and enpath files are valid
		if ( false == checkObj(id, cnname, enname, cnpath, enpath, isDirectory )) {
			System.err.print("ERROR : Failed to check object : " + enname + ".\n" );
			valid = Boolean.valueOf(false);
		}
		// process subobjlist if exist
//		System.out.println("*********************************************************************************************");
//		System.out.println("cnname : " + cnname);
//		System.out.println("pid : " + pid);
		
		if ( isDirectory && ( false == changeObjlist2( objlist, pid, order, language ) ) ) {
			System.err.print ( "ERROR : Failed to process obj list.\n") ;
			valid = Boolean.valueOf(false);
		}

		element.element("enabled").setText(pid);
//		System.out.println("enabled : " + element.element("enabled").getTextTrim());
//		System.out.println("*********************************************************************************************\n");
//		System.out.println("******************************valid : " + valid + "****************************");
		return valid ;
	}

	/**
     * This function is used to process a document element
     * @param element
     * @param language
     * @param doc
     * @return boolean
     */
    public boolean runProcessDoc(Element element,String language,Document doc){
//		System.out.println("##############################################################################################");
//    	System.out.println("doc : " + doc.asXML());
//		System.out.println("##############################################################################################\n");
    	// get objlist from the root element
        Element docElement = element.element("objlist");
        // delete statement tag
        if (!convState()) {
        	System.err.println("ERROR : The statement label deleting file failed.\n");
        	return false;
        }
        // conversion root tag
        if (!convDoc()) {
        	System.err.println("ERROR : The root label converting file failed.\n");
        	return false;
        }
		// conversion obj tag to section
		if (!convObj()) {
        	System.err.println("ERROR : The obj label conversing file failed.\n");
        	return false;
        }
        // conversion id tag to info
		if (!convId()) {
        	System.err.println("ERROR : The Id label conversing file failed.\n");
        	return false;
        }
		// conversion cnname tag to title and deleting enname tag or the opposite situation
		if (language.compareTo("Cn") == 0) {
			if (!convCnname()) {
	        	System.err.println("ERROR : The cnname label conversing file failed.\n");
	        	return false;
	        }
		} else if (language.compareTo("En") == 0){
			if (!convEnname()) {
	        	System.err.println("ERROR : The enname label conversing file failed.\n");
	        	return false;
	        }
		}
		// conversion enable tag
    	if (!convEnable()) {
        	System.err.println("ERROR : The enable label conversion file failed.\n");
        	return false;
        }
        // 1 is always the root pid
        return runProcessObjlist(docElement,language,doc);
    }

	/**
     * This function is used to process a objlist element
     * @param element
     * @param pid language
     * @return boolean
     */
    public Boolean runProcessObjlist(Element element,String language,Document doc){
    	boolean valid = false;
    	// put child object obj into the list
    	@SuppressWarnings("unchecked")
		List<Element> objs = element.elements("chapter");
    	for(Element obj:objs){
    		boolean validObj = runProcessObj(obj,language,doc); 
    		valid = Boolean.valueOf(validObj || false);
		}
    	return valid;
    }
    
	/**
     * This function is used to process a objlist element
     * @param element
     * @param pid language
     * @return boolean
     */
    public Boolean runProcessObjlist2(Element element,String language,Document doc){
    	boolean valid = false;
    	// put child object obj into the list
    	@SuppressWarnings("unchecked")
		List<Element> objs = element.elements("obj");
    	for(Element obj:objs){
    		boolean validObj = runProcessObj(obj,language,doc); 
    		valid = Boolean.valueOf(validObj || false);
		}
    	return valid;
    }
    
    /**
     * This function is used to process a obj element2000
     * @param element
     * @param pid
     * @param order
     * @return boolean
     */
    private boolean runProcessObj(Element element, String language,Document doc) {
//    	SDBDocument doc = new SDBDocument () ;
    	boolean valid = true;
    	// initialize from element
//		Integer id = Integer.parseInt(getString(element,"id"));
//		String cnname = getString(element,"cnname");
//		String enname = getString(element,"enname");
		String cnpath = getString(element,"cnpath");
		String enpath = getString(element,"enpath");
		String enabled = getString(element,"enabled");
		// if the docuemnt contains objlist, that means the current element is a directory
		Element objlist = element.element("objlist") ;
		boolean isDirectory = objlist != null ;
		// initial enabled element 
//		if ( enabled.compareToIgnoreCase("false") == 0 ||
//			 enabled.compareToIgnoreCase("f") == 0 ||
//			 enabled.compareToIgnoreCase("0") == 0 ) {
//			valid = Boolean.valueOf(false);
//		}
		// checkObj is able to check validity of the element
		// 1) make sure the id does not exist in xmlconfig
		// 2) make sure either cnname or enname exist 锛�or both )
		// 3) make sure cnname exist if cnpath appears
		// 4) make sure enname exist if enpath appears
		// 5) make sure cnpath and enpath does not exist if isDirectory = true
		// 6) make sure cnpath and enpath files are valid
//		if ( false == checkObj(id, cnname, enname, cnpath, enpath, isDirectory )) {
//			System.err.print("ERROR : Failed to check object : " + enname + ".\n" );
//			valid = Boolean.valueOf(false);
//		}
		// process subobjlist if exist
		if ( isDirectory && ( false == runProcessObjlist2( objlist, language, doc ) ) ) {
			System.err.print ( "ERROR : Failed to process obj list.\n") ;
			valid = Boolean.valueOf(false);
		}

		// conversion cnpath tag to para and deleting enpath tag or the opposite situation	
		if (language.compareTo("Cn") == 0) {
			if (!convCnpath(cnpath,getContent(cnpath,enabled,doc))) {
	        	System.err.println("ERROR : The cnpath label conversing file failed.\n");
	        	return false;
	        }
		} else if (language.compareTo("En") == 0){
			if (!convEnpath(enpath,getContent(enpath,enabled,doc))) {
	        	System.err.println("ERROR : The enpath label conversing file failed.\n");
	        	return false;
	        }
		}
		return valid ;
	}
    
    /**
     * The function is deleting statement 
     * @param 
     * @return boolean
     */
    private boolean convState() {
    	String oldStateEle = "<.xml version=\"1.0\" encoding=\"UTF-8\".>";
        String newStateEle = "";
        if (!convTOC(confFile,oldStateEle,newStateEle)){
        	System.err.println("ERROR : The statement label deleting file failed.\n");
        	return false;
        }
		return true;
	}
	
	/**
     * The function is conversion .html to .xml
     * @param 
     * @return boolean
     */
    public boolean convHtml(){
    	String oldHtmlEle = ".html";
        String newXmlEle = ".xml";
        if (!convTOC(confFile,oldHtmlEle,newXmlEle)){
        	System.err.println("ERROR : The .html conversion file failed.\n");
        	return false;
        }
    	return true;
    }
	
    /**
     * The function is conversion enable tag to info
     * @param 
     * @return boolean
     */
    public boolean convEnable(){
    	String oldEnableEle = "<enabled>.*</enabled>";
        String newEnableEle = "";
        if (!convTOC(confFile,oldEnableEle,newEnableEle)){
        	System.err.println("ERROR : The enable label conversion file failed.\n");
        	return false;
        }
    	return true;
    }
    
    /**
     * The function is conversion cnpath tag to title and deleting enpath tag 
     * @param title
     * @return boolean
     */
    public boolean convCnpath(String cnpath,String content){
//    	System.out.println("*********************************************************************************************");
//		System.out.println("cnpath : " + content);
//		System.out.println("confFile : " + confFile);
//		System.out.println("*********************************************************************************************\n");
    	String oldCnPathEle = "<cnpath>" + cnpath + "</cnpath>";
        String newCnPathEle = "<para>" + content + "</para>";
        String oldEnPathEle = "<enpath>.*</enpath>";
        String newEnPathEle = "";
        if (!convTOC(confFile,oldCnPathEle,newCnPathEle)){
        	System.err.println("ERROR : The cnpath label conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldEnPathEle,newEnPathEle)){
        	System.err.println("ERROR : The enpath label deleting file failed.\n");
        	return false;
        } 
    	return true;
    }
    
    /**
     * The function is conversion enpath tag to title and deleting cnpath tag 
     * @param title
     * @return boolean
     */
    public boolean convEnpath(String enpath,String content){
    	String oldEnPathEle = "<enpath>" + enpath + "</enpath>";
        String newEnPathEle = "<para>" + content + "</para>";
        String oldCnPathEle = "<cnpath>.*</cnpath>";
        String newCnPathEle = "";
        if (!convTOC(confFile,oldEnPathEle,newEnPathEle)){
        	System.err.println("ERROR : The enpath label conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldCnPathEle,newCnPathEle)){
        	System.err.println("ERROR : The cnpath label deleting file failed.\n");
        	return false;
        } 
    	return true;
    }
    
	/**
     * The function is conversion cnname tag to title and deleting enname tag 
     * @return boolean
     */
    public boolean convCnname(){
    	String oldCnnameEleBegin = "<cnname>";
        String newCnnameEleBegin = "<title>";
        String oldCnnameEleEnd = "</cnname>";
        String newCnnameEleEnd = "</title></info>";
        String oldEnnameEle = "<enname>.*</enname>";
        String newEnnameEle = "";
        if (!convTOC(confFile,oldCnnameEleBegin,newCnnameEleBegin)){
        	System.err.println("ERROR : The cnname begin label conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldCnnameEleEnd,newCnnameEleEnd)){
        	System.err.println("ERROR : The cnname end label conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldEnnameEle,newEnnameEle)){
        	System.err.println("ERROR : The enname label deleting file failed.\n");
        	return false;
        } 
    	return true;
    }
    
    /**
     * The function is conversion ename tag to title and deleting cnname tag 
     * @return boolean
     */
    public boolean convEnname(){
    	String oldEnnameEleBegin = "<enname>";
        String newEnnameEleBegin = "<title>";
        String oldEnnameEleEnd = "</enname>";
        String newEnnameEleEnd = "<title></info>";
        String oldCnnameEle = "<cnname>.*</cnname>";
        String newCnnameEle = "";
        if (!convTOC(confFile,oldEnnameEleBegin,newEnnameEleBegin)){
        	System.err.println("ERROR : The enname begin label conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldEnnameEleEnd,newEnnameEleEnd)){
        	System.err.println("ERROR : The enname end label conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldCnnameEle,newCnnameEle)){
        	System.err.println("ERROR : The cnname label deleting file failed.\n");
        	return false;
        } 
    	return true;
    }
    
    /**
     * The function is conversion obj tag of root to chapter
     * @param 
     * @return boolean
     */
    public boolean convRootObj(){
    	String oldObjEleBegin = "<obj>";
        String newObjEleBegin = "<chapter>";
        String oldObjEleEnd = "</obj>";
        String newObjEleEnd = "</chapter>";
        if (!convTOC(confFile,oldObjEleBegin,newObjEleBegin)){
        	System.err.println("ERROR : The begin obj label of root conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldObjEleEnd,newObjEleEnd)){
        	System.err.println("ERROR : The end obj label of root conversing file failed.\n");
        	return false;
        } 
    	return true;
    }
    
    /**
     * The function is conversion obj tag to section
     * @param 
     * @return boolean
     */
    public boolean convObj(){
    	String oldObjEleBegin = "<obj>";
        String newObjEleBegin = "<section>";
        String oldObjEleEnd = "</obj>";
        String newObjEleEnd = "</section>";
        if (!convTOC(confFile,oldObjEleBegin,newObjEleBegin)){
        	System.err.println("ERROR : The obj label conversing file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldObjEleEnd,newObjEleEnd)){
        	System.err.println("ERROR : The obj label conversing file failed.\n");
        	return false;
        }
    	return true;
    }
    
    /**
     * The function is conversion id tag to info
     * @param 
     * @return boolean
     */
    public boolean convId(){
    	String oldIdEle = "<id>.*</id>";
        String newIdEle = "<info>";
        if (!convTOC(confFile,oldIdEle,newIdEle)){
        	System.err.println("ERROR : The id label conversing file failed.\n");
        	return false;
        }
    	return true;
    }
    
    /**
     * The function is conversion doc tag to book
     * @param 
     * @return boolean
     */
    public boolean convDoc(){
    	String oldRootEleBegin = "<doc.*>";
        String newRootEleBegin = "<book xmlns=\"http://docbook.org/ns/docbook\" version=\"5.0\">";
        String oldRootEleEnd = "</doc>";
        String newRootEleEnd = "</book>";
        if (!convTOC(confFile,oldRootEleBegin,newRootEleBegin)){
        	System.err.println("ERROR : The root begin label converting file failed.\n");
        	return false;
        }
        if (!convTOC(confFile,oldRootEleEnd,newRootEleEnd)){
        	System.err.println("ERROR : The root end label converting file failed.\n");
        	return false;
        }
    	return true;
    }
    
    /**
     * The function is deleting objlist tag
     * @param 
     * @return boolean
     */
    public boolean delObjlist(){
    	String oldObjlistEle = "<.*objlist>";
        String newObjlistEle = "";
        if (!convTOC(confFile,oldObjlistEle,newObjlistEle)){
        	System.err.println("ERROR : The objlist label deleting file failed.\n");
        	return false;
        }
    	return true;
    }
    
    /**
     * The function is able to check validity of the element
     * @param id
     * @param cnname
     * @param enname
     * @param cnpath
     * @param enpath
     * @param isDirectory
     * @return boolean
     */
    private boolean checkObj(Integer id, String cnname,
			String enname, String cnpath, String enpath,
			boolean isDirectory ) {
		// checkObj is able to check validity of the element
		// 1) make sure the id does not exist in xmlconfig
		// 2) make sure either cnname or enname exist 锛�or both )
		// 3) make sure cnname exist if cnpath appears
		// 4) make sure enname exist if enpath appears
		// 5) make sure cnpath and enpath does not exist if isDirectory = true
		// 6) make sure cnpath and enpath files are valid
		
		// 1) make sure the id does not exist in xmlconfig
		if ( xmlconfig.idExist ( id ) == true ) {
			System.err.println ( "ERROR : id " + id + " already exists.\n");
			return false ;
		}
		// 2) make sure either cnname or enname exist 锛�or both )
		if ( cnname.compareTo("") == 0 &&
			 enname.compareTo("") == 0 ) {
			System.err.println ( "ERROR : either cnname or enname must exist:" + id + ".\n" );
			return false ;
		}
		// 3) make sure cnname exist if cnpath appears
		if ( cnpath.compareTo("") != 0 &&
			 cnname.compareTo("") == 0 ) {
			System.err.println ( "ERROR : cnpath must has a name:" + id + ".\n" );
			return false ;
		}
		// 4) make sure enname exist if enpath appears
		if ( enpath.compareTo("") != 0 &&
			 enname.compareTo("") == 0 ) {
			System.err.println ( "ERROR : enpath must has a name:" + id + ".\n" );
			return false ;
		}
		// 5) make sure cnpath and enpath does not exist if isDirectory = true
		if ( isDirectory && ( enpath.compareTo("")!=0 || cnpath.compareTo("")!=0)) {
			System.err.println ( "ERROR : directory cannot have a path" + id + ".\n" );
			return false ;
		}
		// 6) make sure cnpath files are valid
		if ( fileExist(changeSuffix(cnpath)) == false && false == isDirectory){
			System.err.println ( "ERROR : file " + cnname + " does not exist.\n" );
			return false;
		}
		// 7) make sure enpath files are valid
//		if ( fileExist(changeSuffix(enpath)) == false && false == isDirectory){
//			System.err.println ( "ERROR : file " + enname + " does not exist.\n" );
//			return false;
//		}
		return true ;
	}
    
    public String changeSuffix(String path){
    	Pattern str_xml = Pattern.compile(".html",Pattern.CASE_INSENSITIVE);
		Matcher suffix = str_xml.matcher(path);
		while(suffix.find()){
			path = suffix.replaceAll(".xml");
			suffix = str_xml.matcher(path);
		}
    	return path;
    }
    
    /**
     * The function is verifing that the file exists
     * @param path
     * @return boolean
     */
    public boolean fileExist(String path){
    	File file = new File(rootDir + "/" + path);
//    	System.out.println(String.valueOf(file));
    	if(!file.exists()){    
    		return false;     
    	}   
    	return true;
    }
    
    /**
     * This function is Judging whether the element exists, the return of its value or an empty string if not exist
     * @param element
     * @param tag
     * @return string
     */
    public String getString(Element element,String tag){
    	Element elementEle = element.element(tag);
    	if ( null == elementEle ) {
    		return "" ;
    	}
    	return elementEle.getTextTrim() ;
    }

    /**
     * The function is converting generated TOC file
     * @param path,oldStr,newStr
     * @return boolean
     * @throws IOException,FileNotFoundException
     */
 	private Boolean convTOC(String path,String oldStr,String newStr)  {
 		String content = null;
 		BufferedReader br = null;
 		try {
 			if (path.compareTo("") == 0){
 				System.err.println("ERROR : The parameter path is empty.\n");
 				return false;
 			}
 			br = new BufferedReader(new InputStreamReader(new FileInputStream(String.valueOf(new File(path))),"UTF-8"));
 			StringBuffer sb = new  StringBuffer();
 			String temp = null;
// 			String dollerSign = "$";
 			

 			while((temp=br.readLine())!=null){ 
 				sb.append(temp);  
 				sb.append("\r\n");
 			}
 			content = sb.toString();
 			
 			Pattern pattern_root = Pattern.compile(oldStr,Pattern.CASE_INSENSITIVE);
 			Matcher root = pattern_root.matcher(content);
 			while(root.find()){
 				content = root.replaceAll(newStr);
 				root = pattern_root.matcher(content);
 			}
 			br.close(); 			
 		} catch (FileNotFoundException e) {  
 			e.printStackTrace();  
		} catch (IOException e) {
			e.printStackTrace();
		} 
 		
// 		RandomAccessFile newFile = null;
 		File file = new File(path);
		try {
			PrintStream ps = new PrintStream(new FileOutputStream(file));
			ps.println(content);
			ps.close();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} 
 		return true;
 	}
    
   /**
    * The function is getting the file contents
    * @param path
    * @return string
    */
	private String getContent(String path,String enabled,Document doc) {
		String content = null;
		
		try {
			if (path.compareTo("") == 0){
				return "";
			}
			BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(String.valueOf(new File(rootDir + "/" + path))),  "UTF-8"));
			StringBuffer sb = new  StringBuffer();
			String statement_str = "<.xml version=\"1.0\" encoding=\"utf-8\" .>";
			String DOCTYPE1_str = "<!DOCTYPE article PUBLIC \"-//OASIS//DTD DocBook XML V4.5//EN\"";
			String DOCTYPE2_str = "\"http://www.oasis-open.org/docbook/xml/4.5/docbookx.dtd\">";
			String article_str = "<.*article.*>";
			String uselessTitle_str = "<title></title>";
			String sectionBegin_str = "<sect.*>";
			String sectionEnd_str = "</sect.*>";
			String dollar_str = "(?<!\\\\)\\$";
			String bracketsBegin_str = "< ";
			String bracketsEnd_str = " >";
			String cn_str = "<[^\\x00-\\xff]+>";
			String image_dir = "./images/";
//			String api_dir = "./api/";
			
			String temp = null;

			while((temp=br.readLine())!=null){ 
				sb.append(temp);  
				sb.append("\r\n");
			}
			content = sb.toString();
			
//			Pattern pattern_statement = Pattern.compile(statement_str,Pattern.CASE_INSENSITIVE);
//			Matcher statement = pattern_statement.matcher(content);
//			while(statement.find()){
//				content = statement.replaceFirst("");
//				statement = pattern_statement.matcher(content);
//			}
//			
//			Pattern pattern_DOCTYPE1 = Pattern.compile(DOCTYPE1_str,Pattern.CASE_INSENSITIVE);
//			Matcher DOCTYPE1 = pattern_DOCTYPE1.matcher(content);
//			while(DOCTYPE1.find()){
//				content = DOCTYPE1.replaceFirst("");
//				DOCTYPE1 = pattern_DOCTYPE1.matcher(content);
//			}
//			
//			Pattern pattern_DOCTYPE2 = Pattern.compile(DOCTYPE2_str,Pattern.CASE_INSENSITIVE);
//			Matcher DOCTYPE2 = pattern_DOCTYPE2.matcher(content);
//			while(DOCTYPE2.find()){
//				content = DOCTYPE2.replaceFirst("");
//				DOCTYPE2 = pattern_DOCTYPE2.matcher(content);
//			}
			
			Pattern pattern_url = Pattern.compile("url=[\"|\']([^http://|^api|^./|^ch].*?)[\"|\']",Pattern.CASE_INSENSITIVE);
			Matcher url = pattern_url.matcher(sb);
			while(url.find()){
				String match_url = url.group(1);
	 			
				InputSource inputSource = new InputSource(new FileInputStream(tocFile_bak));
				XPath xPath = XPathFactory.newInstance().newXPath();
				String expression = "//objlist/obj[cnpath='" + match_url + "' or enpath='" + match_url + "']/enabled";
				Node node = (Node) xPath.evaluate(expression,inputSource,XPathConstants.NODE);
				
				if (null == node) {
					System.err.println("The URL '" + match_url + "' is not exists!");
					return content;
				}

				content = url.replaceFirst("url=\"" + node.getTextContent() + ".html\"");
				url = pattern_url.matcher(content);
			}
			
			Pattern pattern_image = Pattern.compile("fileref=[\"|\']([^\\./].*?)[\"|\']",Pattern.CASE_INSENSITIVE);
			Matcher image = pattern_image.matcher(content);
			while(image.find()){
				String match_image = image.group(1);
				content = image.replaceFirst("fileref=\"" + image_dir + match_image + "\"");
				image = pattern_image.matcher(content);
			}
			
//			Pattern pattern_api = Pattern.compile("url=[\"|\']([api].*?)[\"|\']",Pattern.CASE_INSENSITIVE);
//			Matcher api = pattern_api.matcher(content);
//			while(api.find()){
//				String match_api = api.group(1);
//				System.out.println("*********************************************************************************************");
//	 			System.out.println("match_api : " + match_api);
//	 			System.out.println("*********************************************************************************************\n");
//				content = api.replaceFirst("url=\"" + api_dir + match_api + "\"");
//				api = pattern_api.matcher(content);
//			}
			
			Pattern pattern_statement = Pattern.compile(statement_str,Pattern.CASE_INSENSITIVE);
			Matcher statement = pattern_statement.matcher(content);
			while(statement.find()){
				content = statement.replaceFirst("");
				statement = pattern_statement.matcher(content);
			}
			
			Pattern pattern_DOCTYPE1 = Pattern.compile(DOCTYPE1_str,Pattern.CASE_INSENSITIVE);
			Matcher DOCTYPE1 = pattern_DOCTYPE1.matcher(content);
			while(DOCTYPE1.find()){
				content = DOCTYPE1.replaceFirst("");
				DOCTYPE1 = pattern_DOCTYPE1.matcher(content);
			}
			
			Pattern pattern_DOCTYPE2 = Pattern.compile(DOCTYPE2_str,Pattern.CASE_INSENSITIVE);
			Matcher DOCTYPE2 = pattern_DOCTYPE2.matcher(content);
			while(DOCTYPE2.find()){
				content = DOCTYPE2.replaceFirst("");
				DOCTYPE2 = pattern_DOCTYPE2.matcher(content);
			}
			
			Pattern pattern_article = Pattern.compile(article_str,Pattern.CASE_INSENSITIVE);
			Matcher article = pattern_article.matcher(content);
			while(article.find()){
				content = article.replaceFirst("");
				article = pattern_article.matcher(content);
			}
			
			Pattern pattern_uselessTitle = Pattern.compile(uselessTitle_str,Pattern.CASE_INSENSITIVE);
			Matcher uselessTitle = pattern_uselessTitle.matcher(content);
			while(uselessTitle.find()){
				content = uselessTitle.replaceFirst("");
				uselessTitle = pattern_uselessTitle.matcher(content);
			}
			
			Pattern pattern_sectionBegin = Pattern.compile(sectionBegin_str,Pattern.CASE_INSENSITIVE);
			Matcher sectionBegin = pattern_sectionBegin.matcher(content);
			while(sectionBegin.find()){
				content = sectionBegin.replaceFirst("<procedure><info>");
				sectionBegin = pattern_sectionBegin.matcher(content);
			}
			
			Pattern pattern_titleEnd = Pattern.compile("</title>(?!</info>)",Pattern.CASE_INSENSITIVE);
			Matcher titleEnd = pattern_titleEnd.matcher(content);
			while(titleEnd.find()){
				content = titleEnd.replaceFirst("</title></info>");
				titleEnd = pattern_titleEnd.matcher(content);
			}
			
			Pattern pattern_sectionEnd = Pattern.compile(sectionEnd_str,Pattern.CASE_INSENSITIVE);
			Matcher sectionEnd = pattern_sectionEnd.matcher(content);
			while(sectionEnd.find()){
				content = sectionEnd.replaceFirst("</procedure>");
				sectionEnd = pattern_sectionEnd.matcher(content);
			}
			
			Pattern pattern_dollar = Pattern.compile(dollar_str,Pattern.CASE_INSENSITIVE);
			Matcher dollar = pattern_dollar.matcher(content);
			while(dollar.find()){
				content = dollar.replaceFirst("&#36;");
				dollar = pattern_dollar.matcher(content);
			}
			
			Pattern pattern_bracketsBegin = Pattern.compile(bracketsBegin_str,Pattern.CASE_INSENSITIVE);
			Matcher bracketsBegin = pattern_bracketsBegin.matcher(content);
			while(bracketsBegin.find()){
				content = bracketsBegin.replaceFirst("&lt;");
				bracketsBegin = pattern_bracketsBegin.matcher(content);
			}
			
			Pattern pattern_bracketsEnd = Pattern.compile(bracketsEnd_str,Pattern.CASE_INSENSITIVE);
			Matcher bracketsEnd = pattern_bracketsEnd.matcher(content);
			while(bracketsEnd.find()){
				content = bracketsEnd.replaceFirst("&gt;");
				bracketsEnd = pattern_bracketsEnd.matcher(content);
			}
			
			Pattern pattern_cn = Pattern.compile(cn_str,Pattern.CASE_INSENSITIVE);
			Matcher cn = pattern_cn.matcher(content);
			while(cn.find()){
				String match_cn = cn.group(0);
				Pattern pattern_match_cn = Pattern.compile("<",Pattern.CASE_INSENSITIVE);
				Matcher brackets_begin = pattern_match_cn.matcher(match_cn);
				match_cn = brackets_begin.replaceFirst("&lt;");
				brackets_begin = pattern_match_cn.matcher(match_cn);
				
				Pattern pattern_match_cn2 = Pattern.compile(">",Pattern.CASE_INSENSITIVE);
				Matcher brackets_end = pattern_match_cn2.matcher(match_cn);
				match_cn = brackets_end.replaceFirst("&gt;");
				brackets_end = pattern_match_cn2.matcher(match_cn);

				content = cn.replaceFirst(match_cn);
				cn = pattern_cn.matcher(content);
			}
			
			br.close();
		} catch (FileNotFoundException e) {  
		e.printStackTrace();  
		} catch (IOException e) {  
		e.printStackTrace();  
		} catch (XPathExpressionException e) {
			e.printStackTrace();
		}
		return content; 
	}


    public static XmlConfigReader getInstance() {  
        return getInstance();  
    }  
    public XmlConfig getXmlConfig(){
        return xmlconfig;  
    }  
}
