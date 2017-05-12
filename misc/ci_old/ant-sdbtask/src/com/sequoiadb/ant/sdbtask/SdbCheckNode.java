package com.sequoiadb.ant.sdbtask;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.net.InetAddress;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.ibm.staf.STAFException;
import com.ibm.staf.STAFHandle;
import com.ibm.staf.STAFResult;

public class SdbCheckNode extends Task{
	private String hostName ; 
	
	public void setHostName( String value )
	{
		this.hostName = this.changeHostName( value ) ; 
	}
	private String STAFResultToString(STAFResult result) {

		String msg = "RC=" + result.rc + "\nmsg=" + result.result;
		return msg;
	}
	private String changeHostName( String value )
	{
		final String hostName1 = "suse-test1";
		final String hostName2 = "suse-test2";
		final String hostName3 = "suse-test3";
		final String hostName4 = "suse-test4";
		String varHostName = value ;
		if( value.equals( hostName1 ) ) varHostName = "suse-test1.control" ; 
		if( value.equals( hostName2 ) ) varHostName = "suse-test2.control" ; 
		if( value.equals( hostName3 ) ) varHostName = "suse-test3.control" ; 
		if( value.equals( hostName4 ) ) varHostName = "suse-test4.control" ;
		
		return varHostName ; 
	}
	public void execute(){
		STAFHandle handle = null;
		String file_string = null ;
		String doWork = null ;
		try{
			handle = new STAFHandle("ant-sdbtasks");
			file_string = "serverPort=`ifconfig | grep eth | awk '{print $1}'` ; \n " +
		      "for list in $string  \n " +
		      "do  \n " +
			  "  case ${list:10:(5)} in  \n " + 
			  " 50000 ) echo 50000 port have start;; \n " + 
			  " 30000 ) echo 30000 port have start;; \n " + 
			  " 51000 ) echo 51000 port have start;;\n " + 
			  "  *) echo all port have start;; \n " + 
			  "esac \n" +
			  "done ; \n " ;
			String now_dir = System.getProperty("user.dir") ; 
			String fileName = now_dir + "/CheckNode.sh" ;
			File file = new File( fileName ) ; 
			if( ! file.exists() ){
				if( ! file.createNewFile() )
					throw new Exception( "create CheckNode.sh file fail" ) ; 
			}
			BufferedWriter output = new  BufferedWriter( new FileWriter( file ) ) ; 
			output.write( file_string ) ;
			output.close() ; 
			
			InetAddress addr = InetAddress.getLocalHost() ; 
			String localHostName = this.changeHostName( addr.getHostName().toString() ) ; 
			String request = null ; 
			STAFResult result = null ; 
			
			doWork = " chmod a+x /opt/sequoiadb/CheckNode.sh ; "
					+ " /opt/sequoiadb/CheckNode.sh ; " 
					+ " rm  /opt/sequoiadb/CheckNode.sh ; " ; 
			
			request = "  COPY FILE   " + fileName + "    TODIRECTORY /opt/sequoiadb   TOMACHINE     " + this.hostName ; 
			log( "exec : staf   " + localHostName + request ) ; 
			result = handle.submit2( localHostName ,  "FS", request);
			log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}
			
			request = "  START SHELL COMMAND   " + doWork + "   WORKDIR  /opt/sequoiadb   WAIT 30m " ; 
			
			log("exec: staf   " + this.hostName + "  PROCESS  " + request);
			result = handle.submit2( this.hostName ,  "PROCESS", request);
			log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}}catch (STAFException e) {
				String errorMsg = "STAFException, RC=" + e.rc + "\nmsg="
				+ e.getLocalizedMessage();
		log(errorMsg);
		e.printStackTrace();

		throw new BuildException(errorMsg);
	} catch (Exception e) {
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
