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

   Source File Name = coordFactory.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_FACTORY_HPP__
#define COORD_FACTORY_HPP__

#include "coordOperator.hpp"
#include <string>
#include <map>

using namespace std ;

namespace engine
{

   /*
      Common define
   */
   typedef coordOperator* (*COORD_NEW_OPERATOR)() ;

   #define COORD_DECLARE_CMD_AUTO_REGISTER() \
      public: \
         static _coordOperator *newThis () ; \
         virtual const CHAR* getName() const ;

   #define COORD_IMPLEMENT_CMD_AUTO_REGISTER(theClass, className, isReadOnly) \
      _coordOperator *theClass::newThis () \
      { \
         return SDB_OSS_NEW theClass() ;\
      } \
      _coordCommandAssit theClass##Assit ( className, isReadOnly, \
            (COORD_NEW_OPERATOR)theClass::newThis ) ; \
      const CHAR* theClass::getName() const \
      { \
         return className ; \
      }


   /*
      _coordFactoryItem define
   */
   struct _coordFactoryItem
   {
      BOOLEAN                 _isReadOnly ;
      COORD_NEW_OPERATOR      _pFunc ;

      _coordFactoryItem()
      {
         _isReadOnly = FALSE ;
         _pFunc = NULL ;
      }
      _coordFactoryItem( BOOLEAN isReadOnly, COORD_NEW_OPERATOR pFunc )
      {
         _isReadOnly = isReadOnly ;
         _pFunc = pFunc ;
      }
   } ;
   typedef _coordFactoryItem coordFactoryItem ;

   /*
      _coordCommandFactory define
   */
   class _coordCommandFactory : public SDBObject
   {
      typedef _utilStringMap< coordFactoryItem, 1 > MAP_COMMAND ;
      typedef MAP_COMMAND::iterator                 MAP_COMMAND_IT ;

      friend class _coordCommandAssit ;

      public:
         _coordCommandFactory() ;
         ~_coordCommandFactory() ;

      public:
         INT32          create( const CHAR *pCmdName,
                                coordOperator *&pOperator ) ;
         void           release( coordOperator *pOperator ) ;

      protected:
         INT32          _register( const CHAR *pCmdName,
                                   BOOLEAN isReadOnly,
                                   COORD_NEW_OPERATOR pFunc ) ;

      private:
         MAP_COMMAND          _mapCommand ;

   } ;
   typedef _coordCommandFactory coordCommandFactory ;

   coordCommandFactory* coordGetFactory() ;

   /*
      _coordCommandAssit define
   */
   class _coordCommandAssit
   {
      public:
         _coordCommandAssit( const CHAR *pCmdName,
                             BOOLEAN isReadOnly,
                             COORD_NEW_OPERATOR pFunc ) ;
         _coordCommandAssit( COORD_NEW_OPERATOR pFunc ) ;
         ~_coordCommandAssit() ;
   } ;
   typedef _coordCommandAssit coordCommandAssit ;

}

#endif // COORD_FACTORY_HPP__
