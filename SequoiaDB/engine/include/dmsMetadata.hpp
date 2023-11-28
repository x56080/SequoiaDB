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

   Source File Name = dmsMetadata.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_METADATA_HPP_
#define SDB_DMS_METADATA_HPP_

#include "dms.hpp"
#include "dmsEngineDef.hpp"
#include "msgDef.h"
#include "utilCompression.hpp"
#include "utilRecycleItem.hpp"
#include "utilRenameLogger.hpp"
#include "utilResult.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   // forward declaration
   class _dmsSUDescriptor ;
   class _dmsMetadataBlock ;
   class _dmsMBStatInfo ;
   class _ixmIndexCB ;

   /*
      _dmsCLMetadataKey define
    */
   class _dmsCLMetadataKey : public SDBObject
   {
   public:
      _dmsCLMetadataKey() = default ;
      ~_dmsCLMetadataKey() = default ;
      _dmsCLMetadataKey( const _dmsCLMetadataKey &o ) = default ;
      _dmsCLMetadataKey &operator =( const _dmsCLMetadataKey & ) = default ;

      _dmsCLMetadataKey( utilCLUniqueID clUID,
                         UINT32 clLID )
      : _clOrigUID( clUID ),
        _clOrigLID( clLID )
      {
      }

      _dmsCLMetadataKey( _dmsMetadataBlock *mb ) ;

      BOOLEAN operator ==( const _dmsCLMetadataKey &o ) const
      {
         return _clOrigUID == o._clOrigUID && _clOrigLID == o._clOrigLID ;
      }

      BOOLEAN operator <( const _dmsCLMetadataKey &o ) const
      {
         if ( _clOrigUID < o._clOrigUID )
         {
            return TRUE ;
         }
         else if ( _clOrigUID > o._clOrigUID )
         {
            return FALSE ;
         }
         return _clOrigLID < o._clOrigLID ;
      }

      utilCLUniqueID getCLOrigUID() const
      {
         return _clOrigUID ;
      }

      void setCLOrigUID( utilCLUniqueID origUID )
      {
         _clOrigUID = origUID ;
      }

      UINT32 getCLOrigLID() const
      {
         return _clOrigLID ;
      }

      void setCLOrigLID( UINT32 origLID )
      {
         _clOrigLID = origLID ;
      }

      BOOLEAN isValid() const
      {
         return UTIL_UNIQUEID_NULL != _clOrigUID &&
                DMS_INVALID_LOGICCLID != _clOrigLID ;
      }

   protected:
      utilCLUniqueID _clOrigUID = UTIL_UNIQUEID_NULL ;
      UINT32 _clOrigLID = DMS_INVALID_LOGICCLID ;
   } ;

   typedef class _dmsCLMetadataKey dmsCLMetadataKey ;

   /*
      _dmsIdxMetadataKey define
    */
   class _dmsIdxMetadataKey : public _dmsCLMetadataKey
   {
   public:
      _dmsIdxMetadataKey() = default ;
      ~_dmsIdxMetadataKey() = default ;
      _dmsIdxMetadataKey( const _dmsIdxMetadataKey &o ) = default ;
      _dmsIdxMetadataKey &operator =( const _dmsIdxMetadataKey & ) = default ;

      _dmsIdxMetadataKey( utilCLUniqueID clUID,
                          UINT32 clLID,
                          utilIdxInnerID idxInnerID )
      : _dmsCLMetadataKey( clUID, clLID ),
         _idxInnerID( idxInnerID )
      {
      }

      _dmsIdxMetadataKey( _dmsMetadataBlock *mb,
                          _ixmIndexCB *idxCB ) ;

      BOOLEAN operator ==( const _dmsIdxMetadataKey &o ) const
      {
         return ( _dmsCLMetadataKey::operator ==( o ) ) &&
                ( _idxInnerID == o._idxInnerID ) ;
      }

      BOOLEAN operator <( const _dmsIdxMetadataKey &o ) const
      {
         if ( _dmsCLMetadataKey::operator <( o ) )
         {
            return TRUE ;
         }
         else if ( _dmsCLMetadataKey::operator ==( o ) )
         {
            return _idxInnerID < o._idxInnerID ;
         }
         return FALSE ;
      }

      utilIdxInnerID getIdxInnerID() const
      {
         return _idxInnerID ;
      }

      void setIdxInnerID( utilIdxInnerID idxInnerID )
      {
         _idxInnerID = idxInnerID ;
      }

      BOOLEAN isValid() const
      {
         return _dmsCLMetadataKey::isValid() &&
                UTIL_UNIQUEID_NULL != _idxInnerID ;
      }

   protected:
      utilIdxInnerID _idxInnerID = UTIL_UNIQUEID_NULL ;
   } ;

   typedef class _dmsIdxMetadataKey dmsIdxMetadataKey ;

   /*
      _dmsCSMetadata define
    */
   class _dmsCSMetadata : public SDBObject
   {
   public:
      _dmsCSMetadata() = default ;
      virtual ~_dmsCSMetadata() = default ;
      _dmsCSMetadata( const _dmsCSMetadata &o ) = default ;
      _dmsCSMetadata &operator =( const _dmsCSMetadata & ) = default ;

      _dmsCSMetadata( utilCSUniqueID csUID )
      : _csUID( csUID )
      {
      }

      _dmsCSMetadata( _dmsSUDescriptor *su ) ;

      utilCSUniqueID getCSUID() const
      {
         return _csUID ;
      }

      void setCSUID( utilCSUniqueID csUID )
      {
         _csUID = csUID ;
      }

      _dmsSUDescriptor *getSU() const
      {
         return _su ;
      }

      void setSU( _dmsSUDescriptor *su )
      {
         _su = su ;
      }

   protected:
      _dmsSUDescriptor *_su = nullptr ;
      utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL ;
   } ;

   typedef class _dmsCSMetadata dmsCSMetadata ;

   /*
      _dmsCLMetadata define
    */
   class _dmsCLMetadata : public _dmsCSMetadata
   {
   public:
      _dmsCLMetadata() = default ;
      virtual ~_dmsCLMetadata() = default ;
      _dmsCLMetadata( const _dmsCLMetadata &o ) = default ;
      _dmsCLMetadata &operator =( const _dmsCLMetadata & ) = default ;

      _dmsCLMetadata( utilCLUniqueID clUID,
                      UINT32 clLID,
                      utilCLInnerID clOrigInnerUID,
                      UINT32 clOrigLID )
      : _dmsCSMetadata( utilGetCSUniqueID( clUID ) ),
        _clInnerID( utilGetCLInnerID( clUID ) ),
        _clLID( clLID ),
        _clOrigInnerID( clOrigInnerUID ),
        _clOrigLID( clOrigLID )
      {
      }

      _dmsCLMetadata( _dmsSUDescriptor *su,
                      _dmsMetadataBlock *mb,
                      _dmsMBStatInfo *mbStat ) ;

      utilCLInnerID getCLInnerID() const
      {
         return _clInnerID ;
      }

      void setCLInnerID( utilCLInnerID clUID )
      {
         _clInnerID = clUID ;
      }

      UINT32 getCLLID() const
      {
         return _clLID ;
      }

      void setCLLID( UINT32 clLID )
      {
         _clLID = clLID ;
      }

      utilCLInnerID getCLOrigInnerID() const
      {
         return _clOrigInnerID ;
      }

      void setCLOrigInnerID( utilCLInnerID clOrigInnerID )
      {
         _clOrigInnerID = clOrigInnerID ;
      }

      UINT32 getCLOrigLID() const
      {
         return _clOrigLID ;
      }

      void setCLOrigLID( UINT32 clOrigLID )
      {
         _clOrigLID = clOrigLID ;
      }

      utilCLUniqueID getCLUID() const
      {
         return utilBuildCLUniqueID( getCSUID(), getCLInnerID() ) ;
      }

      utilCLUniqueID getOrigUID() const
      {
         return utilBuildCLUniqueID( getCSUID(), getCLOrigInnerID() ) ;
      }

      _dmsMetadataBlock *getMB()
      {
         return _mb ;
      }

      void setMB( _dmsMetadataBlock *mb )
      {
         _mb = mb ;
      }

      _dmsMBStatInfo *getMBStat()
      {
         return _mbStat ;
      }

      void setMBStat( _dmsMBStatInfo *mbStat )
      {
         _mbStat = mbStat ;
      }

      dmsCLMetadataKey getKey() const
      {
         return dmsCLMetadataKey( getOrigUID(), getCLLID() ) ;
      }

   protected:
      _dmsMetadataBlock *_mb = nullptr ;
      _dmsMBStatInfo *_mbStat = nullptr ;
      utilCLInnerID _clInnerID = UTIL_UNIQUEID_NULL ;
      UINT32 _clLID = DMS_INVALID_LOGICCLID ;
      utilCLInnerID _clOrigInnerID = UTIL_UNIQUEID_NULL ;
      UINT32 _clOrigLID = DMS_INVALID_LOGICCLID ;
   } ;

   typedef class _dmsCLMetadata dmsCLMetadata ;

   /*
      _dmsIdxMetadata define
    */
   class _dmsIdxMetadata : public _dmsCLMetadata
   {
   public:
      _dmsIdxMetadata() = default ;
      virtual ~_dmsIdxMetadata() = default ;
      _dmsIdxMetadata( const _dmsIdxMetadata &o ) = default ;
      _dmsIdxMetadata &operator =( const _dmsIdxMetadata & ) = default ;

      _dmsIdxMetadata( utilCLUniqueID clUID,
                       utilIdxUniqueID idxUID,
                       UINT32 clLID,
                       UINT32 idxLID,
                       utilCLInnerID clOrigInnerUID,
                       UINT32 clOrigLID )
      : _dmsCLMetadata( clUID, clLID, clOrigInnerUID, clOrigLID ),
        _idxInnerID( utilGetIdxInnerID( idxUID ) ),
        _idxLID( idxLID )
      {
      }

      _dmsIdxMetadata( _dmsSUDescriptor *su,
                       _dmsMetadataBlock *mb,
                       _dmsMBStatInfo *mbStat,
                       utilIdxUniqueID idxUID,
                       UINT32 idxLID ) ;

      utilIdxInnerID getIdxInnerID() const
      {
         return _idxInnerID ;
      }

      void setIdxInnerID( utilIdxInnerID idxInnerID )
      {
         _idxInnerID = idxInnerID ;
      }

      UINT32 getIdxLID() const
      {
         return _idxLID ;
      }

      void setIdxLID( UINT32 idxLID )
      {
         _idxLID = idxLID ;
      }

      utilIdxUniqueID getIdxUID() const
      {
         return utilBuildIdxUniqueID( getCSUID(), getIdxInnerID() ) ;
      }

      void setIdxUID( utilIdxUniqueID idxUID )
      {
         _idxInnerID = utilGetIdxInnerID( idxUID ) ;
      }

   protected:
      utilIdxInnerID _idxInnerID = UTIL_UNIQUEID_NULL ;
      UINT32 _idxLID = DMS_INVALID_EXTENT ;
   } ;

   typedef class _dmsIdxMetadata dmsIdxMetadata ;

} // namespace engine

#endif // SDB_DMS_METADATA_HPP_
