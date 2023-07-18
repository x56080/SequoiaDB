/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = rtnStreamDataBuilder.hpp

   Descriptive Name = Stream Data Builder

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_STREAM_DATA_BUILDER_HPP__
#define RTN_STREAM_DATA_BUILDER_HPP__

#include "dpsDef.hpp"
#include "dpsOp2Record.hpp"
#include "oss.hpp"
#include "rtnContext.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"
#include "utilStreamToken.hpp"

namespace engine
{

   /*
      _rtnStreamRecordData define
    */
   class _rtnStreamRecord
   {
   public:
      _rtnStreamRecord() = default ;
      ~_rtnStreamRecord() = default ;
   } ;

   typedef class _rtnStreamRecord rtnStreamRecord ;


   /*
      _rtnStreamControlRecord define
    */
   // control record
   class _rtnStreamControlRecord : public _rtnStreamRecord
   {
   public:
      _rtnStreamControlRecord() = default ;
      ~_rtnStreamControlRecord() = default ;

      _rtnStreamControlRecord( utilStreamControlType controlType,
                               const DPS_LSN &lsn,
                               BOOLEAN isResumeAt = FALSE,
                               BOOLEAN isNotResumable = FALSE )
      : _controlType( controlType ),
        _lsn( lsn ),
        _isResumeAt( isResumeAt ),
        _isNotResumable( isNotResumable )
      {
      }

      utilStreamControlType getControlType() const
      {
         return _controlType ;
      }

      const CHAR *getControlTypeName() const
      {
         return utilGetStreamControlTypeName( _controlType ) ;
      }

      const DPS_LSN &getLSN() const
      {
         return _lsn ;
      }

      BOOLEAN isResumeAt() const
      {
         return _isResumeAt ;
      }

      BOOLEAN isNotResumable() const
      {
         return _isNotResumable ;
      }

      INT32 getControlRC() const
      {
         return _controlRC ;
      }

      const ossPoolString &getControlReason() const
      {
         return _controlReason ;
      }

      INT32 setControlData( INT32 errorCode, const CHAR *errorDesc ) ;

   protected:
      utilStreamControlType _controlType = UTIL_STREAM_CONTROL_EMPTY ;
      DPS_LSN _lsn ;
      BOOLEAN _isResumeAt = FALSE ;
      BOOLEAN _isNotResumable = FALSE ;
      INT32 _controlRC = SDB_OK ;
      ossPoolString _controlReason ;
   } ;

   typedef class _rtnStreamControlRecord rtnStreamControlRecord ;

   /*
      _rtnStreamChangeRecord define
    */
   // change record
   class _rtnStreamChangeRecord : public _rtnStreamRecord
   {
   public:
      _rtnStreamChangeRecord() = default ;
      ~_rtnStreamChangeRecord() = default ;

      INT32 loadRecord( const CHAR *logData ) ;

      const dpsLogRecord &getRecord() const
      {
         return _logRecord ;
      }

   protected:
      dpsLogRecord _logRecord ;
   } ;

   typedef class _rtnStreamChangeRecord rtnStreamChangeRecord ;

   /*
      _rtnStreamRecordBuilderBase define
    */
   // base class of record builder
   class _rtnStreamRecordBuilderBase : public SDBObject
   {
   public:
      _rtnStreamRecordBuilderBase( utilStreamRecordType recordType,
                                   utilStreamToken &token,
                                   bson::BSONObjBuilder &builder ) ;
      virtual ~_rtnStreamRecordBuilderBase() = default ;

      INT32 init() ;
      INT32 buildRecord( const rtnStreamRecord &recordData,
                         bson::BSONObj &result ) ;

      const utilStreamToken &getToken() const
      {
         return _tokenRef ;
      }

   protected:
      virtual INT32 _prepare( const rtnStreamRecord &recordData ) = 0 ;
      virtual INT32 _buildRecord( const rtnStreamRecord &recordData,
                                  bson::BSONObjBuilder &builder ) = 0 ;

   protected:
      // record type
      utilStreamRecordType _recordType ;
      // record type name
      const CHAR *         _recordTypeName ;
      // token
      utilStreamToken &    _tokenRef ;
      // builder
      bson::BSONObjBuilder &_builder ;
   } ;

   typedef class _rtnStreamRecordBuilderBase rtnStreamRecordBuilderBase ;
   typedef class _rtnStreamRecordBuilderBase rtnStreamRecordBuilder ;

   /*
      _rtnStreamControlRecordBuilder define
    */
   // control record builder
   class _rtnStreamControlRecordBuilder : public _rtnStreamRecordBuilderBase
   {
   public:
      _rtnStreamControlRecordBuilder( bson::BSONObjBuilder &builder ) ;
      virtual ~_rtnStreamControlRecordBuilder() = default ;

   public:
      virtual INT32 _prepare( const rtnStreamRecord &recordData ) ;
      virtual INT32 _buildRecord( const rtnStreamRecord &recordData,
                                  bson::BSONObjBuilder &builder ) ;

   protected:
      utilChangeStreamToken _token ;
   } ;

   typedef class _rtnStreamControlRecordBuilder rtnStreamControlRecordBuilder ;

   /*
      _rtnStreamChangeRecordBuilder define
    */
   // change record builder
   class _rtnStreamChangeRecordBuilder : public _rtnStreamRecordBuilderBase
   {
   public:
      _rtnStreamChangeRecordBuilder( bson::BSONObjBuilder &builder ) ;
      virtual ~_rtnStreamChangeRecordBuilder() = default ;

   public:
      virtual INT32 _prepare( const rtnStreamRecord &recordData ) ;
      virtual INT32 _buildRecord( const rtnStreamRecord &recordData,
                                  bson::BSONObjBuilder &builder ) ;

   protected:
      utilChangeStreamToken _token ;
   } ;

   typedef class _rtnStreamChangeRecordBuilder rtnStreamChangeRecordBuilder ;

}

#endif // RTN_STREAM_DATA_SERIALIZER_HPP__
