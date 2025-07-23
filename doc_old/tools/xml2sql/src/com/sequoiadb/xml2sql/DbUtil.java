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

   Source File Name = DbUtil.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.xml2sql;

public class DbUtil {	
	public static String queryEditionSQL(){
    	String sql = "SELECT * from tp_edition where id = ?;" ;
		return sql;
	}
	
	public static String insertEditionSQL(){
		String sql = "INSERT INTO tp_edition ("
												+ "id,"
												+ "cat_id,"
												+ "title,"
												+ "`describe`,"
												+ "content,"
												+ "add_time) "
    				+ "VALUES "
					+ "(?,?,?,?,null,?)";
		return sql;
	}
	
	public static String updateEditionSQL(){
		String sql = "UPDATE tp_edition SET "
											+ "id = ?, "
							    			+ "cat_id = ?, "
							    			+ "title = ?, "
							    			+ "`describe` = ?, "
							    			+ "content = null, "
							    			+ "add_time = ? "
					+ "WHERE id = ?;";
		return sql;
	}
	
	public static String queryContentSQL(){
    	String sql = "SELECT * from tp_articlecat where cat_id = ?;" ;
		return sql;
	}
	
	public static String insertContentSQL(){
		String sql = "INSERT INTO tp_articlecat ("
												+ "cat_id,"
												+ "cat_img,"
												+ "cat_name,"
												+ "cat_en_name,"
												+ "cat_type,"
												+ "keywords,"
												+ "cat_desc,"
												+ "sort_order,"
												+ "parent_id,"
												+ "content,"
												+ "is_open) "
    				+ "VALUES "
					+ "(?,null,?,null,1,'','',?,?,null,?)";
		return sql;
	}
	
	public static String updateContentSQL(){
		String sql = "UPDATE tp_articlecat SET "
												+ "cat_id = ?,"
												+ "cat_img = null,"
												+ "cat_name = ?,"
												+ "cat_en_name = null,"
												+ "cat_type = 1,"
												+ "keywords = '',"
												+ "cat_desc = '',"
												+ "sort_order = ?,"
												+ "parent_id = ?,"
												+ "content = null,"
												+ "is_open = ? "
					+ "WHERE cat_id = ?;";
		return sql;
	}
	
	public static String queryFileSQL(){
    	String sql = "SELECT article_id from tp_article where cat_id = ? AND edition = ? AND subEdition = 1;" ;
		return sql;
	}
	
	public static String insertFileSQL(){
		String sql = "INSERT INTO tp_article ("
    										+ "cat_id,"
    										+ "title,"
    										+ "filetitle,"
    										+ "fileshort,"
    										+ "content,"
    										+ "keywords,"
    										+ "is_open,"
    										+ "is_recommend,"
    										+ "add_time,"
    										+ "file_url,"
    										+ "link,"
    										+ "description,"
    										+ "sort_order,"
    										+ "short,"
    										+ "original_img,"
    										+ "thumb_img,"
    										+ "label,"
    										+ "is_link,"
    										+ "downocunt,"
    										+ "article_url,"
    										+ "edition,"
    										+ "subEdition) "
    				+ "VALUES "
					+ "(?,?,null,'',?,'',?,0,?,'','',null,?,null,null,null,null,1,0,null,?,1);";
		return sql;
	}
	
	public static String updateFileSQL(){
		String sql = "UPDATE tp_article SET "
											+ "cat_id = ?, "
											+ "title = ?, "
											+ "filetitle = null, "
											+ "fileshort = '', "
											+ "content = ?, "
											+ "keywords = '', "
											+ "is_open = ?, "
											+ "is_recommend = 0, "
											+ "add_time = ?, "
											+ "file_url = '', "
											+ "link = '', "
											+ "description = null, "
											+ "sort_order = ?, "
											+ "short = null, "
											+ "original_img = null, "
											+ "thumb_img = null, "
											+ "label = null, "
											+ "is_link = 1, "
											+ "downocunt = 0, "
											+ "article_url = null, "
											+ "edition = ?," 
											+ "subEdition = 1 "
					+ "WHERE cat_id = ? AND edition = ? AND subEdition = 1;";
		return sql;
	}
}
