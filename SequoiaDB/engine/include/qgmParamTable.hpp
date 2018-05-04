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

   Source File Name = qgmParamTable.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPARAMTABLE_HPP_
#define QGMPARAMTABLE_HPP_

#include "qgmDef.hpp"
#include "utilMap.hpp"
#include <vector>

using namespace bson ;

namespace engine
{
   struct _qgmBsonPair
   {
      BSONObj obj ;
      BSONElement ele ;
   } ;

   typedef std::list<_qgmBsonPair>                 QGM_CONST_TABLE ;
   typedef std::map<qgmDbAttr, BSONElement >       QGM_VAR_TABLE ;

   class _qgmParamTable : public SDBObject
   {
   public:
      _qgmParamTable() ;
      virtual ~_qgmParamTable() ;

   public:
      INT32 addConst( const qgmOpField &value,
                      const BSONElement *&out ) ;

      INT32 addConst( const BSONObj &obj,
                      const BSONElement *&out ) ;

      INT32 addVar( const qgmDbAttr &key,
                    const BSONElement *&out,
                    BOOLEAN *pExisted = NULL ) ;

      /// ensure that obj will not be released until
      /// u do not use this var or set a new value.
      INT32 setVar( const varItem &item,
                    const BSONObj &obj ) ;

      OSS_INLINE void removeVar( const qgmDbAttr &key )
      {
         _var.erase( key ) ;
         return ;
      }

      OSS_INLINE void clearVar()
      {
         _var.clear() ;
         return ;
      }

      OSS_INLINE void clearConst()
      {
         _const.clear() ;
         return ;
      }

      OSS_INLINE void clear()
      {
         _var.clear() ;
         _const.clear() ;
         return ;
      }

   private:
      QGM_CONST_TABLE _const ;
      QGM_VAR_TABLE _var ;
   } ;
   typedef class _qgmParamTable qgmParamTable ;
}

#endif

