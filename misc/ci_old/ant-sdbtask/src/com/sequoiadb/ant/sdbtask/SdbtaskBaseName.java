package com.sequoiadb.ant.sdbtask;

import java.io.File;


import org.apache.tools.ant.Task;

/**
 * @author chenzichuan
 */
public class SdbtaskBaseName extends Task {
	private String file ; 
	private String property ; 
	private String suffix = null ; 
	
	public void setFile( String value )
	{
		this.file = value ; 
	}
	
	public void setProperty( String value )
	{
		this.property = value ; 
	}
	public void setSuffix( String value )
	{
		this.suffix = value ; 
	}
	public void execute()
	{
		File filePath = new File( this.file ) ; 
		String fileName = filePath.getName(); 
		
		if( this.suffix != null ) 
		{
			String[] token = fileName.split( this.suffix ) ;
			this.getProject().setProperty( this.property  , token[0] ) ;
		}
		else{
			this.getProject().setProperty( this.property , fileName ) ; 
		}
		
	}
}
