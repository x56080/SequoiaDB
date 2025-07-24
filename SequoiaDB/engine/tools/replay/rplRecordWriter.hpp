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

   Source File Name = rplRecordWriter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_RECORD_WRITER_HPP_
#define REPLAY_RECORD_WRITER_HPP_

#include "rplMonitor.hpp"
#include "ossFile.hpp"
#include "ossPath.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include <sstream>

using namespace std ;
using namespace bson ;
using namespace engine ;

namespace replay
{
   class rplFileWriter : public SDBObject
   {
   public:
      rplFileWriter() ;
      ~rplFileWriter() ;

   public:
      INT32 init( const CHAR *fileName ) ;
      INT32 restore( const CHAR *fileName, INT64 size ) ;
      INT32 writeRecord( const CHAR *record ) ;
      INT32 flushAndClose() ;
      INT32 flush() ;
      const CHAR *getTmpFileName() ;
      const CHAR *getFileName() ;
      UINT64 getWriteSize() ;

   private:
      CHAR _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
      CHAR _tmpFileName[ OSS_MAX_PATHSIZE + 1 ] ;
      ossFile _writer ;
      UINT64 _size ;
   } ;

   class rplRecordWriter : public SDBObject
   {
   public:
      rplRecordWriter( Monitor *monitor, const CHAR *outputDir,
                       const CHAR *prefix, const CHAR *suffix ) ;
      ~rplRecordWriter() ;

   public:
      INT32 init() ;
      INT32 writeRecord( const CHAR *dbName, const CHAR *tableName, UINT64 lsn,
                         const CHAR *record ) ;
      INT32 submit() ;
      INT32 flush() ;
      INT32 getStatus( BSONObj &status ) ;

   private:
      INT32 _restoreFile( const CHAR *tableName, const CHAR *fileName,
                          INT64 size ) ;

   private:
      // clFullName <-> rplFileWriter
      typedef map< string, rplFileWriter* > CL2WriterMap ;
      CL2WriterMap _clWriters ;

      Monitor *_monitor ;
      string _outputDir ;
      string _prefixWithConnector ;
      string _suffixWithConnector ;
   } ;
}

#endif  /* REPLAY_RECORD_WRITER_HPP_ */


