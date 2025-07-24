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

   Source File Name = SdbCount.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.bson.BSONObject;

import com.sequoiadb.ant.datatype.JsonElement;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbCount extends Task {
   private String uuid = null;
   private String CSName = null;
   private String CLName = null;
   private JsonElement record = null;
   private String CountProp = null;

   public void setSdbhandle(String value) {
      uuid = value;
   }

   public void setCsname(String value) {
      CSName = value;
   }

   public void setClname(String value) {
      CLName = value;
   }

   public void setCountproperty(String value) {
      CountProp = value;
   }

   public JsonElement createQuery() {
      if (record != null) {
         throw new BuildException("Error: cannot set more than one record.");
      }

      record = new JsonElement();
      return record;
   }

   public void execute() {
      Object obj = this.getProject().getReference(uuid);
      if (!(obj instanceof Sequoiadb)) {
         throw new BuildException("The SdbUUID" + uuid
               + " cannot get Sequoiadb Object.");
      }

      try {
         Sequoiadb sdb = (Sequoiadb) obj;
         CollectionSpace cs = sdb.getCollectionSpace(CSName);
         DBCollection cl = cs.getCollection(CLName);

         long size = 0;

         if (record != null) {
            size = cl.getCount(record.toBSONObj());
            System.out.println("size="+size);
            System.out.println("CountProp="+CountProp);
         } else {
            size = cl.getCount((BSONObject)null);
            System.out.println("size="+size);
            System.out.println("CountProp="+CountProp);
         }

         this.getProject().setProperty(CountProp, Long.toString(size));

      } catch (BaseException e) {
         e.printStackTrace();
         throw new BuildException(e);
      }
   }
}
