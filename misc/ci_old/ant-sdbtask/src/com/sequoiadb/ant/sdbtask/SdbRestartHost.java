package com.sequoiadb.ant.sdbtask;

import java.io.IOException;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.ibm.staf.STAFException;
import com.ibm.staf.STAFHandle;
import com.ibm.staf.STAFResult;

public class SdbRestartHost extends Task{
	private String hostName ; 
	
	public void setHostName ( String value )
	{
		this.hostName = value ; 
	}
	private String STAFResultToString(STAFResult result) {

		String msg = "RC=" + result.rc + "\nmsg=" + result.result;
		return msg;
	}
	public void execute(){
		STAFHandle handle = null;
		try{
			handle = new STAFHandle("ant-sdbtasks");
			String strKill = " shutdown -r  0" ;  
			String request = "START SHELL COMMAND " + strKill + " WAIT 30m " ; 
			
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
