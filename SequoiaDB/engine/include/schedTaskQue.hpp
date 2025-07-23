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

   Source File Name = schedTaskQue.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_QUE_HPP__
#define SCHED_TASK_QUE_HPP__

#include "pmdDef.hpp"
#include "ossQueue.hpp"
#include "ossPriorityQueue.hpp"
#include "schedDef.hpp"

namespace engine
{

   /*
      SCHED_TASK_QUE_TYPE define
   */
   enum SCHED_TASK_QUE_TYPE
   {
      SCHED_TASK_FIFO_QUE  = 1,
      SCHED_TASK_PIRORITY_QUE
   } ;

   /*
      _schedTaskQueBase define
   */
   class _schedTaskQueBase : public SDBObject
   {
      public:
         _schedTaskQueBase() {}
         virtual ~_schedTaskQueBase() {}

      public:
         virtual UINT32    size() = 0 ;
         virtual BOOLEAN   isEmpty() = 0 ;
         virtual void      push( const pmdEDUEvent &event, INT64 userData, UINT64 recvTimeUs ) = 0 ;
         virtual BOOLEAN   pop( pmdEDUEvent &event, UINT64 &recvTimeUs, INT64 millisec ) = 0 ;

   } ;
   typedef _schedTaskQueBase schedTaskQueBase ;

   /*
      _schedFIFOTaskQue define
   */
   class _schedFIFOTaskQue : public _schedTaskQueBase
   {
      struct _queData
      {
         pmdEDUEvent    _event ;
         UINT64         _recvTimeMs ;

         _queData()
         {
            _recvTimeMs = 0 ;
         }

         _queData( const pmdEDUEvent &event, UINT64 recvTimeUs )
         {
            _event = event ;
            _recvTimeMs = recvTimeUs ;
         }
      } ;

      public:
         _schedFIFOTaskQue() ;
         virtual ~_schedFIFOTaskQue() ;

      public:
         virtual UINT32    size() ;
         virtual BOOLEAN   isEmpty() ;
         virtual void      push( const pmdEDUEvent &event, INT64 userData, UINT64 recvTimeUs ) ;
         virtual BOOLEAN   pop( pmdEDUEvent &event, UINT64 &recvTimeUs, INT64 millisec ) ;

      private:
         ossQueue<_queData>         _queue ;
   } ;
   typedef _schedFIFOTaskQue schedFIFOTaskQue ;

   /*
      _schedPriorityTaskQue define
   */
   class _schedPriorityTaskQue : public _schedTaskQueBase
   {
      struct _priorityEvent
      {
         public:
            pmdEDUEvent    _event ;
            UINT64         _recvTimeUs ;

         private:
            INT64          _priority ;

         public:
            _priorityEvent( const pmdEDUEvent &event,
                            UINT64 recvTimeUs,
                            INT64 priority = 0 )
            {
               _event = event ;
               _recvTimeUs = recvTimeUs ;
               _priority = 0 - priority ;
            }
            _priorityEvent()
            {
               _priority = 0 ;
               _recvTimeUs = 0 ;
            }

            bool operator< ( const _priorityEvent &right ) const
            {
               if ( _priority < right._priority )
               {
                  return true ;
               }
               return false ;
            }
      } ;
      typedef _priorityEvent priorityEvent ;

      public:
         _schedPriorityTaskQue() ;
         virtual ~_schedPriorityTaskQue() ;

      public:
         virtual UINT32    size() ;
         virtual BOOLEAN   isEmpty() ;
         virtual void      push( const pmdEDUEvent &event, INT64 userData, UINT64 recvTimeUs ) ;
         virtual BOOLEAN   pop( pmdEDUEvent &event, UINT64 &recvTimeUs, INT64 millisec ) ;

      private:
         ossPriorityQueue<priorityEvent>        _queue ;
   } ;
   typedef _schedPriorityTaskQue schedPriorityTaskQue ;

}

#endif // SCHED_TASK_QUE_HPP__

