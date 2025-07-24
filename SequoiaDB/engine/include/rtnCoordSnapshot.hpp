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
