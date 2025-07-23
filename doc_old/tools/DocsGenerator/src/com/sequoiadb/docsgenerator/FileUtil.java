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

   Source File Name = FileUtil.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.docsgenerator;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;

/**
 * 
 * 文件辅助类
 * 
 * @author Lichunhao
 * @since 2016-03-11
 *
 */
public class FileUtil {

	/**
	 * 新建临时md文件
	 * 
	 * @return
	 */
	public static File createMdFile() {
		File tmpFile = null;
		try {
			tmpFile = File.createTempFile("tmp", ".md");
		} catch (IOException e) {
			e.printStackTrace();
		}
		return tmpFile;
	}

	/**
	 * 读取文件内容
	 * 
	 * @param path
	 * @return 文件内容
	 * @throws IOException
	 */
	public static String read(String path) throws IOException {
		File f = new File(path);
		FileInputStream fs = new FileInputStream(f);
		String result = null;
		byte[] b = new byte[fs.available()];
		fs.read(b);
		fs.close();
		result = new String(b);
		return result;
	}

	/**
	 * 读取文件内容
	 * 
	 * @param file
	 * @return 文件内容
	 * @throws IOException
	 */
	public static String read(File file) throws IOException {
		FileInputStream fs = new FileInputStream(file);
		String result = null;
		byte[] b = new byte[fs.available()];
		fs.read(b);
		fs.close();
		result = new String(b);
		return result;
	}

	/**
	 * 写文件
	 * 
	 * @param sourceFile
	 * @param fileContent
	 * @return
	 * @throws IOException
	 */
	public static void write(File sourceFile, String fileContent) throws IOException {
		FileOutputStream fs = new FileOutputStream(sourceFile);
		byte[] b = fileContent.getBytes();
		fs.write(b);
		fs.flush();
		fs.close();
	}

	/**
	 * 追加内容到指定文件
	 * 
	 * @param sourceFile
	 * @param fileContent
	 * @return
	 * @throws IOException
	 */
	public static void append(File sourceFile, String fileContent) throws IOException {
		if (sourceFile.exists()) {
			RandomAccessFile rFile = new RandomAccessFile(sourceFile, "rw");
			byte[] b = fileContent.getBytes();
			long originLen = sourceFile.length();
			rFile.setLength(originLen + b.length);
			rFile.seek(originLen);
			rFile.write(b);
			rFile.close();
		}
	}

	/**
	 * 获取指定目录下的所有文件(不包括子文件夹)
	 * 
	 * @param dirPath
	 * @return
	 */
	public static ArrayList<File> getDirFiles(String dirPath) {
		File path = new File(dirPath);
		File[] fileArr = path.listFiles();
		ArrayList<File> files = new ArrayList<File>();

		for (File f : fileArr) {
			if (f.isFile()) {
				files.add(f);
			}
		}
		return files;
	}

	/**
	 * 获取指定目录下特定文件后缀名的文件列表(不包括子文件夹)
	 * 
	 * @param dirPath
	 * @param suffix
	 * @return
	 */
	public static ArrayList<File> getDirFiles(String dirPath, final String suffix) {
		File path = new File(dirPath);
		File[] fileArr = path.listFiles(new FilenameFilter() {
			public boolean accept(File dir, String name) {
				String lowerName = name.toLowerCase();
				String lowerSuffix = suffix.toLowerCase();
				if (lowerName.endsWith(lowerSuffix)) {
					return true;
				}
				return false;
			}

		});
		ArrayList<File> files = new ArrayList<File>();

		for (File f : fileArr) {
			if (f.isFile()) {
				files.add(f);
			}
		}
		return files;
	}

	/**
	 * 合并指定文件
	 * 
	 * @param sourceFile
	 * @param filePath
	 * @return
	 */
	public static File mergeFile(File sourceFile, String filePath) {
		try {
			String content = read(filePath);
			append(sourceFile, content);
		} catch (IOException e) {
			e.printStackTrace();
		}
		return sourceFile;
	}

	/**
	 * 合并文件夹下所有文件
	 * 
	 * @param sourceFile
	 * @param dirPath
	 * @return
	 */
	public static File mergeFiles(File sourceFile, String dirPath) {
		try {
			ArrayList<File> fileList = getDirFiles(dirPath, "md");
			for (File file : fileList) {
				String content = read(file);
				append(sourceFile, content);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return sourceFile;
	}
}
