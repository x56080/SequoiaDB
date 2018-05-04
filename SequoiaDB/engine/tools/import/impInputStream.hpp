/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = impInputStream.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_INPUT_STREAM_HPP_
#define IMP_INPUT_STREAM_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "ossNPipe.hpp"
#include "impOptions.hpp"

using namespace std;

namespace import
{
   /* interface */
   class InputStream: public SDBObject
   {
   private:
      // disallow copy and assign
      InputStream(const InputStream&);
      void operator=(const InputStream&);
   protected:
      InputStream(){}
   public:
      virtual ~InputStream() {}
      virtual INT32 read(CHAR* buf, INT64 bufSize, INT64& readSize) = 0;

      static INT32 createInstance(INPUT_TYPE inputType,
                                  const string& input,
                                  const Options& options,
                                  InputStream*& istream);
      static void  releaseInstance(InputStream* istream);
   };

   class FileInputStream: public InputStream
   {
   public:
      FileInputStream(const string& fileName);
      ~FileInputStream();
      INT32 init();
      INT32 read(CHAR* buf, INT64 bufSize, INT64& readSize);

   private:
      string _fileName;
      OSSFILE _file;
   };

   class StdinInputStream: public InputStream
   {
   public:
      StdinInputStream();
      ~StdinInputStream();
      INT32 read(CHAR* buf, INT64 bufSize, INT64& readSize);
   };

   class UTF8InputStream: public InputStream
   {
   public:
      UTF8InputStream(InputStream* inputStream, BOOLEAN managed=FALSE);
      ~UTF8InputStream();
      INT32 read(CHAR* buf, INT64 bufSize, INT64& readSize);

   private:
      INT32 _firstRead(CHAR* buf, INT64 bufSize, INT64& readSize);
   private:
      InputStream* _upstream;
      BOOLEAN      _first;
      BOOLEAN      _managed; // true if manage the memory of upstream
   };

   class GBXInputStream: public InputStream
   {
      //TODO
   };

   class SubProcessInputStream: public InputStream
   {
   public:
      SubProcessInputStream(const string& subProcessCmd);
      ~SubProcessInputStream();
      INT32 init();
      INT32 read(CHAR* buf, INT64 bufSize, INT64& readSize);

   private:
      string   _subProcessCmd;
      OSSPID   _subPid;
      OSSNPIPE _pipe;
   };
}

#endif /* IMP_INPUT_STREAM_HPP_ */
