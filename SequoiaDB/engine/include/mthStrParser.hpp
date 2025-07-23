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

   Source File Name = mthStrParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_STRPARSER_HPP_
#define MTH_STRPARSER_HPP_

#include "mthSActionParser.hpp"

namespace engine
{
   class _mthSubStrParser : public _mthSActionParser::parser
   {
   public:
      _mthSubStrParser()
      {
         _name = MTH_S_SUBSTR ;
      }
      virtual ~_mthSubStrParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthSubStrCPParser : public _mthSActionParser::parser
   {
   public:
      _mthSubStrCPParser()
      {
         _name = MTH_S_SUBSTRCP ;
      }
      virtual ~_mthSubStrCPParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthSubStrBytesParser : public _mthSActionParser::parser
   {
   public:
      _mthSubStrBytesParser()
      {
         _name = MTH_S_SUBSTRBYTES ;
      }
      virtual ~_mthSubStrBytesParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthRightCPParser : public _mthSActionParser::parser
   {
   public:
      _mthRightCPParser()
      {
         _name = MTH_S_RIGHTCP ;
      }
      virtual ~_mthRightCPParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthRightBytesParser : public _mthSActionParser::parser
   {
   public:
      _mthRightBytesParser()
      {
         _name = MTH_S_RIGHTBYTES ;
      }
      virtual ~_mthRightBytesParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthLeftCPParser : public _mthSActionParser::parser
   {
   public:
      _mthLeftCPParser()
      {
         _name = MTH_S_LEFTCP ;
      }
      virtual ~_mthLeftCPParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthLeftBytesParser : public _mthSActionParser::parser
   {
   public:
      _mthLeftBytesParser()
      {
         _name = MTH_S_LEFTBYTES ;
      }
      virtual ~_mthLeftBytesParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthConcatParser : public _mthSActionParser::parser
   {
   public:
      _mthConcatParser()
      {
         _name = MTH_S_CONCAT ;
      }
      virtual ~_mthConcatParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthDayParser : public _mthSActionParser::parser
   {
   public:
      _mthDayParser()
      {
         _name = MTH_S_DAY ;
      }
      virtual ~_mthDayParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthMonthParser : public _mthSActionParser::parser
   {
   public:
      _mthMonthParser()
      {
         _name = MTH_S_MONTH ;
      }
      virtual ~_mthMonthParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthYearParser : public _mthSActionParser::parser
   {
   public:
      _mthYearParser()
      {
         _name = MTH_S_YEAR ;
      }
      virtual ~_mthYearParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;


   class _mthStrLenParser : public _mthSActionParser::parser
   {
   public:
      _mthStrLenParser()
      {
         _name = MTH_S_STRLEN ;
      }
      virtual ~_mthStrLenParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthStrLenBytesParser : public _mthSActionParser::parser
   {
   public:
      _mthStrLenBytesParser()
      {
         _name = MTH_S_STRLENBYTES ;
      }
      virtual ~_mthStrLenBytesParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthStrLenCPParser : public _mthSActionParser::parser
   {
   public:
      _mthStrLenCPParser()
      {
         _name = MTH_S_STRLENCP ;
      }
      virtual ~_mthStrLenCPParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthLowerParser : public _mthSActionParser::parser
   {
   public:
      _mthLowerParser()
      {
         _name = MTH_S_LOWER ;
      }
      virtual ~_mthLowerParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthUpperParser : public _mthSActionParser::parser
   {
   public:
      _mthUpperParser()
      {
         _name = MTH_S_UPPER ;
      }
      virtual ~_mthUpperParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthTrimParser : public _mthSActionParser::parser
   {
   public:
      _mthTrimParser()
      {
         _name = MTH_S_TRIM ;
      }
      virtual ~_mthTrimParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthLTrimParser : public _mthSActionParser::parser
   {
   public:
      _mthLTrimParser()
      {
         _name = MTH_S_LTRIM ;
      }
      virtual ~_mthLTrimParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthRTrimParser : public _mthSActionParser::parser
   {
   public:
      _mthRTrimParser()
      {
         _name = MTH_S_RTRIM ;
      }
      virtual ~_mthRTrimParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;
}

#endif

