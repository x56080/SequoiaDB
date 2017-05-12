package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbTransBegin extends Task {
   private String uuid = null;
   
   public void setSdbhandle(String value)
   {
      uuid = value;
   }
   
   public void execute() {
      Object obj = this.getProject().getReference(uuid);
      if (! (obj instanceof Sequoiadb))
      {
         throw new BuildException("The SdbUUID" + uuid + " cannot get Sequoiadb Object.");         
      }
      
      try
      {
         Sequoiadb sdb = (Sequoiadb) obj;
         sdb.beginTransaction();
      }
      catch(BaseException e)
      {
         e.printStackTrace();
         throw new BuildException(e.toString());
      }
   }

}
