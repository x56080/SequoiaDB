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

   Source File Name = catSequence.cpp

   Descriptive Name = Sequence definition

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catSequence.hpp"
#include "catGTSDef.hpp"
#include "utilArguments.hpp"
#include "utilStr.hpp"
#include "utilMath.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"
#include <string>
#include <set>

using namespace bson ;

namespace engine
{
   _catSequence::_catSequence( const std::string& name )
   {
      _name = name ;
      _internal = FALSE ;
      _version = 0 ;
      _currentValue = 1 ;
      _cachedValue = _currentValue ;
      _startValue = 1 ;
      _minValue = 1 ;
      _maxValue = OSS_SINT64_MAX ;
      _increment = 1 ;
      _cacheSize = 1000 ;
      _acquireSize = 1000 ;
      _cycled = FALSE ;
      _initial = TRUE ;
      _exceeded = FALSE ;
      _ID = UTIL_GLOBAL_NULL ;
   }

   _catSequence::~_catSequence()
   {
   }

   void _catSequence::setOID( const bson::OID& oid )
   {
      _oid = oid ;
   }

   void _catSequence::setInternal( BOOLEAN internal )
   {
      _internal = internal ;
   }

   void _catSequence::setCachedValue( INT64 cachedValue )
   {
      _cachedValue = cachedValue ;
   }

   void _catSequence::setVersion( INT64 version )
   {
      _version = version ;
   }

   void _catSequence::increaseVersion()
   {
      _version++ ;
   }

   void _catSequence::setCurrentValue( INT64 currentValue )
   {
      _currentValue = currentValue ;
   }

   void _catSequence::setStartValue( INT64 startValue )
   {
      _startValue = startValue ;
   }

   void _catSequence::setMinValue( INT64 minValue )
   {
      _minValue = minValue ;
   }

   void _catSequence::setMaxValue( INT64 maxValue )
   {
      _maxValue = maxValue ;
   }

   void _catSequence::setIncrement( INT32 increment )
   {
      _increment = increment ;
   }

   void _catSequence::setCacheSize( INT32 cacheSize )
   {
      _cacheSize = cacheSize ;
   }

   void _catSequence::setAcquireSize( INT32 acquireSize )
   {
      _acquireSize = acquireSize ;
   }

   void _catSequence::setCycled( BOOLEAN cycled )
   {
      _cycled = cycled ;
   }

   void _catSequence::setInitial( BOOLEAN initial )
   {
      _initial = initial ;
   }

   void _catSequence::setExceeded( BOOLEAN exceeded )
   {
      _exceeded = exceeded ;
   }

   void _catSequence::setID( utilSequenceID id )
   {
      _ID = id ;
   }

   INT32 _catSequence::toBSONObj( bson::BSONObj& obj, BOOLEAN forUpdate ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         if ( !forUpdate )
         {
            builder.append( CAT_SEQUENCE_OID, _oid ) ;
            builder.append( CAT_SEQUENCE_NAME, _name ) ;
            builder.append( CAT_SEQUENCE_INTERNAL, (bool)_internal ) ;
            builder.append( CAT_SEQUENCE_ID, (INT64)_ID ) ;
         }
         builder.append( CAT_SEQUENCE_VERSION, _version ) ;
         builder.append( CAT_SEQUENCE_CURRENT_VALUE, _currentValue ) ;
         builder.append( CAT_SEQUENCE_START_VALUE, _startValue ) ;
         builder.append( CAT_SEQUENCE_MIN_VALUE, _minValue ) ;
         builder.append( CAT_SEQUENCE_MAX_VALUE, _maxValue ) ;
         builder.append( CAT_SEQUENCE_INCREMENT, _increment ) ;
         builder.append( CAT_SEQUENCE_CACHE_SIZE, _cacheSize ) ;
         builder.append( CAT_SEQUENCE_ACQUIRE_SIZE, _acquireSize ) ;
         builder.append( CAT_SEQUENCE_CYCLED, (bool)_cycled ) ;
         builder.append( CAT_SEQUENCE_INITIAL, (bool)_initial ) ;

         obj = builder.obj() ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build BSONObj for sequence %s, exception: %s, rc=%d",
                 _name.c_str(), e.what(), rc ) ;
      }

      return rc ;
   }

   void _catSequence::copyFrom( const _catSequence& other, BOOLEAN withInternalField )
   {
      _currentValue = other.currentValue() ;
      _cachedValue = other.cachedValue() ;
      _increment = other.increment() ;
      _startValue = other.startValue() ;
      _minValue = other.minValue() ;
      _maxValue = other.maxValue() ;
      _cacheSize = other.cacheSize() ;
      _acquireSize = other.acquireSize() ;
      _cycled = other.cycled() ;
      _initial = other.initial() ;
      _exceeded = other.exceeded() ;
      if ( withInternalField )
      {
         _oid = other.oid() ;
         _version = other.version() ;
         _internal = other.internal() ;
         _ID = other.ID() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_SET_OPTIONS, "_catSequence::setOptions" )
   INT32 _catSequence::setOptions( const BSONObj& options,
                                   BOOLEAN init,
                                   BOOLEAN withInternalField,
                                   UINT32 * alterMask )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      UINT32 fieldMask = UTIL_ARG_FIELD_EMPTY ;
      utilSequenceID ID = UTIL_SEQUENCEID_NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_SET_OPTIONS ) ;

      // CAT_SEQUENCE_INCREMENT
      ele = options.getField( CAT_SEQUENCE_INCREMENT ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 increment = ele.numberLong() ;
         if ( increment > OSS_SINT32_MAX || increment < OSS_SINT32_MIN )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Option[%s] is overflow: %lld",
                    CAT_SEQUENCE_INCREMENT, increment ) ;
            goto error ;
         }
         if ( this->increment() != (INT32) increment )
         {
            this->setIncrement( (INT32) increment ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_INCREMENT_FIELD ) ;
         }
         if ( increment < 0 && init )
         {
            _currentValue = -1 ;
            _cachedValue = _currentValue ;
            _startValue = -1 ;
            _minValue = OSS_SINT64_MIN ;
            _maxValue = -1 ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_INCREMENT ) ;
         goto error ;
      }

      // CAT_SEQUENCE_START_VALUE
      ele = options.getField( CAT_SEQUENCE_START_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 startValue = ele.numberLong() ;
         if ( this->startValue() != startValue )
         {
            this->setStartValue( startValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) ;
            if ( init )
            {
               this->setCurrentValue( startValue ) ;
               this->setCachedValue( startValue ) ;
            }
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_START_VALUE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_MIN_VALUE
      ele = options.getField( CAT_SEQUENCE_MIN_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 minValue = ele.numberLong() ;
         if ( this->minValue() != minValue )
         {
            this->setMinValue( minValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_MINVALUE_FIELD ) ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_MIN_VALUE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_MAX_VALUE
      ele = options.getField( CAT_SEQUENCE_MAX_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 maxValue = ele.numberLong() ;
         if ( this->maxValue() != maxValue )
         {
            this->setMaxValue( maxValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_MAXVALUE_FIELD ) ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_MAX_VALUE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_CURRENT_VALUE
      ele = options.getField( CAT_SEQUENCE_CURRENT_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 currentValue = ele.numberLong() ;
         if ( this->currentValue() != currentValue )
         {
            if( currentValue < this->minValue() ||
                currentValue > this->maxValue() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid currentValue[%lld], out of bounds for "
                       "minValue[%lld] and maxValue[%lld]",
                       currentValue, minValue(), maxValue() ) ;
               goto error ;
            }
            /* make sure getNextValue from CurrentValue not StartValue
               when alter CurrentValue on a non-used sequence */
            if( this->initial() )
            {
               this->setInitial( FALSE ) ;
            }
            this->setCurrentValue( currentValue ) ;
            this->setCachedValue( currentValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CURVALUE_FIELD ) ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_CURRENT_VALUE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_CACHE_SIZE
      ele = options.getField( CAT_SEQUENCE_CACHE_SIZE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 cacheSize = ele.numberLong() ;
         if ( cacheSize > OSS_SINT32_MAX || cacheSize < OSS_SINT32_MIN )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Option[%s] is overflow: %lld",
                    CAT_SEQUENCE_CACHE_SIZE, cacheSize ) ;
            goto error ;
         }
         if ( this->cacheSize() != (INT32) cacheSize )
         {
            this->setCacheSize( (INT32) cacheSize ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CACHESIZE_FIELD ) ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_CACHE_SIZE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_ACQUIRE_SIZE
      ele = options.getField( CAT_SEQUENCE_ACQUIRE_SIZE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 acquireSize = ele.numberLong() ;
         if ( acquireSize > OSS_SINT32_MAX || acquireSize < OSS_SINT32_MIN )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Option[%s] is overflow: %lld",
                    CAT_SEQUENCE_ACQUIRE_SIZE, acquireSize ) ;
            goto error ;
         }
         if ( this->acquireSize() != (INT32) acquireSize )
         {
            this->setAcquireSize( (INT32) acquireSize ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_ACQUIRESIZE_FIELD ) ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_ACQUIRE_SIZE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_CYCLED
      ele = options.getField( CAT_SEQUENCE_CYCLED ) ;
      if ( Bool == ele.type() )
      {
         bool cycled = ele.Bool();
         if ( this->cycled() != (BOOLEAN) cycled )
         {
            this->setCycled( cycled ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CYCLED_FIELD ) ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_CYCLED ) ;
         goto error ;
      }

      // CAT_SEQUENCE_INITIAL
      ele = options.getField( CAT_SEQUENCE_INITIAL ) ;
      if ( Bool == ele.type() )
      {
         bool initial = ele.Bool();
         if ( this->initial() != (BOOLEAN) initial )
         {
            this->setInitial( initial ) ;
         }
      }
      else if ( EOO != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_INITIAL ) ;
         goto error ;
      }

      if ( !this->initial() )
      {
         // clear flag before exceeded check
         this->setExceeded( FALSE ) ;
         if ( this->increment() > 0 )
         {
            if ( this->cachedValue() == this->maxValue() ||
                 this->cachedValue() > ( this->maxValue() - this->increment() ) )
            {
               this->setExceeded( TRUE ) ;
            }
         }
         else if ( this->increment() < 0 )
         {
            if ( this->cachedValue() == this->minValue() ||
                 this->cachedValue() < ( this->minValue() - this->increment() ) )
            {
               this->setExceeded( TRUE ) ;
            }
         }
      }

      if ( withInternalField )
      {
         // CAT_SEQUENCE_OID
         ele = options.getField( CAT_SEQUENCE_OID ) ;
         if ( jstOID == ele.type() )
         {
            OID oid = ele.OID() ;
            if ( this->oid() != oid )
            {
               this->setOID( oid ) ;
            }
         }
         else if ( EOO != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                    ele.type(), CAT_SEQUENCE_OID ) ;
            goto error ;
         }

         // CAT_SEQUENCE_ID
         ele = options.getField( CAT_SEQUENCE_ID ) ;
         if ( ele.isNumber() )
         {
            ID = ele.Long() ;
            if ( this->ID() != ID )
            {
               this->setID( ID ) ;
            }
         }
         else if ( EOO != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                    ele.type(), CAT_SEQUENCE_ID ) ;
            goto error ;
         }

         // CAT_SEQUENCE_VERSION
         ele = options.getField( CAT_SEQUENCE_VERSION ) ;
         if ( NumberInt == ele.type() || NumberLong == ele.type() )
         {
            INT64 version = ele.numberLong() ;
            if ( this->version() != version )
            {
               this->setVersion( version ) ;
            }
         }
         else if ( EOO != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                    ele.type(), CAT_SEQUENCE_VERSION ) ;
            goto error ;
         }
      }

      if ( init || withInternalField )
      {
         // CAT_SEQUENCE_INTERNAL
         ele = options.getField( CAT_SEQUENCE_INTERNAL ) ;
         if ( Bool == ele.type() )
         {
            bool internal = ele.Bool();
            if ( this->internal() != (BOOLEAN) internal )
            {
               this->setInternal( internal ) ;
            }
         }
         else if ( EOO != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                    ele.type(), CAT_SEQUENCE_INTERNAL ) ;
            goto error ;
         }
      }

      if ( NULL != alterMask )
      {
         *alterMask = fieldMask ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_SET_OPTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_VALIDATE, "_catSequence::validate" )
   INT32 _catSequence::validate() const
   {
      INT32 rc = SDB_OK ;
      UINT64 sequenceNum = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_VALIDATE ) ;

      if ( !_catSequence::isValidName( name(), internal() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid sequence name: %s",
                 name().c_str() ) ;
         goto error ;
      }

      if ( version() < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid sequence version: %lld",
                 version() ) ;
         goto error ;
      }

      if ( minValue() >= maxValue() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "MinValue[%lld] is greater than or equals to MaxValue[%lld]",
                 minValue(), maxValue() ) ;
         goto error ;
      }

      if ( startValue() < minValue() ||
           startValue() > maxValue() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid startValue[%lld]",
                 startValue() ) ;
         goto error ;
      }

      if ( 0 == increment() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Increment cannot be zero" ) ;
         goto error ;
      }

      if ( cacheSize() <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid CacheSize[%lld]",
                 cacheSize() ) ;
         goto error ;
      }

      if ( acquireSize() <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid FetchSize[%lld]",
                 acquireSize() ) ;
         goto error ;
      }

      if ( acquireSize() > cacheSize() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "AcquireSize[%lld] is greater than CacheSize[%lld]",
                 acquireSize(), cacheSize() ) ;
         goto error ;
      }

      if ( maxValue() <= 0 || minValue() >= 0 )
      {
         UINT64 diff = (UINT64) ( maxValue() - minValue() ) ;
         sequenceNum = diff / utilAbs( increment() ) + 1 ;
      }
      else
      {
         UINT64 diff = (UINT64) maxValue() + (UINT64)( -minValue() ) ;

         sequenceNum = diff / utilAbs( increment() ) ;

         // if sequenceNum equals OSS_UINT64_MAX, plus 1 will overflow
         if ( sequenceNum != OSS_UINT64_MAX )
         {
            sequenceNum += 1 ;
         }
      }

      if ( (UINT64)cacheSize() > sequenceNum )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "CacheSize[%d] is greater than amount of sequence value[%lld]",
                 cacheSize(), sequenceNum ) ;
         goto error ;
      }

      if ( (UINT64)acquireSize() > sequenceNum )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "FetchSize[%d] is greater than amount of sequence value[%lld]",
                 acquireSize(), sequenceNum ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_VALIDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _catSequence::isValidName( const std::string& name, BOOLEAN internal )
   {
      static CHAR* invalidUserDef[] = { "$", "SYS" } ;
      static CHAR* invalidCommon[] = { " ", "\t" } ;
      const UINT32 invalidUserDefSize = sizeof( invalidUserDef ) / sizeof( CHAR* ) ;
      const UINT32 invalidCommonSize = sizeof( invalidCommon ) / sizeof( CHAR* ) ;

      if ( !internal )
      {
         for ( UINT32 i = 0; i < invalidUserDefSize; i++ )
         {
            if ( utilStrStartsWithIgnoreCase( name, invalidUserDef[i] ) )
            {
               return FALSE ;
            }
         }
      }

      for ( UINT32 i = 0; i < invalidCommonSize; i++ )
      {
         if ( utilStrStartsWith( name, invalidCommon[i] ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   INT32 _catSequence::validateFieldNames( const bson::BSONObj& options )
   {
      static std::string fieldNameArray[] = {
         CAT_SEQUENCE_NAME,
         CAT_SEQUENCE_OID,
         CAT_SEQUENCE_ID,
         CAT_SEQUENCE_VERSION,
         CAT_SEQUENCE_CURRENT_VALUE,
         CAT_SEQUENCE_INCREMENT,
         CAT_SEQUENCE_START_VALUE,
         CAT_SEQUENCE_MIN_VALUE,
         CAT_SEQUENCE_MAX_VALUE,
         CAT_SEQUENCE_CACHE_SIZE,
         CAT_SEQUENCE_ACQUIRE_SIZE,
         CAT_SEQUENCE_CYCLED,
         CAT_SEQUENCE_INTERNAL,
         CAT_SEQUENCE_INITIAL,
         CAT_SEQUENCE_NEXT_VALUE
      } ;
      static std::set<std::string> fieldNames( fieldNameArray,
         fieldNameArray + sizeof( fieldNameArray ) / sizeof( *fieldNameArray ) ) ;

      INT32 rc = SDB_OK ;
      BSONObjIterator iter( options ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         std::string fieldName = ele.fieldName() ;
         if ( fieldNames.find( fieldName ) == fieldNames.end() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Unknown sequence field: %s", fieldName.c_str() ) ;
            break ;
         }
      }

      return rc ;
   }
}

