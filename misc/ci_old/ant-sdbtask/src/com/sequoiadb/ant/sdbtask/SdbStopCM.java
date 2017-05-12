package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.ibm.staf.STAFException;
import com.ibm.staf.STAFHandle;
import com.ibm.staf.STAFResult;

/**
 * @author chenzichuan
 */
public class SdbStopCM extends Task{
	private String hostName ; 
	private String installPath = "/opt" ; 
	
	public void setInstallPath( String value )
	{
		this.installPath = value ;
	}
	public void setHostName ( String value )
	{
		this.hostName = value ; 
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
			//String strKill = " kill -9 \\(" + this.nodePort ;
			String stopcm = this.installPath + "/sequoiadb/bin/sdbcmtop" ; 
			String request = "START SHELL COMMAND " + stopcm + " WAIT 30m " ; 
			
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
