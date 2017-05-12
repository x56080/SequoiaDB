/**
*
*/
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
