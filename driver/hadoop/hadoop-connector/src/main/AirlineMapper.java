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

   Source File Name = AirlineMapper.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package main;

import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.bson.BSONObject;

import com.sequoiadb.hadoop.io.BSONWritable;

public class AirlineMapper extends
		Mapper<Object, BSONObject, Text, BSONWritable> {

	private static Log log = LogFactory.getLog(AirlineMapper.class);

	@Override
	public void setup(Context context) {

	}

	public void map(Object key, BSONObject value, Context context)
			throws IOException, InterruptedException {

		// 提取身份证为KEY
		String credentNo = (String) value.get("credent_no");
		if (credentNo != null) {
			context.write(new Text(credentNo), new BSONWritable(
					value));
		}
	}
}
