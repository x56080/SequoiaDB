package org.sequoiadb.tool;

import org.junit.Ignore;
import org.junit.Test;

public class ConcurrentFileWriterTest {
    /**
     * 文件内容为：
     * 11111
     * 22222
     * 33333
     * 符合预期
     */
    @Test
    @Ignore
    public void doubleOpenAndCloseFileTest() {
        ConcurrentFileWriter fileWriter =
                new ConcurrentFileWriter("ConcurrentFileWriter.txt", 2048*1024, 2048*1024);
        fileWriter.writeToFile("11111");
        fileWriter.closeFile();
        fileWriter.writeToFile("22222");
        fileWriter.closeFile();
        ConcurrentFileWriter fileWriter2 =
                new ConcurrentFileWriter("ConcurrentFileWriter.txt", 2048*1024, 2048*1024);
        fileWriter2.writeToFile("33333");
        fileWriter2.closeFile();
    }
}
