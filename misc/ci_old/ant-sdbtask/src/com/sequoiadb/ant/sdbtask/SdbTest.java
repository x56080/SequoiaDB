/**
 * 
 */
package com.sequoiadb.ant.sdbtask;

import java.io.File;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.types.Parameter;

import com.ibm.staf.STAFException;
import com.ibm.staf.STAFHandle;
import com.ibm.staf.STAFResult;

/**
 * @author qiushanggao
 * 
 */
public class SdbTest extends Task {
	private String maxWaitTime = "1000m";
	
	private boolean getLogBack = true;
	
	private boolean mkDirIs = true;

	private String hostName = "localhost";

	private String scriptFileName;

	private String remoteReportsPath;

	private String masterReportsPath;
	
	private String antFileName = Integer.toString( (int)(Math.random()*100000) ) ; 

	private List<Parameter> params = new ArrayList<Parameter>();
	
	public void setGetLogBack(boolean value){
		this.getLogBack = value;
	}
	public void setMkDirIs( boolean value ){
		this.mkDirIs = value ;
	}
	public void setAntFileName( String value )
	{
		this.antFileName = value ; 
	}
	public void setHost(String value) {
		hostName = value;
	}

	public void setTestscript(String value) {
		scriptFileName = value;
	}

	public void setTimeout(String value) {
		maxWaitTime = value;
	}

	public void setRemotereports(String value) {
		remoteReportsPath = value;
	}

	public void setMasterreports(String value) {
		masterReportsPath = value;
	}

	public Parameter createParam() {
		Parameter param = new Parameter();
		params.add(param);
		return param;
	}

	private String STAFResultToString(STAFResult result) {

		String msg = "RC=" + result.rc + "\nmsg=" + result.result;
		return msg;
	}

	public void execute() {

		STAFHandle handle = null;
		try {

			handle = new STAFHandle("ant-sdbtasks");

			//String antFileFullName = this.getProject().getProperty("ant.file");
			//File tempFile = new File(antFileFullName);
			//String antFileName = tempFile.getName();
			//antFileName = antFileName.substring(0, antFileName.indexOf("."));
      
			String lineNum = Integer.toString(this.getLocation()
					.getLineNumber() );
			this.remoteReportsPath += hostName + "_" + Integer.toString( (int)(Math.random()*100000) ) + "_" + lineNum;
			
			this.masterReportsPath += File.separator + hostName + "_"+ this.antFileName + "_" + lineNum + File.separator;
			File dir = null;
			if( this.mkDirIs ){
				dir = new File(masterReportsPath);
			
				if (!dir.mkdirs())
				{
					log("Failed to create dir:" + masterReportsPath);
					throw new BuildException("Failed to create dir:" + masterReportsPath);
				}
				else
				{
					log("Success to create dir:" + masterReportsPath);
				}
			}

			// Staf PROCESS START SHELL COMMAND ant -l
			// ${test.machine.deploy.path}/install-basic-in-host.log -f
			// ${test.machine.deploy.path}/install-basic-in-host.xml
			// -Dtest.basedir=${test.machine.deploy.path}
			// -Ddeploy.filename=${deploy.tar.file.name} WORKDIR
			// ${test.machine.deploy.path} WAIT 30m
			String request = "START SHELL COMMAND ant -f " + scriptFileName
					+ " -l " + scriptFileName + lineNum +".log";

			request += " -Dtest.package.name=" + antFileName;
			request += " -Dreports.path=" + this.remoteReportsPath;
			request += " -Dparallel.num=" + lineNum;
			request += " -DhostName=" + hostName ;

			for (Parameter param : params) {
				request += " -D" + param.getName();
				request += "=" + param.getValue();
			}

			request += " WAIT " + maxWaitTime;

			log("exec: staf " + hostName + " PROCESS " + request);
			STAFResult result = handle.submit2(hostName, "PROCESS", request);

			log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}
			
			if( this.getLogBack ){
				// Staf ${test.machine.no2} FS GET FILE scriptFileName + ".log" TEXT
				request = "GET FILE " + scriptFileName + lineNum + ".log TEXT";
				log("exec: staf " + hostName + " FS " + request);
				result = handle.submit2(hostName, "FS", request);
				log(STAFResultToString(result));
				if (result.rc != STAFResult.Ok) {
					throw new BuildException(STAFResultToString(result));
				}
			}
			//log(result.result);

			// <echo message="${STAF.PATH}\bin\staf ${test.machine.no2} FS COPY
			// DIRECTORY
			// ${test.machine.deploy.path}/deploy/hlt/js_testcases/reports
			// TODIRECTORY ${test.reports.path} TOMACHINE ${host.Name}" />
			// <exec command="${STAF.PATH}\bin\staf ${test.machine.no2} FS COPY
			// DIRECTORY
			// ${test.machine.deploy.path}/deploy/hlt/js_testcases/reports
			// TODIRECTORY ${test.reports.path} TOMACHINE
			// ${host.Name}" dir="${STAF.PATH}" failonerror="true" failifexecutionfails="true">
			// <env key="PATH" path="${env.PATH}:${STAF.PATH}/bin" />
			// <env key="LD_LIBRARY_PATH"
			// path="${env.LD_LIBRARY_PATH}:${STAF.PATH}/lib" />
			// <env key="STAFCONVDIR" path="${STAF.PATH}/codepage" />
			// </exec>
			final String hostName1 = "suse-test1";
			final String hostName2 = "suse-test2";
			final String hostName3 = "suse-test3";
			final String hostName4 = "suse-test4";
			String value = InetAddress.getLocalHost().getHostName() ; 
			String varHostName = value ;
			if( value.equals( hostName1 ) ) varHostName = "suse-test1" ; 
			if( value.equals( hostName2 ) ) varHostName = "suse-test2" ; 
			if( value.equals( hostName3 ) ) varHostName = "suse-test3" ; 
			if( value.equals( hostName4 ) ) varHostName = "suse-test4" ; 
			request = "COPY DIRECTORY " + this.remoteReportsPath
					+ " TODIRECTORY " + this.masterReportsPath + " TOMACHINE "
					+ varHostName ;

			log("exec: staf " + hostName + " FS " + request);
			result = handle.submit2(hostName, "FS", request);

			log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}

			// <echo
			// message="exec ${STAF.PATH}/bin/staf ${deploy.host.name} FS DELETE ENTRY ${test.machine.test.reports} RECURSE CONFIRM"
			// />
			// <exec
			// command="${STAF.PATH}/bin/staf ${deploy.host.name} FS DELETE ENTRY ${test.machine.test.reports} RECURSE CONFIRM"
			// failifexecutionfails="true">
			// <env key="PATH" path="${env.PATH}:${STAF.PATH}/bin" />
			// <env key="LD_LIBRARY_PATH"
			// path="${env.LD_LIBRARY_PATH}:${STAF.PATH}/lib" />
			// <env key="STAFCONVDIR" path="${STAF.PATH}/codepage" />
			// </exec>
			/*
			request = "DELETE ENTRY " + this.remoteReportsPath
					+ " RECURSE CONFIRM";

			log("exec: staf " + hostName + " FS " + request);
			result = handle.submit2(hostName, "FS", request);

			log(STAFResultToString(result));
			if (result.rc != STAFResult.Ok) {
				throw new BuildException(STAFResultToString(result));
			}
			*/
		} catch (UnknownHostException e) {
			e.printStackTrace();

			throw new BuildException(e.getMessage());
		} catch (STAFException e) {
			String errorMsg = "STAFException, RC=" + e.rc + "\nmsg="
					+ e.getLocalizedMessage();
			log(errorMsg);
			e.printStackTrace();

			throw new BuildException(errorMsg);
		} finally {
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
