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

   Source File Name = testByPigServer.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoia.pig.test;

import java.io.IOException;
//import java.util.Properties;

import org.apache.pig.ExecType;
import org.apache.pig.PigServer;


public class testByPigServer {
	public static void main(String[] args) {
		try {
//			Properties props = new Properties();
//			props.setProperty("fs.default.name", "hdfs://rhel-test1:9000");
//			props.setProperty("mapred.job.tracker", "rhel-test1:9001");
			PigServer server = new PigServer(ExecType.LOCAL);
			//server.registerJar("tool.jar");
			runIdQuery(server, "info.txt");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public static void runIdQuery(PigServer server, String inputFile) throws IOException {
		server.debugOn();
//		server.registerQuery("A = load 'sequoia://192.168.20.111:40000/hadoopInputCS/hadoopInputC' using com.sequoia.pig.SequoiaLoader('name:chararray, age:chararray');");
//		server.registerQuery("B = foreach A generate $0 as id;");
//		server.registerQuery("store A into 'id.out';");
		server.registerScript("testPig.script");
//		server.registerQuery("store A into '192.168.20.111:40000/pigOutCs/pigOutC' using com.sequoia.pig.SequoiaWriter('info:name, info:age');");
	}

}
