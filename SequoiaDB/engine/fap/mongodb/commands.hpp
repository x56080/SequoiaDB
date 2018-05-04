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

   Source File Name = commands.hpp

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
#ifndef _SDB_FAP_MONGO_COMMANDS_HPP_
#define _SDB_FAP_MONGO_COMMANDS_HPP_

#include "baseCommand.hpp"

#define __DECLARE_COMMAND( cmd, secondName, clsName )                \
class clsName : public baseCommand                                   \
{                                                                    \
public:                                                              \
   clsName() : baseCommand( cmd, secondName )                        \
   {}                                                                \
                                                                     \
   virtual INT32 convert( msgParser &parser ) ;                      \
                                                                     \
   virtual INT32 buildMsg( msgParser &parser, msgBuffer &sdbMsg ) ;  \
                                                                     \
   virtual INT32 doCommand( void *pData = NULL ) ;                   \
} ;

#define __DECLARE_COMMAND_VAR( commandClass, var )                \
      commandClass var ;

#define DECLARE_COMMAND( command )                                \
      __DECLARE_COMMAND( #command, NULL, command##Command )

#define DECLARE_COMMAND_ALAIS( command, secondName )              \
      __DECLARE_COMMAND( #command, secondName, command##Command)

#define DECLARE_COMMAND_VAR( command )                            \
      __DECLARE_COMMAND_VAR( command##Command, command##Cmd )

///////////////////////////////////////////////////////////////////
// declare all command supported

DECLARE_COMMAND( insert )
DECLARE_COMMAND( delete )
DECLARE_COMMAND( update )
DECLARE_COMMAND( query )
DECLARE_COMMAND( getMore )
DECLARE_COMMAND( killCursors )

// other command
DECLARE_COMMAND( getnonce )
DECLARE_COMMAND( authenticate )
DECLARE_COMMAND( createUser )
DECLARE_COMMAND( dropUser )
DECLARE_COMMAND( listUsers )
DECLARE_COMMAND( create )
DECLARE_COMMAND( createCS )
//DECLARE_COMMAND( createCollection )
DECLARE_COMMAND( listCollection )
DECLARE_COMMAND( drop )
DECLARE_COMMAND( count )
DECLARE_COMMAND( aggregate )
DECLARE_COMMAND( dropDatabase )
DECLARE_COMMAND( createIndexes )
DECLARE_COMMAND_ALAIS( deleteIndexes, "dropIndexes")
DECLARE_COMMAND( listIndexes )
DECLARE_COMMAND( getlasterror )
DECLARE_COMMAND_ALAIS( ismaster, "isMaster" )
DECLARE_COMMAND( ping )
DECLARE_COMMAND( logout )

#endif
