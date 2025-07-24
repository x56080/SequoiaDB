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

   Source File Name = MainTask.java

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

import org.apache.hadoop.conf.*;
import org.apache.hadoop.fs.Path;
//import org.apache.hadoop.io.NullWritable;

import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
//import org.apache.hadoop.mapreduce.Reducer.Context;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.bson.BSONObject;


//import com.sequoiadb.base.Sequoiadb;
//import com.sequoiadb.hadoop.io.BSONWritable;
import com.sequoiadb.hadoop.mapreduce.SequoiadbInputFormat;
//import com.sequoiadb.hadoop.mapreduce.SequoiadbOutputFormat;
//import com.sequoiadb.hadoop.util.SdbConnAddr;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapred.TextOutputFormat;
//import org.apache.hadoop.mapred.OutputCollector;

// implements Configurable
public class MainTask {
	private String key;
	
	
	static class Mapper1 extends Mapper<Object, BSONObject,Text,IntWritable> {
		
		@Override
		protected void setup(org.apache.hadoop.mapreduce.Mapper.Context context)
				throws IOException, InterruptedException {
			selectField = context.getConfiguration().get("sequoiadb.in.field");		
		}

		public final IntWritable one = new IntWritable(1);
		public String selectField=null;
		
		@Override
		protected void map(Object key, BSONObject value,
				org.apache.hadoop.mapreduce.Mapper.Context context)
				throws IOException, InterruptedException {
			System.out.println(key);
			System.out.println(value);
	        if (null == value.get(selectField)){
	        	return;
	        }
			context.write(new Text(value.get(selectField).toString()),one);
		}

	}
	
	static  class Reduce1 extends Reducer<Text,IntWritable,Text, IntWritable> {
		@Override
		protected void reduce(Text key, Iterable<IntWritable> values,Context context)
				throws IOException, InterruptedException {
			//System.out.println(nullWritable);
			
			int sum =0;
			/*
			while (values.hasNext())
			{
				sum += values.next().get();
			}
			*/
			for(IntWritable in:values){
				sum += in.get();
			}
			context.write(key, new IntWritable(sum));
		}

	}
	/**
	 * @param args
	 * @throws IOException 
	 * @throws ClassNotFoundException 
	 * @throws InterruptedException 
	 */
	public static void main(String[] args) throws IOException, InterruptedException, ClassNotFoundException {
		
		Configuration conf=new Configuration();
		conf.addResource("sequoiadb-hadoop.xml");
		Job job=new Job(conf);
		job.setJarByClass(MainTask.class);
		job.setJobName("test");
		job.setMapperClass(MainTask.Mapper1.class);
		job.setReducerClass(MainTask.Reduce1.class);	

		
		job.setOutputKeyClass(Text.class);		
		job.setOutputValueClass(IntWritable.class);
		
		job.setMapOutputKeyClass(Text.class);
		job.setMapOutputValueClass(IntWritable.class);
		job.setNumReduceTasks(1);
		job.setInputFormatClass(SequoiadbInputFormat.class);
		FileOutputFormat.setOutputPath(job,new Path(args[0]));
		job.waitForCompletion(true);
	}

}
