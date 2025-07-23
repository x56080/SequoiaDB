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

   Source File Name = Driver.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.jdbc;

import java.sql.Connection;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.Properties;
import java.util.logging.Logger;

import com.sequoiadb.jdbc.utils.StringUtil;



public class Driver implements java.sql.Driver{
	
	private static final String PREFIX = "jdbc:sequoiadb:";
	
	public static final String HOST_PROPERTY_KEY = "HOST";
	
	private static final String LOADBALANCE_URL_PREFIX = "jdbc:mysql:loadbalance://";
	
	public static final String PORT_PROPERTY_KEY = "PORT";
	
	public static final String DBNAME_PROPERTY_KEY = "DBNAME";
	
	static {
		try {
			java.sql.DriverManager.registerDriver(new Driver());
		} catch (SQLException E) {
			throw new RuntimeException("Can't register driver!");
		}
	}
    
	public Driver() throws SQLException {
		// Required for Class.forName().newInstance()
	}
	
	public Connection connect(String url, Properties info) throws SQLException {
		
		try {
			Connection newConn = new com.sequoiadb.jdbc.Connection("192.168.20.45",
					11810, null,null, null);
			
			return newConn;
		} catch (SQLException sqlEx) {
			
			throw sqlEx;
		} 
	}

	public boolean acceptsURL(String url) throws SQLException {
		return url != null && url.toLowerCase().startsWith(PREFIX);
	}

	public DriverPropertyInfo[] getPropertyInfo(String url, Properties info)
			throws SQLException {
		 DriverPropertyInfo sharedCache = new DriverPropertyInfo(
		            "shared_cache", "false");
		        sharedCache.choices = new String[] { "true", "false" };
		        sharedCache.description =
		            "Enable SQLite Shared-Cache mode, native driver only.";
		        sharedCache.required = false;

		 return new DriverPropertyInfo[] { sharedCache };
	}

	public int getMajorVersion() {return 1;}

	public int getMinorVersion() {return 1;}

	public boolean jdbcCompliant() {return false;}

	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}
    
	public String host(Properties props) {
		return props.getProperty(HOST_PROPERTY_KEY, "localhost");
	}
	
	public int port(Properties props) {
		return Integer.parseInt(props.getProperty(PORT_PROPERTY_KEY, "3306"));
	}
	
	public String database(Properties props) {
		return props.getProperty(DBNAME_PROPERTY_KEY);
	}
}
