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


import java.util.ArrayList;
import java.util.List;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.types.Parameter;

/**
 * @author chenzichuan
 *
 */
public class createPrefix extends Task{
	
   private String prefixName = null ; 
   
   private List<Parameter> params = new ArrayList<Parameter>();
   
   public Parameter createParam(){
		 
		 Parameter param = new Parameter();
		 
		 params.add(param);
		 
		 return param;
	 }
   public void setPrefixName( String value )
   {
	   this.prefixName = value ; 
   }
   
   public void execute(){
      
      	
      String request = "" ;
      	 
      String lineNum = Integer.toString( (int)(Math.random()*1000) );
      	 
      for ( Parameter param : params ){
      	    
      	 request += param.getValue() ; 	
      	    
      }
      //request += lineNum ; 
      	 
      this.getProject().setProperty( this.prefixName  , request.replaceAll( "[-_]" , "") ) ; 


   	
   }
	
}
