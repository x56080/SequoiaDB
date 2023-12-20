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

   Source File Name = rtnScanner.hpp

   Descriptive Name = RunTime Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_SCANNER_HPP__
#define RTN_SCANNER_HPP__

#include "oss.hpp"
#include "dms.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   // forward declaration
   class _dmsStorageUnit ;
   class _dmsMBContext ;
   class _pmdEDUCB ;

   /*
      _rtnScanner define
    */
   class _rtnScanner : public _utilPooledObject
   {
   public:
      _rtnScanner( _dmsStorageUnit  *su,
                   _dmsMBContext    *mbContext,
                   INT32             direction,
                   _pmdEDUCB        *cb )
      : _su( su ),
        _mbContext( mbContext ),
        _direction( direction ),
        _isEOF( FALSE ),
        _cb( cb )
      {
      }

      virtual ~_rtnScanner()
      {
      }

   public:
      BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 advance( dmsRecordID &rid ) = 0 ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) = 0 ;
      virtual INT32 pauseScan() = 0 ;

      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) = 0 ;

   protected:
      _dmsStorageUnit *_su ;
      _dmsMBContext *_mbContext ;
      INT32 _direction ;
      BOOLEAN _isEOF ;
      _pmdEDUCB *_cb ;
   } ;

   typedef class _rtnScanner rtnScanner ;

}

#endif // RTN_SCANNER_HPP__