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

   Source File Name = rtnCoordSnapshot.hpp

   Descriptive Name = Runtime Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09-23-2016  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef RTNCOORD_SNAPSHOT_HPP__
#define RTNCOORD_SNAPSHOT_HPP__

#include "rtnCoordCommands.hpp"

using namespace bson ;

namespace engine
{

   /*
      rtnCoordCMDSnapshotIntrBase define
   */
   class rtnCoordCMDSnapshotIntrBase : public rtnCoordCMDMonIntrBase
   {
   private:
      virtual BOOLEAN _useContext() { return TRUE ; }
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) {}
      virtual UINT32  _getControlMask() const { return RTN_CTRL_MASK_ALL ; }

   } ;

   /*
      rtnCoordCMDSnapshotIntrCurBase define
   */
   class rtnCoordCMDSnapshotCurIntrBase : public rtnCoordCMDMonCurIntrBase
   {
   private:
      virtual BOOLEAN _useContext() { return TRUE ; }
      virtual UINT32  _getControlMask() const { return RTN_CTRL_MASK_ALL ; }
   } ;

   /*
      rtnCoordCmdSnapshotReset define
   */
   class rtnCoordCmdSnapshotReset : public rtnCoordCMDSnapshotIntrBase
   {
   private:
      virtual BOOLEAN _useContext() { return FALSE ; }
   } ;

   /*
      rtnCoordSnapshotTransCurIntr define
   */
   class rtnCoordSnapshotTransCurIntr : public rtnCoordCMDSnapshotIntrBase
   {
   private:
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   /*
      rtnCoordSnapshotTransIntr define
   */
   class rtnCoordSnapshotTransIntr : public rtnCoordCMDSnapshotIntrBase
   {
   private:
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   /*
      rtnCoordSnapshotTransCur define
   */
   class rtnCoordSnapshotTransCur : public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   } ;

   /*
      rtnCoordSnapshotTrans define
   */
   class rtnCoordSnapshotTrans : public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   } ;

   /*
      rtnCoordCMDSnapshotDataBase define
   */
   class rtnCoordCMDSnapshotDataBase: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotSystem define
   */
   class rtnCoordCMDSnapshotSystem: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotCollections define
   */
   class rtnCoordCMDSnapshotCollections: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotSpaces define
   */
   class rtnCoordCMDSnapshotSpaces: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotContexts define
   */
   class rtnCoordCMDSnapshotContexts: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotContextsCur define
   */
   class rtnCoordCMDSnapshotContextsCur: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotSessions define
   */
   class rtnCoordCMDSnapshotSessions: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotSessionsCur define
   */
   class rtnCoordCMDSnapshotSessionsCur: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDSnapshotCata define
   */
   class rtnCoordCMDSnapshotCata : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   };

}
#endif // RTNCOORD_SNAPSHOT_HPP__
