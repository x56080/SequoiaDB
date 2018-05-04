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

   Source File Name = mthSActionParser.hpp

   Descriptive Name = mth selector action parser

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SACTIONPARSER_HPP_
#define MTH_SACTIONPARSER_HPP_

#include "mthSAction.hpp"
#include "ossLatch.hpp"
#include <map>

namespace engine
{
   class _mthSActionParser : public SDBObject
   {
   public:
      _mthSActionParser() ;
      ~_mthSActionParser() ;

   public:
      static const _mthSActionParser *instance() ;

      INT32 parse( const bson::BSONElement &e,
                   _mthSAction &action ) const ;

      INT32 buildDefaultValueAction( const bson::BSONElement &e,
                                     _mthSAction &action ) const ;

      INT32 buildSliceAction( INT32 begin,
                              INT32 limit,
                               _mthSAction &action ) const ;
   public:
      /// all children will be used as a singleton
      /// do not hold any dynamic member in child class.
      class parser : public SDBObject
      {
      public:
         parser(){}
         virtual ~parser() {}

      public:
         virtual INT32 parse( const bson::BSONElement &e,
                              _mthSAction &action ) const = 0 ;

         OSS_INLINE const std::string &getActionName() const
         {
            return _name ;
         }

      protected:
         std::string _name ;
      } ;

   private:
      typedef std::map<std::string, parser *> PARSERS ;

   private:
      INT32 _registerParsers() ;
   private:
      PARSERS _parsers ;
   } ;
   typedef class _mthSActionParser mthSActionParser ;

   class _mthTypeParser : public _mthSActionParser::parser
   {
   public:
      _mthTypeParser()
      {
         _name = MTH_S_TYPE ;
      }
      virtual ~_mthTypeParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthSizeParser : public _mthSActionParser::parser
   {
   public:
      _mthSizeParser()
      {
         _name = MTH_S_SIZE ;
      }
      virtual ~_mthSizeParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;
}

#endif

