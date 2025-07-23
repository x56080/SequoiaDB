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

   Source File Name = SystemStatus.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3;

import com.sequoias3.config.ServiceInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;

@Component
@Order(3)
public class SystemStatus implements ApplicationRunner {
    private static final Logger logger = LoggerFactory.getLogger(SystemStatus.class);

    public static final int STATUS_INIT   = 0;
    public static final int STATUS_NORMAL = 1;
    public static final int STATUS_EXIT   = 2;

    // status: 0 Init; 1 Normal; 2 Exit
    private int systemStatus = STATUS_INIT;

    @Autowired
    ServiceInfo serviceInfo;

    @Override
    public void run(ApplicationArguments applicationArguments) throws Exception {
        writePidFile();
        this.systemStatus = STATUS_NORMAL;
    }

    public int getSystemStatus() {
        return systemStatus;
    }

    public void exitSystemStatus(){
        this.systemStatus = STATUS_EXIT;
    }

    private void writePidFile(){
        FileOutputStream fos = null;
        OutputStreamWriter osw = null;
        int processID = serviceInfo.getPid();
        int port = serviceInfo.getPort();
        String fileName = processID+".pid";
        try{
            File file = new File(fileName);
            if (!file.exists()){
                file.createNewFile();
            }
            logger.info("fileName:"+file.getAbsolutePath() + ", port:" + port);

            fos = new FileOutputStream(file);
            osw = new OutputStreamWriter(fos);
            osw.write(""+port);
            osw.close();
            fos.close();
        }catch (Exception e){
            logger.warn("write pid:port to " + fileName + " failed. e:"+e.getMessage());
        }finally {
            closeStream(osw);
            closeFile(fos);
        }
    }

    private void closeFile(FileOutputStream fos){
        try{
            if (fos != null) {
                fos.close();
            }
        }catch (Exception e){
            logger.warn("closeFile failed. e:"+e.getMessage());
        }
    }

    private void closeStream(OutputStreamWriter osw){
        try {
            if (osw != null) {
                osw.close();
            }
        }catch (Exception e){
            logger.warn("closeStream failed. e:"+e.getMessage());
        }
    }
}
