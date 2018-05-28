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
