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

   Source File Name = rtnCoordList.hpp

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
#ifndef RTNCOORD_LIST_HPP__
#define RTNCOORD_LIST_HPP__

#include "rtnCoordCommands.hpp"

using namespace bson ;

namespace engine
{

   /*
      rtnCoordCMDListIntrBase define
   */
   class rtnCoordCMDListIntrBase : public rtnCoordCMDMonIntrBase
   {
   private:
      virtual BOOLEAN _useContext() { return TRUE ; }
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) {}
      virtual UINT32  _getControlMask() const { return RTN_CTRL_MASK_ALL ; }

   } ;

   /*
      rtnCoordCMDListCurIntrBase define
   */
   class rtnCoordCMDListCurIntrBase : public rtnCoordCMDMonCurIntrBase
   {
   private:
      virtual BOOLEAN _useContext() { return TRUE ; }
      virtual UINT32  _getControlMask() const { return RTN_CTRL_MASK_ALL ; }
   } ;

   /*
      rtnCoordListTransCurIntr define
   */
   class rtnCoordListTransCurIntr : public rtnCoordCMDListIntrBase
   {
   private:
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   /*
      rtnCoordListTransIntr define
   */
   class rtnCoordListTransIntr : public rtnCoordCMDListIntrBase
   {
   private:
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   /*
      rtnCoordListTransCur define
   */
   class rtnCoordListTransCur : public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   } ;

   /*
      rtnCoordListTrans define
   */
   class rtnCoordListTrans : public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   } ;

   /*
      rtnCoordListBackupIntr define
   */
   class rtnCoordListBackupIntr : public rtnCoordCMDListIntrBase
   {
   private:
      virtual void    _preSet( pmdEDUCB *cb, rtnCoordCtrlParam &ctrlParam ) ;
   } ;

   /*
      rtnCoordListBackup define
   */
   class rtnCoordListBackup : public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   } ;

   /*
      rtnCoordCMDListGroups define
   */
   class rtnCoordCMDListGroups : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListCollectionSpace define
   */
   class rtnCoordCMDListCollectionSpace : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListCollectionSpaceIntr define
   */
   class rtnCoordCMDListCollectionSpaceIntr : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListCollection define
   */
   class rtnCoordCMDListCollection : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListCollectionIntr define
   */
   class rtnCoordCMDListCollectionIntr : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListContexts define
   */
   class rtnCoordCMDListContexts: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDListContextsCur define
   */
   class rtnCoordCMDListContextsCur: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDListSessions define
   */
   class rtnCoordCMDListSessions: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDListSessionsCur define
   */
   class rtnCoordCMDListSessionsCur: public rtnCoordCMDMonBase
   {
   private:
      virtual const CHAR *getIntrCMDName() ;
      virtual const CHAR *getInnerAggrContent() ;
   };

   /*
      rtnCoordCMDListUser define
   */
   class rtnCoordCMDListUser : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCmdListTask define
   */
   class rtnCoordCmdListTask : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListProcedures define
   */
   class rtnCoordCMDListProcedures : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListDomains define
   */
   class rtnCoordCMDListDomains : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListCSInDomain define
   */
   class rtnCoordCMDListCSInDomain : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   /*
      rtnCoordCMDListCLInDomain define
   */
   class rtnCoordCMDListCLInDomain : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      INT32 _rebuildListResult( const std::vector<BSONObj> &infoFromCata,
                                pmdEDUCB *cb,
                                SINT64 &contextID ) ;
   } ;

   /*
      rtnCoordCMDListLobs define
   */
   class rtnCoordCMDListLobs : public rtnCoordCommand
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

}

#endif // RTNCOORD_LIST_HPP__
