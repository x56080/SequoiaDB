/**
 * Copyright (C) 2023 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.sequoiadb.tool;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class FileLoader {
    private File file;
    private BufferedReader reader;
    private String[] currentLines;
    private int cacheCount;
    private int currentLineIndex;
    private int maxLines;

    public FileLoader(String fileName, int maxLines) throws IOException {
        this.file = new File(fileName);
        this.reader = new BufferedReader(new FileReader(this.file));
        this.maxLines = maxLines < 2 ? 2 : maxLines;
        this.currentLines = new String[this.maxLines];
        this.cacheCount = 0;
        this.currentLineIndex = 0;
    }

    public String getLine() throws IOException {
        if (this.currentLineIndex >= cacheCount) {
            // 如果已经获取完本地的缓存，类需要自动从文件再加载指定行数的内容。
            cacheCount = 0;
            String line = this.reader.readLine();
            while (line != null && cacheCount < this.maxLines) {
                this.currentLines[cacheCount] = line;
                cacheCount++;
                if (cacheCount < this.maxLines) { // 防止把内容读丢了
                    line = this.reader.readLine();
                } else {
                    break;
                }
            }
            if (cacheCount == 0) {
                // 如果文件已经没有内容可以加载了，类接口返回 null，并关闭文件
                this.reader.close();
                return null;
            }
            this.currentLineIndex = 0;
        }
        String line = this.currentLines[this.currentLineIndex];
        this.currentLineIndex++;
        return line;
    }

    public static List<String> getAllLines(String fileName) throws IOException {
        ArrayList<String> lineList = new ArrayList<>();
        File file = new File(fileName);
        BufferedReader reader = new BufferedReader(new FileReader(file));

        // read all the lines from the file
        try {
            String line = reader.readLine();
            while (line != null) {
                lineList.add(line);
                line = reader.readLine();
            }
        } finally {
            reader.close();
        }
        return lineList;
    }
}

