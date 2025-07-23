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

   Source File Name = SdbConnAddr.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.hive;

public class SdbConnAddr {
	private String host = "";
	private int port = 0;

	public String getHost() {
		return host;
	}

	public int getPort() {
		return port;
	}

	public void setHost(String value) {
		host = value;
	}

	public void setPort(int value) {
		port = value;
	}

	public SdbConnAddr() {

	}

	public SdbConnAddr(String connStr) {
		String[] splitList = connStr.split(":");

		if (splitList.length != 2) {
			throw new IllegalArgumentException("Connect String(" + connStr
					+ ") error, the form must be host:port.");
		}

		host = splitList[0];
		port = Integer.parseInt(splitList[1]);
	}

	public SdbConnAddr(String host, int port) {
		this.host = host;
		this.port = port;
	}

	@Override
	public String toString() {
		return String.format("Host:%s,Port:%d", host, port);
	}

	@Override
	public int hashCode() {
		return host.hashCode() * 31 + port;
	}

	@Override
	public boolean equals(Object other) {
		if (!(other instanceof SdbConnAddr)) {
			return false;
		}
		SdbConnAddr otherAddr = (SdbConnAddr) other;

		if (otherAddr.port != this.port) {
			return false;
		}

		if (!otherAddr.host.equals(this.host)) {
			return false;
		}

		return true;

	}
}
