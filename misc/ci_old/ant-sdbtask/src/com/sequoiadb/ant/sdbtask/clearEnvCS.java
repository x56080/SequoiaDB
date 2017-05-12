package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.Task;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
public class clearEnvCS extends Task {
	private String hostName;
	private String csprefix;
	private int port = 50000;
	
	public void setHostName( String value )
	{
		this.hostName = value ;
	}
	public void setCsprefix( String value )
	{
		this.csprefix = value;
	}
	public void setPort( String value )
	{
		this.port = Integer.parseInt( value );
	}
	
	public void execute(){
		try{
			Sequoiadb sdb = new Sequoiadb( this.hostName , this.port ,"" , "");
			DBCursor cur = sdb.listCollectionSpaces();
			String t_cs = null;
			log("one test fail , will drop the cs , cspre is "+this.csprefix);
			while( cur.hasNext() ){
				t_cs = cur.getNext().get("Name").toString();
				//System.out.println(t_cs);
				if( t_cs.contains(this.csprefix) ){
					log("will be droped cs's name is "+ t_cs);
					sdb.dropCollectionSpace( t_cs );
					//break;
				}
			}
		}catch( BaseException e ){
	    	System.out.println(e.getMessage());
	    }
	}
	
	
	

}
