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

   Source File Name = impCSVRecordParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_CSV_RECORD_PARSER_HPP_
#define IMP_CSV_RECORD_PARSER_HPP_

#include "impRecordParser.hpp"
#include "ossUtil.h"
#include <vector>

namespace import
{

   #define CSV_INVALID_FIELD_ID (-1)

   enum CSV_TYPE
   {
      CSV_TYPE_AUTO = 0,
      CSV_TYPE_INT,
      CSV_TYPE_LONG,
      CSV_TYPE_NUMBER,
      CSV_TYPE_DOUBLE,
      CSV_TYPE_DECIMAL,
      CSV_TYPE_BOOL,
      CSV_TYPE_STRING,
      CSV_TYPE_TIMESTAMP,
      CSV_TYPE_AUTO_TIMESTAMP,
      CSV_TYPE_DATE,
      CSV_TYPE_AUTO_DATE,
      CSV_TYPE_NULL,
      CSV_TYPE_OID,
      CSV_TYPE_REGEX,
      CSV_TYPE_BINARY,
      CSV_TYPE_SKIP,
      CSV_TYPE_NUM
   };

   struct CSVString
   {
      CHAR*    str;
      INT32    length;
      BOOLEAN  hasEscape;
      BOOLEAN  escaped;
   };

   struct CSVTimestamp
   {
      INT32 sec; // seconds
      INT32 us;  // microseconds
   };

   struct CSVRegex
   {
      CHAR* pattern;
      CHAR* option;
      INT32 patternLen;
      INT32 optionLen;
   };

   struct CSVBinary
   {
      CHAR* str;
      CHAR* bin;
      INT32 binLen;
      INT32 type;
   };

   union CSVFieldValue
   {
      INT32          intVal;
      INT64          longVal;
      FLOAT64        doubleVal;
      bson_decimal   decimalVal;
      BOOLEAN        boolVal;
      CSVString      strVal;
      CSVTimestamp   timestampVal;
      INT64          dateVal;
      CSVString      oidVal;
      CSVRegex       regexVal;
      CSVBinary      binaryVal;

      CSVFieldValue()
      {
         ossMemset(this, 0, sizeof(CSVFieldValue));
      }

      void reset( CSV_TYPE type, CSV_TYPE subType = CSV_TYPE_AUTO )
      {
         if (CSV_TYPE_BINARY == type)
         {
            SAFE_OSS_FREE(binaryVal.bin);
            binaryVal.binLen = 0;
         }
         else if (CSV_TYPE_STRING == type)
         {
            if (strVal.escaped)
            {
               SAFE_OSS_FREE(strVal.str);
            }
         }
         else if (CSV_TYPE_DECIMAL == type ||
                  (CSV_TYPE_NUMBER == type && CSV_TYPE_DECIMAL == subType))
         {
            decimal_free(&decimalVal);
         }
         ossMemset(this, 0, sizeof(CSVFieldValue));
      }
   };

   struct CSVDecimalOpt
   {
      INT32 precision;
      INT32 scale;
   };

   #define CSV_TIMESTAMP_FMT_MAX_LEN 63
   struct CSVTimestampOpt
   {
      INT32 fmtLength ;
      CHAR format[CSV_TIMESTAMP_FMT_MAX_LEN + 1] ;
   } ;

   struct CSVFieldOpt
   {
      BOOLEAN hasOpt;
      union {
         CSVDecimalOpt decimalOpt;
         CSVTimestampOpt timestampOpt ;
      } opt;

      CSVFieldOpt()
      {
         ossMemset(&opt, 0, sizeof(opt));
         hasOpt = FALSE;
      }
   };

   struct CSVField: public SDBObject
   {
      INT32          id;
      CSV_TYPE       type;
      CSV_TYPE       subType;
      string         name;
      CSVFieldOpt    opt;
      BOOLEAN        hasDefault;
      CSVFieldValue  defaultValue;

      CSVField()
      {
         id = CSV_INVALID_FIELD_ID;
         type = CSV_TYPE_AUTO;
         subType = CSV_TYPE_AUTO;
         name = "";
         hasDefault = FALSE;
      }

      ~CSVField()
      {
         if (hasDefault)
         {
            defaultValue.reset( type, subType ) ;
         }
      }
   };

   struct CSVFieldData: public SDBObject
   {
      CSV_TYPE       type;
      CSV_TYPE       subType;
      CSVFieldValue  value;

      CSVFieldData()
      {
         type = CSV_TYPE_AUTO;
         subType = CSV_TYPE_AUTO;
      }

      void reset()
      {
         if (CSV_TYPE_BINARY == type)
         {
            SAFE_OSS_FREE(value.binaryVal.bin);
            value.binaryVal.binLen = 0;
         }
         else if (CSV_TYPE_STRING == type)
         {
            if (value.strVal.escaped)
            {
               SAFE_OSS_FREE(value.strVal.str);
            }

            value.strVal.hasEscape = FALSE;
            value.strVal.escaped = FALSE;
            value.strVal.length = 0;
         }
         else if (CSV_TYPE_DECIMAL == type ||
                  (CSV_TYPE_NUMBER == type && CSV_TYPE_DECIMAL == subType))
         {
            decimal_free(&(value.decimalVal));
         }

         type = CSV_TYPE_AUTO;
      }

      ~CSVFieldData()
      {
         reset();
      }
   };

   class CSVRecordParser: public RecordParser
   {
   public:
      CSVRecordParser(const string& fieldDelimiter,
                      const string& stringDelimiter,
                      const string& dateFormat,
                      const string& timestampFormat,
                      STR_TRIM_TYPE stringTrimType,
                      BOOLEAN autoAddField,
                      BOOLEAN autoAddValue,
                      BOOLEAN hasHeaderLine,
                      BOOLEAN cast,
                      BOOLEAN ignoreNull,
                      BOOLEAN forceNotUTF8,
                      BOOLEAN strictFieldNum);
      ~CSVRecordParser();
      INT32 parseRecord(const CHAR* data, INT32 length, bson& obj);
      INT32 parseFields(const CHAR* data, INT32 length, BOOLEAN isHeaderline );
      void  printFieldsDef();

   private:
      INT32 _pushField(CSVField* field);

   private:
      vector<CSVField*> _fieldVec;
      string            _fields;
      BOOLEAN           _hasHeaderLine;
      BOOLEAN           _hasId;
      BOOLEAN           _strictFieldNum;
   };
}

#endif /* IMP_CSV_RECORD_PARSER_HPP_ */
