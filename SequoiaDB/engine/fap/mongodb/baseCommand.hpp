/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = baseCommand.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_FAP_MONGO_BASE_COMMAND_HPP_
#define _SDB_FAP_MONGO_BASE_COMMAND_HPP_

#include <map>
#include "util.hpp"
#include "mongodef.hpp"
#include "parser.hpp"

class baseCommand : public SDBObject
{
public:
   baseCommand( const CHAR *name, const CHAR *secondName = NULL ) ;

   virtual ~baseCommand() {} ;

   const CHAR *name() const
   {
      return _name ;
   }

   virtual INT32 convert( msgParser &parser )
   {
      return SDB_OK ;
   }

   virtual INT32 buildMsg( msgParser &parser, msgBuffer &sdbMsg )
   {
      return SDB_OK ;
   }

   virtual INT32 doCommand( void *pData = NULL )
   {
      return SDB_OK ;
   }

private:
   const CHAR *_name ;
} ;


class commandMgr : public SDBObject
{
public:
   static commandMgr *instance() ;

   void addCommand( const std::string &name, baseCommand *cmd )
   {
      baseCommand *tmp = _cmdMap[name] ;
      if ( NULL != tmp )
      {
         // should never hit here!
      }
      _cmdMap[name] = cmd ;
   }

   baseCommand *findCommand( const std::string &name )
   {
      baseCommand *cmd = NULL ;
      std::map< std::string, baseCommand* >::iterator it = _cmdMap.find( name ) ;
      if ( _cmdMap.end() != it )
      {
         cmd = it->second ;
      }
      return cmd ;
   }

private:

   commandMgr()
   {
      _cmdMap.clear() ;
   }

   ~commandMgr()
   {
      _cmdMap.clear() ;
   }

private:
   std::map< std::string, baseCommand* > _cmdMap ;
} ;


#endif
