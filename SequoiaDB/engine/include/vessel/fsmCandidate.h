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

   Source File Name = fsmCandidate.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FSM_CANDIDATE_H_
#define VESSEL_FSM_CANDIDATE_H_

#include "vessel/pageIdentifier.h"
#include "vessel/vesselIdDef.h"
#include "vessel/freeSpaceMapDef.h"
#include <memory> // c++ 11

namespace engine
{
namespace vessel
{
   class fsmCandidate : public SDBObject
   {
      public:
         class mutableInfo : public SDBObject
         {
            public:
               mutableInfo(){}
               ~mutableInfo(){}
               mutableInfo(const mutableInfo &o):
               _lpid(o._lpid),
               _lvl(o._lvl){}
               explicit mutableInfo(PAGE_ID lpid, INT32 lvl):
               _lpid(lpid),
               _lvl(lvl){}
               mutableInfo &operator=(const mutableInfo &o)
               {
                  _lpid = o._lpid;
                  _lvl = o._lvl;
                  return *this;
               }
            public:
               PAGE_ID _lpid = INVALID_PAGE_ID;
               INT32 _lvl = FSM_INVALID_SPACE_LVL;
         };//class mutableInfo

         typedef std::shared_ptr<mutableInfo> SHARED_INFO_PTR;

      public:
         fsmCandidate(){}
         ~fsmCandidate(){}
         fsmCandidate(const fsmCandidate &o) = delete;

         fsmCandidate &operator=(const fsmCandidate &o)
         {
            _seq = o._seq;
            _sptr = o._sptr;
            return *this;
         }


      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _sptr.get();
         }

         OSS_INLINE void reset()
         {
            _seq = 0;
            _sptr.reset();
            return;
         }

         OSS_INLINE void reset(UINT32 seq,
                               const SHARED_INFO_PTR &sptr)
         {
            _seq = seq;
            _sptr = sptr;
            return;
         }

         OSS_INLINE UINT32 getSeq()const
         {
            return _seq;
         }

         OSS_INLINE PAGE_ID getLpid()const
         {
            return (nullptr == _sptr.get()) ?
                   INVALID_PAGE_ID :
                   _sptr->_lpid;
         }

         OSS_INLINE INT32 getSpaceLvl()const
         {
            return (nullptr == _sptr.get()) ?
                   FSM_INVALID_SPACE_LVL :
                   _sptr->_lvl;
         }

         OSS_INLINE BOOLEAN setLpid(PAGE_ID lpid)
         {
            BOOLEAN r = FALSE;
            if (nullptr != _sptr.get())
            {
               _sptr->_lpid = lpid;
               r = TRUE;
            }
            return r;
         }

         OSS_INLINE BOOLEAN setSpaceLvl(INT32 lvl)
         {
            BOOLEAN r = FALSE;
            if (nullptr != _sptr.get() &&
                (isValidFsmLvL(lvl) ||
                 FSM_INVALID_SPACE_LVL == lvl))
            {
               _sptr->_lvl = lvl;
               r = TRUE;
            }
            return r;
         }

         OSS_INLINE const SHARED_INFO_PTR &getInfoPtr()const
         {
            return _sptr;
         }
         OSS_INLINE SHARED_INFO_PTR &getInfoPtr()
         {
            return _sptr;
         }

      private:
         UINT32 _seq = 0;
         SHARED_INFO_PTR _sptr;
   };//class fsmCandidate

   INT32 makeFsmCandidateSharedInfoPtr(PAGE_ID lpid,
                                       INT32 lvl,
                                       fsmCandidate::SHARED_INFO_PTR &ptr);
}//namespace vessel
}//namespace engine

#endif//VESSEL_FSM_CANDIDATE_H_