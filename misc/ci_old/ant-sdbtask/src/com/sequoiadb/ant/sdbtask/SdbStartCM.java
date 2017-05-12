package com.sequoiadb.ant.sdbtask;

import com.ibm.staf.STAFResult;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.ibm.staf.STAFException;
import com.ibm.staf.STAFHandle;

/**
 * @author chenzichuan
 */
public class SdbStartCM extends Task {
	private String hostName ; 
	private String installPath = "/opt" ; 
	
	public void setInstallPath( String value )
	{
		this.installPath = value ;
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
	public void setHostName ( String value )
	{
		this.hostName = changeHostName( value ) ; 
	}

	private String STAFResultToString(STAFResult result) {

		String msg = "RC=" + result.rc + "\nmsg=" + result.result;
		return msg;
	}
	
	public void execute ()
	{
		STAFHandle handle = null;
		try{
			handle = new STAFHandle("ant-sdbtasks");
			String doWork = null ; 
			String setSdbCMconf = " sed -i 's:AutoStart=false:AutoStart=true:g'  " + this.installPath + "/sequoiadb/conf/sdbcm.conf ; \n" ; 
			String startcm = "su - sdbadmin -c " + this.installPath + "/sequoiadb/bin/sdbcmart ; \n" ;
			doWork = setSdbCMconf + startcm ; 
			String request = "START SHELL COMMAND " + doWork + " WAIT 30m " ; 
			
			log("exec: staf " + this.hostName + " PROCESS " + request);
			STAFResult result = handle.submit2( this.hostName ,  "PROCESS", request);
			log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}
		}catch (STAFException e) {
			String errorMsg = "STAFException, RC=" + e.rc + "\nmsg="
					+ e.getLocalizedMessage();
			log(errorMsg);
			e.printStackTrace();
	
			throw new BuildException(errorMsg);
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
