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

   Source File Name = dmsTransContext.hpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_TRANS_CONTEXT_HPP__
#define DMS_TRANS_CONTEXT_HPP__

#include "sdbInterface.hpp"
#include "dms.hpp"

using namespace bson ;

namespace engine
{

   class _dmsMBContext ;
   class _pmdEDUCB ;

   #define DMS_TRNCTX_PAUSE_FLG_NONE            ( 0      )
   #define DMS_TRNCTX_PAUSE_FLG_IXADVONHOLD     ( 0xF    )
   #define DMS_TRNCTX_PAUSE_FLG_UNLOCKALLEXT    ( 0xF0   )
   #define DMS_TRNCTX_PAUSE_FLG_IXMCTXONLY      ( 0xF00  )
   #define DMS_TRNCTX_PAUSE_FLG_KEEPMBLATCH     ( 0xF000 )

   /*
      _dmsTBTransContext define
   */
   class _dmsTBTransContext : public _IContext
   {
      public:
         _dmsTBTransContext( _dmsMBContext *pMBContext,
                             DMS_ACCESS_TYPE accessType,
                             _pmdEDUCB *cb ) ;
         virtual ~_dmsTBTransContext() ;
         UINT32   getPauseFlag() ;
         void     setPauseFlag( UINT32 flag ) ;
         void     resetPauseFlag() ;

      protected:
         INT32    _checkAccess() ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume( UINT8 whom ) ;

      protected:
         _dmsMBContext         * _pMBContext ;
         DMS_ACCESS_TYPE         _accessType ;
         UINT32                  _pauseFlag ;
         _pmdEDUCB             * _cb ;
   } ;
   typedef _dmsTBTransContext dmsTBTransContext ;

   class _rtnIXScanner ;
   class _pmdEDUCB ;

   /*
      _dmsIXTransContext define
   */
   class _dmsIXTransContext : public _dmsTBTransContext
   {
      public:
         _dmsIXTransContext( _dmsMBContext *pMBContext,
                             DMS_ACCESS_TYPE accessType,
                             _rtnIXScanner *pScanner,
                             _pmdEDUCB *cb ) ;
         virtual ~_dmsIXTransContext() ;

         BOOLEAN  isCursorSame() const ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume( UINT8 whom = ICTX_RESUME_ALL ) ;

      protected:
         _rtnIXScanner         * _pScanner ;
         BOOLEAN                 _isReadonly ;
         BOOLEAN                 _isSame ;
   } ;
   typedef _dmsIXTransContext dmsIXTransContext ;

}

#endif /* DMS_TRANS_CONTEXT_HPP__ */

