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

   Source File Name = monMgr.hpp

   Descriptive Name = Monitor Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MONMGR_HPP_
#define MONMGR_HPP_

#include <vector>
#include "monClass.hpp"

namespace engine
{

/**
 * A monitor manager responsible for memory management and metric management
 * of the monitor classes
 */
class _monMonitorManager
{
   // disable assignment/copy
   _monMonitorManager& operator= (const _monMonitorManager&) ;
   _monMonitorManager(_monMonitorManager&) ;

public:
   _monMonitorManager() ;

   ~_monMonitorManager() ;

   /**
    * Register a new monitor object
    *
    * @param classType the type of monitor object getting registered
    * @return T a pointer to the new monitor object
    */
   template<class T>
   T* registerMonitorObject()
   {
      //TODO need to verify T is a subclass of monClass
      MON_CLASS_TYPE classType = T::getType() ;
      T* ptr = _monClass[classType]->add<T>();
      return ptr ;
   }

   /**
    * Register a new monitor object
    *
    * @param classType the type of monitor object getting registered
    * @return T a pointer to the new monitor object
    */
   template<class T>
   T* registerMonitorObject(_monClassBaseData *data)
   {
      //TODO need to verify T is a subclass of monClass
      MON_CLASS_TYPE classType = T::getType() ;
      T* ptr = _monClass[classType]->add<T>(data);

      return ptr ;
   }

   /**
    * Register a new monitor object
    *
    * @param classType the type of monitor object getting registered
    * @return T a pointer to the new monitor object
    */
   template<class T>
   T* registerMonitorObject( const T& data )
   {
      //TODO need to verify T is a subclass of monClass
      MON_CLASS_TYPE classType = T::getType() ;
      T* ptr = _monClass[classType]->add<T>(data);

      return ptr ;
   }

   /**
    * Remove a monitor object.
    *
    * This will lookup the object type and remove the object from the
    * appropriate container
    *
    * @param obj the object to be removed
    */
   void removeMonitorObject(monClass* obj)
   {
      SDB_ASSERT ( NULL != obj, "removing a NULL monitor object" ) ;
      MON_CLASS_TYPE classType = obj->getType();
      _monClass[classType]->remove(obj);
   }

   /**
    * Update the monitor status using a mask
    *
    * @param mask the monitor mask
    */
   void setMonitorStatus( UINT32 mask )
   {
      monUpdateGroupMask( mask ) ;

      setMonitorLvl( MON_CLASS_QUERY, monGroupMaskToLevle( mask, MON_CLASS_QUERY ) ) ;
      setMonitorLvl( MON_CLASS_LATCH, monGroupMaskToLevle( mask, MON_CLASS_LATCH ) ) ;
      setMonitorLvl( MON_CLASS_LOCK, monGroupMaskToLevle( mask, MON_CLASS_LOCK ) ) ;
   }

   /**
    * Update the monitor data collection lvl of a class container
    *
    * @param classType the class type to update
    * @param mode      the new collection level
    */
   void setMonitorLvl( MON_CLASS_TYPE classType, MON_DATA_LEVEL mode )
   {
      _monClass[classType]->setMonitorLvl( mode ) ;
   }

   MON_DATA_LEVEL getCollectionLvl( MON_CLASS_TYPE classType )
   {
      return _monClass[classType]->getCollectionLvl() ;
   }

   BOOLEAN isOperational( MON_CLASS_TYPE classType ) const
   {
      return _monClass[classType]->isOperational() ;
   }

   BOOLEAN isCurOperational( MON_CLASS_TYPE classType ) const
   {
      return _monClass[classType]->isCurOperational() ;
   }

   /**
    * Update the history event size of all class container
    *
    * @param size      the new monitor history event size
    */
   void setHistEventSize( UINT32 size )
   {
      for ( INT32 i = 0; i < MON_CLASS_MAX; i ++ )
      {
         _monClass[i]->setMaxArchivedListLen( size ) ;
      }
   }

   /**
    * return the list of the target monClass
    * @param cachedMonClassList the vector to populate
    * @param classType the specific monClass to read
    * @param listType the type of list to read
    */
   template <class T> void dumpList ( ossPoolVector<T> & cachedMonClassList,
                                      MON_CLASS_TYPE classType,
                                      MON_CLASS_LIST_TYPE listType,
                                      IExecutor *cb = NULL )
   {
      _monClass[classType]->dumpList(cachedMonClassList, listType, cb) ;
   }

   /**
    * Cleanup/Relocate each container's pending archive and pending delete objects
    */
   void relocate() ;

   INT32 fini() ;

private:
   ossPoolVector<monClassContainer*> _monClass;
} ;

typedef _monMonitorManager monMonitorManager ;

extern monMonitorManager *g_monMgrPtr;
}
#endif
