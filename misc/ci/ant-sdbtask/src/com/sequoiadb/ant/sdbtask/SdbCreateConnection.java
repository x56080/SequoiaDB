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

   
*******************************************************************************/
package com.sequoiadb.ant.sdbtask;

import java.util.UUID;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 * 
 */
public class SdbCreateConnection extends Task {

   private String hostName = null;
   private String port = null;
   private String sdbHandle = null;

   public void setHostname(String value) {
      this.hostName = value;
   }

   public void setPort(String value) {
      this.port = value;
   }

   public void setSdbhandleproperty(String value) {
      this.sdbHandle = value;
   }

   public void execute() {
      try {
         Sequoiadb sdb = new Sequoiadb(this.hostName,
               Integer.parseInt(this.port), "", "");

         UUID uuid = UUID.randomUUID();
         String strUUID = uuid.toString();

         this.getProject().addReference(strUUID, sdb);
         this.getProject().setProperty(this.sdbHandle, strUUID);
      } catch (BaseException e) {
         e.printStackTrace();
         throw new BuildException(e.toString());
      }
   }
}
