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

   Source File Name = catCommandNode.hpp

   Descriptive Name = Catalogue commands for node

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for catalog
   commands.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/18/2022  LCX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_COMMAND_NODE_HPP__
#define CAT_COMMAND_NODE_HPP__

#include "catCMDBase.hpp"
#include "catNodeManager.hpp"

namespace engine
{
   /*
      _catCMDAlterNode define
   */
   class _catCMDAlterNode : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDAlterNode() ;
      virtual ~_catCMDAlterNode() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_ALTER_NODE ;
      }

   private:
      INT32                  _initNodeInfo( _pmdEDUCB *cb ) ;
   
   private:
      UINT32                 _groupID ;
      UINT16                 _nodeID ;
      string                 _groupName ;
      string                 _nodeName ;
      ossPoolString          _actionName ;
      BSONObj                _option ;
      BSONObj                _nodeObj ;
      catNodeManager         *_pCatNodeMgr ;
   } ;

   typedef class _catCMDAlterNode catCMDAlterNode ;

   /*
      _catCMDAlterRG define
   */
   class _catCMDAlterRG : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDAlterRG() ;
      virtual ~_catCMDAlterRG() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHiant = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_ALTER_GROUP ;
      }

   private:
      BSONObj _queryObj ;
      BSONObj _hintObj ;
   } ;

   typedef class _catCMDAlterRG catCMDAlterRG ;

}

#endif // CAT_COMMAND_NODE_HPP__