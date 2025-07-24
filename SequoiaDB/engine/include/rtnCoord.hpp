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

   Source File Name = rtnCoord.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNCOORD_HPP__
#define RTNCOORD_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "msg.hpp"
#include "netMultiRouteAgent.hpp"
#include <map>

namespace engine
{

   class rtnCoordCommand;
   class rtnCoordOperator;

   #define RTN_COORD_CMD_BEGIN  void rtnCoordProcesserFactory::addCommand(){
   #define RTN_COORD_CMD_END     }
   #define RTN_COORD_CMD_ADD( cmdName, cmdClass, readonly ) \
      do { \
         rtnCoordCommand *pObj = SDB_OSS_NEW cmdClass(); \
         SDB_ASSERT( pObj, "Alloc memory failed" ) ; \
         if ( pObj ) \
         { \
            pObj->_isReadonly = readonly ; \
            _cmdMap.insert ( COORD_CMD_MAP::value_type (cmdName, pObj ) ) ; \
         } \
      } while( 0 )

   #define RTN_COORD_OP_BEGIN    void rtnCoordProcesserFactory::addOperator(){
   #define RTN_COORD_OP_END      }
   #define RTN_COORD_OP_ADD( opCode, opClass, readonly ) \
      do { \
         rtnCoordOperator *pObj = SDB_OSS_NEW opClass() ; \
         SDB_ASSERT( pObj, "Alloc memory failed" ) ; \
         if ( pObj ) \
         { \
            pObj->_isReadonly = readonly ; \
            _opMap.insert ( COORD_OP_MAP::value_type ( opCode, pObj ) ) ; \
         } \
      } while( 0 )

   /*
      rtnCoordProcesserFactory define
   */
   class rtnCoordProcesserFactory : public SDBObject
   {
   typedef std::map<std::string, rtnCoordCommand *> COORD_CMD_MAP;
   typedef std::map<SINT32, rtnCoordOperator *> COORD_OP_MAP;
   public:
      rtnCoordProcesserFactory();
      ~rtnCoordProcesserFactory();
      rtnCoordCommand *getCommandProcesser(const MsgOpQuery *pQuery);
      rtnCoordCommand *getCommandProcesser(const char *pCmd);
      rtnCoordOperator *getOperator( SINT32 opCode );
   private:
      void addCommand();
      void addOperator();
   private:
      COORD_CMD_MAP        _cmdMap;
      COORD_OP_MAP         _opMap;
   };

}

#endif // RTNCOORD_HPP__
