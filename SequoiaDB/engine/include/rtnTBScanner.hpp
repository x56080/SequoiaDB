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

   Source File Name = rtnTBScanner.hpp

   Descriptive Name = RunTime Table Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_TB_SCANNER_HPP__
#define RTN_TB_SCANNER_HPP__

#include "rtnScanner.hpp"
#include "interface/ICursor.hpp"

namespace engine
{

   /*
      _rtnTBScanner define
    */
   class _rtnTBScanner : public _rtnScanner
   {
   public:
      _rtnTBScanner( _dmsStorageUnit  *su,
                     _dmsMBContext    *mbContext,
                     const dmsRecordID &startRID,
                     BOOLEAN           isAfterStartRID,
                     INT32             direction,
                     _pmdEDUCB        *cb ) ;
      virtual ~_rtnTBScanner() ;

   public:
      BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 advance( dmsRecordID &rid ) ;
      INT32 getCurrentRID( dmsRecordID &nextRID ) ;
      INT32 getCurrentRecord( dmsRecordData &recordData ) ;

   protected:
      dmsRecordID _startRID ;
      BOOLEAN _isAfterStartRID ;

      std::unique_ptr<IDataCursor> _cursorPtr ;
   } ;

   typedef class _rtnTBScanner rtnTBScanner ;

}

#endif // RTN_TB_SCANNER_HPP__