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

   Source File Name = impRecordParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_PARSER_HPP_
#define IMP_RECORD_PARSER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impOptions.hpp"
#include "../client/bson/bson.h"
#include "jstobs.h"
#include <string>

using namespace std;

namespace import
{
   #define IMP_MAX_BSON_SIZE (1024 * 1024 * 16)

   // abstract class
   class RecordParser: public SDBObject
   {
   private:
      // disallow copy and assign
      RecordParser(const RecordParser&);
      void operator=(const RecordParser&);

   protected:
      RecordParser(const string& fieldDelimiter,
                   const string& stringDelimiter,
                   BOOLEAN autoAddField = TRUE,
                   BOOLEAN autoAddValue = FALSE);

   public:
      virtual ~RecordParser() {}
      virtual INT32 parseRecord(const CHAR* data, INT32 length, bson& obj) = 0;

   protected:
      string   _fieldDelimiter;
      string   _stringDelimiter;
      BOOLEAN  _autoAddField;
      BOOLEAN  _autoAddValue;

   public:
      static INT32 createInstance(INPUT_FORMAT format, const Options& options,
                                  RecordParser*& parser);
      static void  releaseInstance(RecordParser* parser);
   };

   class JSONRecordParser: public RecordParser
   {
   private:
      CJSON_MACHINE *_pMachine ;
   public:
      JSONRecordParser();
      ~JSONRecordParser();
      INT32 init() ;
      INT32 parseRecord(const CHAR* data, INT32 length, bson& obj);
   };
}

#endif /* IMP_RECORD_PARSER_HPP_ */
