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

   Source File Name = rtnHintModifier.hpp

   Descriptive Name = Hint modifier header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/06/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_HINT_MODIFIER_HPP__
#define RTN_HINT_MODIFIER_HPP__

#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"

using bson::BSONObj ;
using bson::BSONElement ;

namespace engine
{
   enum _RTN_MODIFY_OP
   {
      RTN_MODIFY_INVALID,
      RTN_MODIFY_UPDATE,
      RTN_MODIFY_REMOVE
   } ;
   typedef _RTN_MODIFY_OP RTN_MODIFY_OP ;

   class _rtnHintModifier : public utilPooledObject
   {
   public:
      _rtnHintModifier() ;
      virtual ~_rtnHintModifier() ;

      INT32 init( const BSONObj &hint, BOOLEAN getOwned = FALSE ) ;

      RTN_MODIFY_OP getOpType() const
      {
         return _modifyOp ;
      }

      const BSONObj& getOpOption() const
      {
         return _opOption ;
      }

      void setModifyShardKey( BOOLEAN modify )
      {
         _updateShardingKey = modify ;
      }

      BOOLEAN isModifyShardKey() const
      {
         return _updateShardingKey ;
      }

      BOOLEAN needRebuild() const
      {
         return _updateShardingKey ;
      }

      INT32 hint( BSONObj &hint ) ;

   protected:
      virtual INT32 _parseModifyEle( const BSONElement &ele )
      {
         return SDB_OK ;
      }

      virtual INT32 _onInit()
      {
         return SDB_OK ;
      }

   protected:
      RTN_MODIFY_OP _modifyOp ;
      BSONObj       _hint ;
      BSONObj       _opOption ;  // Operation option, e.g. updator for update.
      BOOLEAN       _updateShardingKey ;
   } ;
   typedef _rtnHintModifier  rtnHintModifier ;
}

#endif /* RTN_HINT_MODIFIER_HPP__ */
