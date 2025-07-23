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

   Source File Name = utilStreamToken.hpp

   Descriptive Name = Stream Token

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_STREAM_TOKEN_HPP__
#define UTIL_STREAM_TOKEN_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "msgDef.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   // get name of token type
   const CHAR *utilGetTokenTypeName( UINT8 tokenType ) ;

   /*
      _utilStreamType define
    */
   // type of stream
   typedef enum _utilStreamType
   {
      UTIL_UNKNOWN_STREAM = 0,
      UTIL_CHANGE_STREAM,
      UTIL_REPLICA_STREAM
   } utilStreamType ;

   // get name of stream type
   const CHAR *utilGetStreamTypeName( utilStreamType type ) ;

   /*
      _utilStreamRecordType define
    */
   // type of record
   typedef enum _utilStreamRecordType
   {
      UTIL_STREAM_INVALID_RECORD = 0,
      UTIL_STREAM_CONTROL_RECORD,
      UTIL_STREAM_CHANGE_RECORD,
      UTIL_STREAM_DATA_RECORD
   } utilStreamRecordType ;

   // get name of record type
   const CHAR *utilGetStreamRecordTypeName( utilStreamRecordType type ) ;

   /*
      _utilStreamControlType define
    */
   // type of control record
   typedef enum _utilStreamControlType
   {
      UTIL_STREAM_CONTROL_EMPTY = 0,
      UTIL_STREAM_CONTROL_STOP,
      UTIL_STREAM_CONTROL_ERROR
   } utilStreamControlType ;

   // get name of control type
   const CHAR *utilGetStreamControlTypeName( utilStreamControlType type ) ;

   /*
      _utilStreamTokenBase define
    */
   // stream token
   class _utilStreamTokenBase : public _utilPooledObject
   {
   public:
      _utilStreamTokenBase() ;
      _utilStreamTokenBase( const _utilStreamTokenBase &other ) ;
      virtual ~_utilStreamTokenBase() ;

      _utilStreamTokenBase &operator =( const _utilStreamTokenBase &other ) ;

      // reset token
      void reset() ;
      // reset description of token
      void resetDesc() ;

      // parse token from string format
      virtual void fromString( const CHAR *str )
      {
      }

      // convert token to string format
      virtual void toString( CHAR *str ) const
      {
      }

      // convert token to BSON format
      virtual INT32 toBSON( bson::BSONObjBuilder &builder ) const
      {
         return SDB_OK ;
      }

      UINT8 getVersion() const
      {
         return _getHeader()._version ;
      }

      void setVersion( UINT8 version )
      {
         _getHeader()._version = version ;
      }

      UINT8 getTokenType() const
      {
         return _getHeader()._tokenType ;
      }

      void setTokenType( UINT8 tokenType )
      {
         _getHeader()._tokenType = tokenType ;
      }

      UINT8 getFlags() const
      {
         return _getHeader()._flags ;
      }

      void setFlags( UINT8 flags )
      {
         _getHeader()._flags = flags ;
      }

      void clearFlags()
      {
         _getHeader()._flags = MSG_STREAM_TOKEN_FLAG_EMPTY ;
      }

      void setResumeAtFlag()
      {
         OSS_BIT_SET( _getHeader()._flags,
                      MSG_STREAM_TOKEN_FLAG_RESUME_AT ) ;
      }

      BOOLEAN isResumeAt() const
      {
         return OSS_BIT_TEST( _getHeader()._flags,
                              MSG_STREAM_TOKEN_FLAG_RESUME_AT ) ? TRUE : FALSE ;
      }

      void setNotResumableFlag()
      {
         OSS_BIT_SET( _getHeader()._flags,
                      MSG_STREAM_TOKEN_FLAG_NOT_RESUMABLE ) ;
      }

      BOOLEAN isNotResumable() const
      {
         return OSS_BIT_TEST( _getHeader()._flags,
                              MSG_STREAM_TOKEN_FLAG_NOT_RESUMABLE ) ? TRUE : FALSE ;
      }

      UINT32 getSource() const
      {
         return _getHeader()._source ;
      }

      void setSource( UINT32 source )
      {
         _getHeader()._source = source ;
      }

   protected:
      /*
         _utilStreamTokenHEader define
       */
      // header of token
      typedef struct _utilStreamTokenHeader
      {
         UINT8  _version ;
         UINT8  _tokenType ;
         UINT8  _flags ;
         UINT8  _reserved ;
         UINT32 _source ;
      } _utilStreamTokenHeader ;

      /*
         _utilStreamTokenStruct define
       */
      typedef union
      {
         UINT8 _values[ MSG_STREAM_TOKEN_SIZE ] ;
         struct
         {
            _utilStreamTokenHeader _header ;
            UINT8 _desc[ MSG_STREAM_TOKEN_DESC_SIZE ] ;
         } _struct ;
      } _utilStreamTokenStruct ;
      static_assert( MSG_STREAM_TOKEN_SIZE == sizeof( _utilStreamTokenStruct ),
                     "size of _utilStreamTokenStruct is invalid" ) ;
      static_assert( MSG_STREAM_TOKEN_SIZE == sizeof( _utilStreamTokenStruct::_struct ),
                     "size of _utilStreamTokenStruct::_struct is invalid" ) ;

   protected:
      _utilStreamTokenHeader &_getHeader()
      {
         return _token._struct._header ;
      }

      const _utilStreamTokenHeader &_getHeader() const
      {
         return _token._struct._header ;
      }

      INT32 _headerToBSON( bson::BSONObjBuilder &builder ) const ;

   protected:
      _utilStreamTokenStruct _token ;
   } ;

   typedef class _utilStreamTokenBase utilStreamTokenBase ;
   typedef class _utilStreamTokenBase utilStreamToken ;


   /*
      _utilChangeStreamToken define
    */
   class _utilChangeStreamToken : public _utilStreamTokenBase
   {
   public:
      _utilChangeStreamToken() ;
      _utilChangeStreamToken( const _utilChangeStreamToken &other ) ;
      _utilChangeStreamToken( const _utilStreamTokenBase &other ) ;
      ~_utilChangeStreamToken() ;

      virtual void fromString( const CHAR *str ) ;
      virtual void toString( CHAR *str ) const ;
      virtual INT32 toBSON( bson::BSONObjBuilder &builder ) const ;

      UINT64 getGlobalTimestamp() const
      {
         return _getDesc()._globalTimestamp ;
      }

      void setGlobalTimestamp( UINT64 globalTimestamp )
      {
         _getDesc()._globalTimestamp = globalTimestamp ;
      }

      UINT64 getLSN() const
      {
         return _getDesc()._lsn ;
      }

      void setLSN( UINT64 lsn )
      {
         _getDesc()._lsn = lsn ;
      }

      UINT32 getCheckCode() const
      {
         return _getDesc()._checkCode ;
      }

      void setCheckCode( UINT32 checkCode )
      {
         _getDesc()._checkCode = checkCode ;
      }

   protected:
      /*
         _utilChangeStreamDesc define
       */
      typedef struct _utilChangeStreamDesc
      {
         // global timestamp of log ( not used )
         UINT64 _globalTimestamp ;
         // LSN of log
         UINT64 _lsn ;
         // check code of log ( current is version of LSN )
         UINT32 _checkCode ;
         // reserved ( not used )
         UINT32 _reserved ;
      } _utilChangeStreamDesc ;
      static_assert( MSG_STREAM_TOKEN_DESC_SIZE == sizeof( _utilChangeStreamDesc ),
                     "size of_utilChangeStreamDesc is invalid" ) ;

      const _utilChangeStreamDesc &_getDesc() const
      {
         return *( (const _utilChangeStreamDesc *)( &( _token._struct._desc ) ) ) ;
      }

      _utilChangeStreamDesc &_getDesc()
      {
         return *( (_utilChangeStreamDesc *)( &( _token._struct._desc ) ) ) ;
      }
   } ;
   typedef class _utilChangeStreamToken utilChangeStreamToken ;

}

#endif // UTIL_STREAM_TOKEN_HPP__
