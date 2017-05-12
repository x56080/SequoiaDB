package com.sequoiadb.ant.sdbtask;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.net.InetAddress;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.ibm.staf.STAFException;
import com.ibm.staf.STAFHandle;
import com.ibm.staf.STAFResult;

/**
 * @author chenzichuan
 */
public class SdbNetWork extends Task {
	
	
	private String upOrDown = null ;
	private String hostName = null ;
	
	public void setUpOrDown( String value )
	{
		value = value.toLowerCase() ; 
		this.upOrDown = value ; 
	}
	public void setHostName( String value )
	{
		this.hostName = this.changeHostName( value ) ; 
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
	private String STAFResultToString(STAFResult result) {

		String msg = "RC=" + result.rc + "\nmsg=" + result.result;
		return msg;
	}
	
	public void execute ()
	{
		STAFHandle handle = null;
		try{
			
			String doWork = null ;
			String file_string = null ;
			handle = new STAFHandle("ant-sdbtasks");
			
			if( this.upOrDown.equals( "down" ) ){
				//file_string is a shell program , and it will be wrode in killNIC.sh file
				file_string = "nicName=`ifconfig | grep eth | awk '{print $1}'` ; \n " +
						      "portName=`ifconfig | grep addr:192.168 | awk '{print \\$2}'` ; \n " +
						      "i=0 ; \n " +
						      "for list in $portName  \n " +
							  "do \n " + 
							  "  echo $list | grep 192.168.30 ; \n " + 
							  "if [ $? -eq 0 ] ; then \n " + 
							  "  getNic=${nicName:i*5:i+4} \n " + 
							  "fi \n " + 
							  "  let i++; \n " + 
							  "done ; \n " + 
							  "if [ !   -n $getNic ] ; then \n " +
							  "  echo fail to getNic Name ; \n " +
							  "  exit 1 ; \n " + 
							  "fi \n " ; 
				file_string += "ifconfig $getNic down ; \n " 
						+ "  if [ $?  -ne  0 ] ; then \n "
						+ "   echo fail to ifconfig $getNic " + this.upOrDown + " ; \n "
						+ "   exit 1 ; \n " 
						+ "  fi \n " ;
			}
			else if ( this.upOrDown.equals( "up" ) ){
				file_string = "nicName=$(ifconfig | grep eth | awk '{print $1}') ; \n"
						    + "nicName_all=$(ifconfig -a | grep eth | awk '{print $1}') ; \n"
						    + "for list in $nicName_all \n"
						    + "do \n"
						    + "   echo $list | grep $nicName ; \n"
						    + "   if [ 0 -ne $? ] ; then \n"
						    + "      getNic=$list  ; \n"
						    + "      break ; \n"
						    + "   fi \n"
						    + "done \n"
						    + "ifconfig $getNic up ; \n" 
						    + "if [ $?  -ne  0 ] ; then \n"
						    + "   echo fail to ifconfig $getNic " + this.upOrDown + " ; \n"
						    + "   exit 1 ; \n"
						    + "fi \n";   
					
			}
			else {
				throw new BuildException( "upOrDown must be up or down" ) ; 
			}
			
			String now_dir = System.getProperty("user.dir") ; 
			String fileName = now_dir + "/killNIC.sh" ;
			File file = new File( fileName ) ; 
			if( ! file.exists() ){
				if( ! file.createNewFile() )
					throw new Exception( "create killNIC.sh file fail" ) ; 
			}
			BufferedWriter output = new  BufferedWriter( new FileWriter( file ) ) ; 
			output.write( file_string ) ;
			output.close() ; 
			
			InetAddress addr = InetAddress.getLocalHost() ; 
			String localHostName = this.changeHostName( addr.getHostName().toString() ) ; 
			String request = null ; 
			STAFResult result = null ; 
			
			doWork = " chmod a+x /opt/sequoiadb/killNIC.sh ; "
					+ " /opt/sequoiadb/killNIC.sh ; " 
					+ " rm  /opt/sequoiadb/killNIC.sh ; " ; 
			
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
			}
			
			/*
			request = "DELETE ENTRY " + fileName + " RECURSE CONFIRM" ; 
			log("exec : staf " + this.hostName + "  " + request ) ; 
			result = handle.submit2( this.hostName ,  "FS", request );
			log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}
			
			if( ! file.delete() ){
				throw new Exception( "delete killNIC.sh file fail" ) ; 
			}
			*/
			
		}catch (STAFException e) {
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
