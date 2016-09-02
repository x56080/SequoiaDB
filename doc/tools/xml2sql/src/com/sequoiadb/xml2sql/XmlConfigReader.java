package com.sequoiadb.xml2sql;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
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
import org.dom4j.io.SAXReader;
import org.w3c.dom.Node;
import org.xml.sax.InputSource;

public class XmlConfigReader {
	private static XmlConfigReader instance = new XmlConfigReader("../../../doc/toc.xml");  
    private XmlConfig xmlconfig = new XmlConfig();
	private String rootDir = new File("../../../doc/toc.xml").getParent();
	private String edition = "200" ;
    /**
     * initial configuration file
     * @param path
     */
    private XmlConfigReader(String path) {    
        try {
        	// initialize reader
        	SAXReader reader = new SAXReader();
        	// create document from file path
        	Document doc = reader.read(new File(path));
        	// get the root element from document
        	Element rootElement = doc.getRootElement();
        	// process the document
        	Boolean valid = runProcessDoc(rootElement);
        	// if the document is not valid, we need to show error message
        	if(valid == false){ 
        		System.err.println("ERROR : The document is not valid.");
        	}
        } catch (DocumentException e) {  
            e.printStackTrace();  
        }       
    }
    
    /**
     * This function is used to process a document element
     * @param element
     * @return boolean
     */
    public boolean runProcessDoc(Element element){
    	// make sure the document name is "doc"
        if(!element.getName().equals("doc")){
        	System.err.println("ERROR : The element : " + element.getName() + "is not root element.");
        	return false;
        }
        // change str .html to .xml
        // get release information
        String majorRelease = element.attributeValue("major_release") ;
        String minorRelease = element.attributeValue("minor_release") ;
        // release information sanity check
        if ( null == majorRelease || majorRelease.compareTo("") == 0 ||
        	 null == minorRelease || minorRelease.compareTo("") == 0 ) {
        	System.err.println("ERROR : Invalid release information" ) ;
        	return false ;
        }
        // construct full edition
        String editionValue = "v" + majorRelease + "." + minorRelease;
        
        edition = majorRelease + "" + minorRelease + "0";
        
        // construct edition id
        int id = Integer.parseInt(majorRelease) * 100 + Integer.parseInt(minorRelease);
        // 
        xmlconfig.addEditionValue(id,editionValue);
        // get objlist from the root element
        Element docElement = element.element("objlist");
        // make sure the element exist
        if(null == docElement){
        	System.err.println ( "ERROR : objlist does not exist in doc") ;
        	return false ;
        }
        // 3 is always the root pid
        return runProcessObjlist(docElement,3);
    }
    
    /**
     * This function is used to process a objlist element
     * @param element
     * @param pid
     * @return boolean
     */
    public boolean runProcessObjlist(Element element,int pid){
    	boolean valid = false;
    	// put child object obj into the list
    	@SuppressWarnings("unchecked")
		List<Element> objs = element.elements("obj");
    	int order = 0;
    	for(Element obj:objs){
    		boolean validObj = runProcessObj(obj,pid,order+1); 
    		valid = Boolean.valueOf(validObj || false);
    		order++;
    		
    		
//			if ( true == runProcessObj(obj,pid,++order) )
//				return true;
		}
    	return valid;
    }
    
    /**
     * This function is used to process a obj element
     * @param element
     * @param pid
     * @param order
     * @return boolean
     */
    private boolean runProcessObj(Element element, int pid, int order) {
    	SDBDocument doc = new SDBDocument () ;
    	boolean valid = true;
    	// initialize from element
		Integer id = Integer.parseInt(getString(element,"id"));
		String cnname = getString(element,"cnname");
		String enname = getString(element,"enname");
		String cnpath = getString(element,"cnpath");
		String enpath = getString(element,"enpath");
		String enabled = getString(element,"enabled");
		System.out.println("cnname : " + cnname);//
		System.out.println("enname : " + enname);//
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
		// 2) make sure either cnname or enname exist 锛�or both )
		// 3) make sure cnname exist if cnpath appears
		// 4) make sure enname exist if enpath appears
		// 5) make sure cnpath and enpath does not exist if isDirectory = true
		// 6) make sure cnpath and enpath files are valid
		if ( false == checkObj(id, cnname, enname, cnpath, enpath, isDirectory )) {
			System.err.print("ERROR : Failed to check object : " + enname + ".\n" );
			valid = Boolean.valueOf(false);
		}
		// process subobjlist if exist
		if ( isDirectory && ( false == runProcessObjlist( objlist, id ) )) {
			System.err.print ( "ERROR : Failed to process objlist.\n") ;
			valid = Boolean.valueOf(false);
		}
		// all sanity check are passed and sub objects are processed
		// now it's time to construct values and push into xmlconfig
		doc.setId ( id ) ;
		doc.setCnname ( cnname ) ;
		doc.setEnname ( enname ) ;
		doc.setPid ( pid ) ;
		doc.setOrder ( order ) ;
		doc.setCnpath ( getContent(cnpath) ) ;
		doc.setEnpath ( getContent(enpath) ) ;
		doc.setDirectory( isDirectory ) ;
		doc.setValid( valid );

		xmlconfig.addDocument(doc);
		xmlconfig.addEditionID(id);
		return valid ;
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
		if ( fileExist(cnpath) == false && false == isDirectory){
			System.err.println ( "ERROR : file " + cnname + " does not exist.\n" );
			return false;
		}
		// 7) make sure enpath files are valid
		if ( fileExist(enpath) == false && false == isDirectory){
			System.err.println ( "ERROR : file " + enname + " does not exist.\n" );
			return false;
		}
		return true ;
	}
    
    /**
     * The function is verifing that the file exists
     * @param path
     * @return boolean
     */
    public boolean fileExist(String path){
    	File file=new File(rootDir + "/" + path);
//    	System.out.println(String.valueOf(new File(rootDir + "/" + path)));
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
    * The function is getting the file contents
    * @param path
    * @return string
    */
	private String getContent(String path) {
		String content = null;
		try {
			if (path.compareTo("") == 0){
				return "";
			}
			BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(String.valueOf(new File(rootDir + "/" + path))),  "UTF-8"));
			StringBuffer sb = new  StringBuffer();
			String comm_url = "/cn/";
			String image_dir = "/cn/index/Public/Home/images/" + edition + "/";
			String api_dir = "/cn/index/Public/Home/document/" + edition;
			String mao_str = "h2 key=\"title\" data-alt=\"alt\"";
			String temp = null;
			String str = null;
			String dir[] = path.split("/");
			if (dir[0].compareTo("SdbDoc_Cn") == 0){
				str = "index.php?";
			}else if (dir[0].compareTo("SdbDoc_En") == 0){
				str = "index.php?";
			}

			while((temp=br.readLine())!=null){ 
				sb.append(temp);  
				sb.append("\r\n");
			}  
			
			content = sb.toString();
			
			Pattern pattern_url = Pattern.compile("href=[\"|\']([^http://|^api|^./].*?)[\"|\']",Pattern.CASE_INSENSITIVE);
			Matcher url = pattern_url.matcher(sb);
			while(url.find()){
				String match_url = url.group(1);
				InputSource inputSource = new InputSource(new FileInputStream("../../../doc/toc.xml"));
				XPath xPath = XPathFactory.newInstance().newXPath();
				String expression = "//objlist/obj[cnpath='" + match_url + "' or enpath='" + match_url + "']/id";
				Node node = (Node) xPath.evaluate(expression,inputSource,XPathConstants.NODE);
				
				if (null == node) {
					System.err.println("The URL '" + match_url + "' is not exists!");
					return content;
				}
				String repl = "m=Files&a=index&cat_id=" + node.getTextContent() + "&edition_id=" + xmlconfig.getEditionValue(0);
				content = url.replaceFirst("href=\"" + comm_url + str + repl + "\"");
				
				url = pattern_url.matcher(content);
			}
			Pattern pattern_image = Pattern.compile("src=[\"|\']([^\\./].*?)[\"|\']",Pattern.CASE_INSENSITIVE);
			Matcher image = pattern_image.matcher(content);
			
			while(image.find()){
				String match_image = image.group(1);

				content = image.replaceFirst("src=\"" + image_dir + match_image + "\"");
				image = pattern_image.matcher(content);
			}
			Pattern pattern_api = Pattern.compile("href=[\"|\']([api].*?)[\"|\']",Pattern.CASE_INSENSITIVE);
			Matcher api = pattern_api.matcher(content);
			while(api.find()){
				String match_api = api.group(1);

				content = api.replaceFirst("href=\"" + api_dir + "/" + match_api + "\"");
				api = pattern_api.matcher(content);
			}
			Pattern pattern_mao = Pattern.compile("h2.id=[\"|\'](.*?)[\"|\']",Pattern.CASE_INSENSITIVE);
			Matcher mao = pattern_mao.matcher(content);
			while(mao.find()){
//				String match_mao = mao.group(1);

				content = mao.replaceFirst(mao_str);
				mao = pattern_mao.matcher(content);
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
        return instance;  
    }  
    public XmlConfig getXmlConfig(){
        return xmlconfig;  
    }  
}
