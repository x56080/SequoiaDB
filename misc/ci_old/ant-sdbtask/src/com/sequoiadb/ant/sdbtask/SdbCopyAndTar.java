package com.sequoiadb.ant.sdbtask;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.InputStreamReader;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.ibm.staf.STAFException;
import com.ibm.staf.STAFHandle;
import com.ibm.staf.STAFResult;

/**
 * @author chenzichuan
 */
public class SdbCopyAndTar extends Task{
	
	private String saveHostName ; 
	private String filehostName ; 
	private String diaglogPath =null ; 
	private String savePath ; 
	private String buildNum = "" ;

	
	public void setSaveHostName( String value )
	{
		this.saveHostName = changeHostName( value ) ;
	}
	private String changeHostName( String value )
	{
		final String hostName1 = "suse-test1";
		final String hostName2 = "suse-test2";
		final String hostName3 = "suse-test3";
		final String hostName4 = "suse-test4";
		String varHostName = value ;
		if( value.equals( hostName1 ) ) varHostName = "suse-test1" ; 
		if( value.equals( hostName2 ) ) varHostName = "suse-test2" ; 
		if( value.equals( hostName3 ) ) varHostName = "suse-test3" ; 
		if( value.equals( hostName4 ) ) varHostName = "suse-test4" ;
		return varHostName ;
	}
	public void setBuildNum( String value )
	{
		this.buildNum = "." + value ;
	}
	public void setFileHostName( String value )
	{
		this.filehostName = changeHostName( value ) ;
	}
	public void setdiaglogPath( String value )
	{
		this.diaglogPath = value ; 
	}
	public void setSavePath( String value )
	{
		this.savePath = value ; 
	}
	
	private String STAFResultToString(STAFResult result) {

		String msg = "RC=" + result.rc + "\nmsg=" + result.result;
		return msg;
	}
	private String write_file1()
	{
		String file_string = null ;
		
		file_string = "#!/bin/bash \n"
				+ "checkDiaglog() \n" 
				+ "{ \n"
				+ "if test -d $1/diaglog \n"
				+ "then \n"
				+ "   echo $1/diaglog is exit ; \n"
				+ "   DIAL_NAME=$1 ; \n"
				+ "   DIAL_NAME=${DIAL_NAME##*/} ; \n"
				+ "   mkdir -p $2\"/\"$DIAL_NAME ; \n"
				+ "   cp -r \"$1/diaglog/\"  \"$2/$DIAL_NAME\" ; \n"
				+ "else \n"
				+ "   for fileName in $(ls $1) \n"
				+ "   do \n"  
				+ "      if test -d $1\"/\"$fileName \n"
				+ "      then \n"
				+ "         checkDiaglog $1\"/\"$fileName $2 ; \n"
				+ "      fi \n"
				+ "   done \n"
				+ "fi \n"
				+ "} \n"
				+ "\n"
				+ "checkConf()\n"
				+ "{\n"
				+ "if test -d $1/../conf/local\n"
				+ "then\n"
				+ "   echo $1/../conf/local is exit ;\n"
				+ "   cp -r \"$1/../conf/local\"  \"$2\" ;\n"
				+ "fi\n"
				+ "}\n"
				+ "\n"
				+ "if test -d $1 \n"
				+ "then \n"
				+ "   BASE_DIR=$(readlink -f $0) ; \n"
				+ "   BASE_DIR=$(dirname $BASE_DIR); \n"
				+ "   rm -rf $BASE_DIR\"/\"" + this.filehostName + "-diaglog ; \n"
				+ "   mkdir -p $BASE_DIR\"/" + this.filehostName + "-diaglog\" ; \n"
				+ "   DIAL_DIR=$BASE_DIR\"/" + this.filehostName + "-diaglog\" ; \n"
				+ "   checkDiaglog $1 $DIAL_DIR ; \n"
				+ "   checkConf $1 $DIAL_DIR ; \n"
				+ "fi \n"
				+ "\n"
				+ "cd $BASE_DIR ; \n"
				+ "\n"
				+ "tar -zcvf $BASE_DIR/" + this.filehostName + "-diaglog.tar.gz  " + this.filehostName+ "-diaglog ; \n" 
				+ ""
				+ ""
				+ ""
				
				;
				
				
		return file_string ;
	}
	public void execute ()
	{
		STAFHandle handle = null;
		try{
			
			String doWork = null ;
			String request = null ;
			STAFResult result = null ;

			String now_dir = System.getProperty("user.dir") ;	 
			String fileName = now_dir + "/shWork.sh" ;
			File file = new File( fileName ) ;
			if( ! file.exists() ){
				if( ! file.createNewFile() )
					throw new Exception( "create shWork.sh file fail" ) ; 
			}
			

			BufferedWriter output = new  BufferedWriter( new FileWriter( file ) ) ; 
			output.write( this.write_file1() ) ;
			output.close() ;
			
			
			
			doWork = " chmod a+x  " + this.diaglogPath + "/shWork.sh ; "
					+ "  " 
					+ this.diaglogPath + "/shWork.sh   " + this.diaglogPath + " ; " ; 

			handle = new STAFHandle("ant-sdbtasks");
			
			request = "  COPY FILE   " + fileName + "    TODIRECTORY  " + this.diaglogPath 
					+ "   TOMACHINE     " + this.filehostName ; 
			log( "exec : staf   " + saveHostName + "   " + request ) ; 
			result = handle.submit2( saveHostName ,  "FS", request);
			//log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}
						
			log(" shWork.sh file : \n" + this.write_file1() ) ;

			request = "START SHELL COMMAND " + doWork + " WAIT 30m " ; 
			
			log("exec: staf " + this.filehostName + " PROCESS " + request);
			result = handle.submit2( this.filehostName ,  "PROCESS", request);
			//log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}

			request = "COPY FILE  " 
					+ this.diaglogPath + "/" + this.filehostName + "-diaglog.tar.gz" + this.buildNum 
					+ " TODIRECTORY " + this.savePath + " TOMACHINE "
					+ this.saveHostName ;

			log("exec: staf " + this.filehostName + " FS " + request);
			result = handle.submit2(this.filehostName, "FS", request);

			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}
		}catch (STAFException e) {
			String errorMsg = "STAFException, RC=" + e.rc + "\nmsg="
					+ e.getLocalizedMessage();
			log(errorMsg);
			e.printStackTrace();
	
			throw new BuildException(errorMsg);
		}catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		finally{
			try {
				if (handle != null)
				{
					handle.unRegister();
				}
			} catch (STAFException e) {
				String errorMsg = "STAFException, RC=" + e.rc + "\nmsg="
						+ e.getLocalizedMessage();
				log(errorMsg);
			}

			handle = null;
			System.gc();
		}
	}

}
