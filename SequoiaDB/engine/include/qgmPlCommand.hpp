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

   Source File Name = qgmPlCommand.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef QGMPLCOMMAND_HPP_
#define QGMPLCOMMAND_HPP_

#include "qgmPlan.hpp"
#include "msgDef.h"
#include "utilResult.hpp"

namespace engine
{
   class _qgmPlCommand : public _qgmPlan
   {
   public:
      _qgmPlCommand( INT32 type,
                     const qgmDbAttr &fullName,
                     const qgmField &indexName,
                     const qgmOPFieldVec &indexColumns,
                     const BSONObj &partition,
                     BOOLEAN uniqIndex ) ;

      virtual ~_qgmPlCommand() ;

   public:
      virtual void close( _pmdEDUCB *eduCB ) ;

      virtual string toString() const ;

      virtual BOOLEAN needRollback() const ;

      virtual void    buildRetInfo( BSONObjBuilder &builder ) const ;
      virtual void    setClientVersion( INT32 version ) ;
      virtual INT32   getCatalogVersion() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext( qgmFetchOut &next, _pmdEDUCB *eduCB ) ;

      INT32 _executeOnData( _pmdEDUCB *eduCB ) ;

      INT32 _executeOnCoord( _pmdEDUCB *eduCB ) ;

      void _killContext( _pmdEDUCB *eduCB ) ;

   private:
      INT32 _commandType ;
      INT64 _contextID ;
      qgmDbAttr _fullName ;
      qgmField _indexName ;
      qgmOPFieldVec _indexColumns ;
      BSONObj _partition ;
      BOOLEAN _uniqIndex ;

      utilWriteResult   _wrResult ;
      INT32   _clientVersion ;
      INT32   _catalogVersion ;

   } ;

   typedef class _qgmPlCommand qgmPlCommand ;
}

#endif

