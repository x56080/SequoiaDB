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

   Source File Name = mthSAction.hpp

   Descriptive Name = mth selector action

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_SACTION_HPP_
#define MTH_SACTION_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"
#include "ossUtil.hpp"
#include "mthMatchRuntime.hpp"
#include <boost/noncopyable.hpp>

namespace engine
{
   class _mthSAction : public utilPooledObject,
                       public boost::noncopyable,
                       public _mthMatchTreeHolder
   {
   public:
      _mthSAction() ;
      ~_mthSAction() ;

   public:
      OSS_INLINE void setValue( const bson::BSONElement &e )
      {
         _value = e ;
         return ;
      }

      OSS_INLINE const bson::BSONElement &getValue() const
      {
         return _value ;
      }

      OSS_INLINE void setFunc( MTH_SACTION_BUILD buildFunc,
                               MTH_SACTION_GET getFunc )
      {
         _buildFunc = buildFunc ;
         _getFunc = getFunc ;
         return ;
      }

      OSS_INLINE void setName( const CHAR *name )
      {
         _name = name ;
         return ;
      }

      OSS_INLINE const CHAR *getName() const
      {
         return _name ;
      }

      OSS_INLINE void setAttribute( MTH_S_ATTRIBUTE attr )
      {
         _attribute = attr ;
         return ;
      }

      OSS_INLINE UINT32 getAttribute() const
      {
         return _attribute ;
      }

      OSS_INLINE void setStrictDataMode( BOOLEAN strictDataMode )
      {
         _strictDataMode = strictDataMode ;
         return ;
      }

      OSS_INLINE BOOLEAN getStrictDataMode() const
      {
         return _strictDataMode ;
      }

      OSS_INLINE const bson::BSONObj &getObj()const
      {
         return _obj ;
      }

      OSS_INLINE void setObj( const bson::BSONObj &obj )
      {
         _obj = obj ;
         return ;
      }

      OSS_INLINE const bson::BSONObj &getArg() const
      {
         return _arg ;
      }

      OSS_INLINE void setArg( const bson::BSONObj &obj )
      {
         _arg = obj ;
         return ;
      }

      OSS_INLINE void clear()
      {
         _value = bson::BSONElement() ;
         _name = NULL ;
         _attribute = MTH_S_ATTR_NONE ;
         deleteMatchTree() ;
         _matchTargetBob.reset() ;
         return ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         return !MTH_ATTR_IS_VALID( _attribute ) ;
      }

      OSS_INLINE BSONObjBuilder *getMatchTargetBob()
      {
         _matchTargetBob.reset() ;
         return &_matchTargetBob ;
      }

   public:
      INT32 build( const CHAR *name,
                   const bson::BSONElement &e,
                   bson::BSONObjBuilder &builder ) ;

      INT32 get( const CHAR *name,
                 const bson::BSONElement &in,
                 bson::BSONElement &out ) ;

   private:
      MTH_SACTION_BUILD  _buildFunc ;
      MTH_SACTION_GET    _getFunc ;

   private:
      bson::BSONElement  _value ;
      const CHAR        *_name ;
      MTH_S_ATTRIBUTE    _attribute ;
      BOOLEAN            _strictDataMode ;
      // Must be reset before calling _matchTargetBob
      BSONObjBuilder     _matchTargetBob ;

      /// think about placement new ?
      /// that we can use different child classes.
      bson::BSONObj       _obj ;
      bson::BSONObj       _arg ;
   } ;
   typedef class _mthSAction mthSAction ;
}

#endif

