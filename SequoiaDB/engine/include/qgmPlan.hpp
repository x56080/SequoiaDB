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

   Source File Name = qgmPlan.hpp

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
#ifndef QGMPLAN_HPP_
#define QGMPLAN_HPP_

#include "qgmDef.hpp"
#include "pd.hpp"
#include "qgmParamTable.hpp"

namespace engine
{
   enum QGM_PLAN_TYPE
   {
      QGM_PLAN_TYPE_RETURN = 0,
      QGM_PLAN_TYPE_FILTER,
      QGM_PLAN_TYPE_SCAN,
      QGM_PLAN_TYPE_NLJOIN,
      QGM_PLAN_TYPE_INSERT,
      QGM_PLAN_TYPE_UPDATE,
      QGM_PLAN_TYPE_AGGR,
      QGM_PLAN_TYPE_SORT,
      QGM_PLAN_TYPE_DELETE,
      QGM_PLAN_TYPE_COMMAND,
      QGM_PLAN_TYPE_SPLIT,
      QGM_PLAN_TYPE_HASHJOIN,

      //
      QGM_PLAN_TYPE_MAX,
   } ;

   class _qgmPlan ;
   typedef ossPoolVector<_qgmPlan *>   QGM_PINPUT ;

   class _pmdEDUCB ;

   class _qgmPlan : public _utilPooledObject
   {
   public:
      _qgmPlan( QGM_PLAN_TYPE type, const qgmField &alias ) ;
      virtual ~_qgmPlan() ;

      INT32    checkTransAutoCommit( BOOLEAN dpsValid, _pmdEDUCB *eduCB ) ;

   public:
      virtual void close() ;

      virtual string toString() const { return "" ; }

      virtual BOOLEAN needRollback() const ;
      virtual BOOLEAN canUseTrans() const ;
      virtual void    buildRetInfo( BSONObjBuilder &builder ) const ;
      virtual void    setClientVersion( INT32 version ) ;
      virtual INT32   getCatalogVersion() const ;
      virtual void    getObjects( ossPoolSet< ossPoolString > &setObjs ) const ;

      OSS_INLINE QGM_PLAN_TYPE type() const { return _type ; }

      OSS_INLINE void merge( BOOLEAN ifMerge ){ _merge = ifMerge ; }

      OSS_INLINE const qgmField &alias() const { return _alias ; }

      OSS_INLINE UINT32 inputSize() const { return _input.size() ; }

      OSS_INLINE BOOLEAN ready(){ return _initialized ;}

      OSS_INLINE void setParamTable( _qgmParamTable *param )
      {
         SDB_ASSERT( NULL != param, "impossible" ) ;
         _param = param ;
         return ;
      }

      OSS_INLINE _qgmPlan *input( UINT32 id ) const
      {
         return id <= _input.size() ?
                _input.at( id ) : NULL ;
      }

      OSS_INLINE void addVar( const varItem &item )
      {
         _varlist.push_back( item ) ;
         return ;
      }

      OSS_INLINE void setVar( const QGM_VARLIST &list )
      {
         _varlist = list ;
         return ;
      }

      INT32 addChild( _qgmPlan *child ) ;

      INT32 execute( _pmdEDUCB *eduCB ) ;

      INT32 fetchNext( qgmFetchOut &next ) ;

   protected:

      INT32       _checkTransOperator( BOOLEAN dpsValid,
                                       BOOLEAN autoCommit = FALSE ) ;
      INT32       _checkTransAutoCommit( BOOLEAN dpsValid, _pmdEDUCB *eduCB ) ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) = 0 ;

      virtual INT32 _fetchNext( qgmFetchOut &next ) = 0 ;

      virtual INT32 _checkPrivilege( _pmdEDUCB *eduCB )
      {
         return SDB_OK ;
      }

   protected:
      QGM_VARLIST _varlist ;
      QGM_PINPUT _input ;
      QGM_PLAN_TYPE _type ;
      _pmdEDUCB *_eduCB ;
      qgmField _alias ;
      BOOLEAN _executed ;
      BOOLEAN _initialized ;
      BOOLEAN _merge ;
      _qgmParamTable *_param ;

   private:
      BOOLEAN _authorized ;
   } ;

   typedef class _qgmPlan qgmPlan ;
}
#endif

