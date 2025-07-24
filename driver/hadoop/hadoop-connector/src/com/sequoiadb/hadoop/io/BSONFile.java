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

   Source File Name = BSONFile.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.hadoop.io;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IOUtils;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.io.SequenceFile;


/**
 * 
 *  writer BSONWriter to SequenceFile
 * @className：BSONFile
 *
 * @author： gaoshengjie
 *
 * @createtime:2013年12月13日 下午3:42:31
 *
 * @changetime:TODO
 *
 * @version 1.0.0 
 *
 */
public class BSONFile {
	
	
	public static void Writer(BSONWritable bsonWritable,String filePath,Configuration conf) throws IOException{
		FileSystem fileSystem=FileSystem.get(conf);
		Path path=new Path(filePath);
		SequenceFile.Writer writer=new SequenceFile.Writer(fileSystem,conf,path,NullWritable.class,BSONWritable.class);
		writer.append(NullWritable.get(), bsonWritable);
		IOUtils.closeStream(writer);
	}
	
	
	public static List<BSONWritable> reader(String filePath,Configuration conf) throws IOException{
		FileSystem fileSystem=FileSystem.get(conf);
		Path path=new Path(filePath);
		SequenceFile.Reader reader=new SequenceFile.Reader(fileSystem,path,conf);
		NullWritable nullWritable=NullWritable.get();
		BSONWritable bsonWritable=new BSONWritable();
		List<BSONWritable> list=new ArrayList<BSONWritable>();
		while(reader.next(nullWritable,bsonWritable)){
			list.add(bsonWritable);
		}
		IOUtils.closeStream(reader);
		return list;
	}
}
