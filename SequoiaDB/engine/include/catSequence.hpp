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

   Source File Name = catSequence.hpp

   Descriptive Name = Sequence definition

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_SEQUENCE_HPP_
#define CAT_SEQUENCE_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "../bson/bson.hpp"
#include "utilGlobalID.hpp"
#include "utilUniqueID.hpp"
#include <string>

namespace engine
{
   class _catSequence: public SDBObject
   {
   public:
      _catSequence( const std::string& name ) ;
      ~_catSequence() ;

   public:
      OSS_INLINE const std::string& getName() const { return _name ; }
      OSS_INLINE const bson::OID& getOID() const { return _oid ; }
      OSS_INLINE BOOLEAN isInternal() const { return _internal ; }
      OSS_INLINE INT64 getVersion() const { return _version ; }
      OSS_INLINE INT64 getCachedValue() const { return _cachedValue ; }
      OSS_INLINE INT64 getCurrentValue() const { return _currentValue ; }
      OSS_INLINE INT64 getStartValue() const { return _startValue ; }
      OSS_INLINE INT64 getMinValue() const { return _minValue ; }
      OSS_INLINE INT64 getMaxValue() const { return _maxValue ; }
      OSS_INLINE INT32 getIncrement() const { return _increment ; }
      OSS_INLINE INT32 getCacheSize() const { return _cacheSize ; }
      OSS_INLINE INT32 getAcquireSize() const { return _acquireSize ; }
      OSS_INLINE BOOLEAN isCycled() const { return _cycled ; }
      OSS_INLINE INT32 getCycledCount() const { return _cycledCount ; }
      OSS_INLINE BOOLEAN isInitial() const { return _initial ; }
      OSS_INLINE BOOLEAN isExceeded() const { return _exceeded ; }
      OSS_INLINE utilSequenceID getID() const { return _ID ; }
      OSS_INLINE utilCLUniqueID getCLUniqueID() const { return _clUniqueID ; }

      OSS_INLINE void lock()
      {
         _latch.get() ;
      }

      OSS_INLINE void unlock()
      {
         _latch.release() ;
      }

      void setOID( const bson::OID& oid ) ;
      void setInternal( BOOLEAN internal ) ;
      void setCachedValue( INT64 cachedValue ) ;
      void setVersion( INT64 version ) ;
      void increaseVersion() ;
      void setCurrentValue( INT64 currentValue ) ;
      void setStartValue( INT64 startValue ) ;
      void setMinValue( INT64 minValue ) ;
      void setMaxValue( INT64 maxValue ) ;
      void setIncrement( INT32 increment ) ;
      void setCacheSize( INT32 cacheSize ) ;
      void setAcquireSize( INT32 acquireSize ) ;
      void setCycled( BOOLEAN cycled ) ;
      void setCycledCount( INT32 cycledCount ) ;
      void increaseCycledCount() ;
      void setInitial( BOOLEAN initial ) ;
      void setExceeded( BOOLEAN exceeded ) ;
      void setID( utilGlobalID id ) ;
      void setCLUniqueID( utilCLUniqueID clUniqueID ) ;
      INT32 toBSONObj( bson::BSONObj& obj, BOOLEAN forUpdate ) const ;
      void copyFrom( const _catSequence& other, BOOLEAN withInternalField = TRUE ) ;
      INT32 setOptions ( const bson::BSONObj & options,
                         BOOLEAN isFirstInitial,
                         BOOLEAN withInternalFields,
                         UINT32 * alterMask = NULL ) ;
      INT32 loadOptions ( const bson::BSONObj & options ) ;
      INT32 validate() const ;

   public:
      static BOOLEAN isValidName( const std::string& name, BOOLEAN internal ) ;
      static INT32 validateFieldNames( const bson::BSONObj& options ) ;

   protected :
      INT32 _loadOptions ( const bson::BSONObj & options,
                           BOOLEAN isFirstInitial,
                           BOOLEAN withInternalFields,
                           UINT32 & fieldMask ) ;
      void _checkExceeded () ;

   private:
      std::string    _name ;           // sequence name
      utilCLUniqueID _clUniqueID ;     // unique ID of collection
      bson::OID      _oid ;            // sequence oid
      BOOLEAN        _internal ;       // system internal defined
      INT64          _version ;        // sequence version
      INT64          _cachedValue ;    // cached current value, only in cache
      INT64          _currentValue ;   // current value
      INT64          _startValue ;     // start value
      INT64          _minValue ;       // minimum value
      INT64          _maxValue ;       // maxinum value
      INT32          _increment ;      // increament value
      INT32          _cacheSize ;      // cache size in Catalog
      INT32          _acquireSize ;    // acquire size in Coordinator
      BOOLEAN        _cycled ;         // true if cycle is allowed
      INT32          _cycledCount ;    // cycled count
      BOOLEAN        _initial ;        // sequence is unused
      BOOLEAN        _exceeded ;       // sequence is exceeded, only in cache
      utilSequenceID _ID ;             // sequence id
      ossSpinXLatch  _latch ;
   } ;
   typedef _catSequence catSequence ;
}

#endif /* CAT_SEQUENCE_HPP_ */

