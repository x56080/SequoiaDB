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

   Source File Name = Configuration.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.xml2web;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Properties;

public class Configuration {
    private Properties conf = new Properties();;  
    private FileInputStream input;
    
    public String getValue(String confPath,String key){
    	String value = null;
        try{
        	input = new FileInputStream(confPath);
            conf.load(input);
            input.close();
            if(conf.containsKey(key)){
                value=conf.getProperty(key);
                return value;
            }
        }catch(FileNotFoundException ex){
        	System.err.print("ERROR : configuration file not found." );
            ex.printStackTrace();
        }catch(IOException ex){
        	System.err.print("ERROR : failed to load file." );
            ex.printStackTrace();
        }catch (Exception ex) {
            ex.printStackTrace();
        }
		return value; 
    }
    
    public void clear(){
        conf.clear();
    }
}
