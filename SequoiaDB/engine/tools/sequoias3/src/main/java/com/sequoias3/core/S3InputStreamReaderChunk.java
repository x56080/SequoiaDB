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

   Source File Name = S3InputStreamReaderChunk.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.core;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.ReadListener;
import javax.servlet.ServletInputStream;
import java.io.IOException;

public class S3InputStreamReaderChunk extends ServletInputStream {
    private static final Logger logger = LoggerFactory.getLogger(S3InputStreamReaderChunk.class);

    private ServletInputStream is;
    private int chunkSize    = 0;
    private int chunkOffset  = 0;
    private boolean chunkEnd = false;

    public S3InputStreamReaderChunk(ServletInputStream is){
        this.is = is;
    }

    public int read(byte[] buf, int offset, int len) throws IOException{
        return readChunk(buf, offset, len);
    }

    private int readChunk(byte[] buf, int offset, int len) throws IOException{
        if (chunkEnd){
            return -1;
        }
        if (chunkOffset > chunkSize){
            logger.error("chunkOffset:{} > chunkSize:{}", chunkOffset, chunkSize);
            throw new IOException("chunkOffset:" + chunkOffset + " > chunkSize:" + chunkSize);
        }
        if (chunkOffset >= chunkSize){
            this.chunkOffset = 0;
            this.chunkSize   = getNextChunkSize();
            if (chunkSize == 0) {
                chunkEnd = true;
                return -1;
            }
        }

        int tempLength = 0;
        if(chunkOffset < chunkSize) {
            tempLength = is.read(buf, offset, Math.min(len, (chunkSize - chunkOffset)));
            if (tempLength > 0){
                chunkOffset += tempLength;
            }
        }

        return tempLength;
    }

    private int getNextChunkSize() throws IOException{
        int size = 0;
        byte[] lineBuf = new byte[1024];
        while ((size = is.readLine(lineBuf, 0, 1024)) != -1 ) {
            if (lineBuf[0] == '\r' && lineBuf[1] == '\n'){
                continue;
            }
            String chunkHead = new String(lineBuf, 0, size);
            try {
                int lengthIndex = chunkHead.indexOf(";");
                if (lengthIndex != -1) {//S3 chunk格式，
                    String length = chunkHead.substring(0, lengthIndex);
                    return Integer.parseInt(length, 16);
                } else {//HTTP协议默认chunk格式
                    return Integer.parseInt(chunkHead, 16);
                }
            }catch (Exception e){
                logger.error("get chunksize failed. chunk head:"+chunkHead);
            }
        }

        return 0;
    }

    @Override
    public boolean isFinished() {
        throw new RuntimeException("invalid function.");
    }

    @Override
    public boolean isReady() {
        throw new RuntimeException("invalid function.");
    }

    @Override
    public void setReadListener(ReadListener readListener) {
        throw new RuntimeException("invalid function.");
    }

    @Override
    public int read() throws IOException {
        throw new IOException("invalid function.");
    }
}
