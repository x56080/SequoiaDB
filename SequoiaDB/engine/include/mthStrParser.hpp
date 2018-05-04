/******************************************************************************

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

