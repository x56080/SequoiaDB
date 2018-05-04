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

   Source File Name = mthMathParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_MATHPARSER_HPP_
#define MTH_MATHPARSER_HPP_

#include "mthSActionParser.hpp"

namespace engine
{
   class _mthAbsParser : public _mthSActionParser::parser
   {
   public:
      _mthAbsParser()
      {
         _name = MTH_S_ABS ;
      }
      virtual ~_mthAbsParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthCeilingParser : public _mthSActionParser::parser
   {
   public:
      _mthCeilingParser()
      {
         _name = MTH_S_CEILING ;
      }
      virtual ~_mthCeilingParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthFloorParser : public _mthSActionParser::parser
   {
   public:
      _mthFloorParser()
      {
         _name = MTH_S_FLOOR ;
      }
      virtual ~_mthFloorParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthModParser : public _mthSActionParser::parser
   {
   public:
      _mthModParser()
      {
         _name = MTH_S_MOD ;
      }
      virtual ~_mthModParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthAddParser : public _mthSActionParser::parser
   {
   public:
      _mthAddParser()
      {
         _name = MTH_S_ADD ;
      }
      virtual ~_mthAddParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthSubtractParser : public _mthSActionParser::parser
   {
   public:
      _mthSubtractParser()
      {
         _name = MTH_S_SUBTRACT ;
      }
      virtual ~_mthSubtractParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthMultiplyParser : public _mthSActionParser::parser
   {
   public:
      _mthMultiplyParser()
      {
         _name = MTH_S_MULTIPLY ;
      }
      virtual ~_mthMultiplyParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthDivideParser : public _mthSActionParser::parser
   {
   public:
      _mthDivideParser()
      {
         _name = MTH_S_DIVIDE ;
      }
      virtual ~_mthDivideParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

}

#endif

