##修改 PostgreSQL 的连接配置##

1. 修改 PostgreSQL 的监听地址

	编辑 /opt/postgresql/data/postgresql.conf 文件，将

	```lang-javascript
	listen_addresses = 'localhost'
	```

	改为：

	```lang-javascript
	listen_addresses = '0.0.0.0'
	```

2. 修改信任的机器列表

	编辑 /opt/postgresql/data/pg_hba.conf 文件，将

	```lang-javascript
	# IPv4 local connections:
	host  all  all  127.0.0.1/32  trust
	```

	改为：

	```lang-javascript
	# IPv4 local connections:
	host  all  all  127.0.0.1/32  trust
	host  all  all  0.0.0.1/0  trust
	```

3. 重启 PostgreSQL

	```lang-javascript
	$ bin/pg_ctl stop -s -D pg_data/ -m fast
	$ bin/postgres -D pg_data/ >> logfile 2>&1 &
	```

##JDBC连接程序##

```lang-javascript
package com.sequoiadb.sample;
import java.sql.*;

public class postgresql_sample {
   	static{
       	try {
           	Class.forName("org.postgresql.Driver");
       	} catch (ClassNotFoundException e) {
           	e.printStackTrace();
       	}
   	}
   	public static void main( String[] args ) throws SQLException{
      	String pghost = "192.168.30.182";
      	String port = "5432";
      	String databaseName = "foo";
      	// postgresql process is running in which user
      	String pgUser = "sdbadmin";
      	String url = "jdbc:postgresql://"+pghost+":"+port+"/" + databaseName;
      	Connection conn = DriverManager.getConnection(url, pgUser, null);
      	Statement stmt = conn.createStatement();
      	String sql = "select * from sdb_upcase_field ";
      	ResultSet rs = stmt.executeQuery(sql);
      	boolean isHeaderPrint = false;
      	while (rs.next()) {
           	ResultSetMetaData md = rs.getMetaData();
           	int col_num = md.getColumnCount();
           	if (!isHeaderPrint){
               	for (int i = 1; i  <= col_num; i++) {
                   	System.out.print(md.getColumnName(i) + "|");
                   	isHeaderPrint = true;
               	}
           	}
           	for (int i = 1; i <= col_num; i++) {
               	System.out.print(rs.getString(i) + "|");
           	}
           	System.out.println();
       	}
       	stmt.close();
       	conn.close();
   	}
}
```
