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

   Source File Name = hybridTreeIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/hybridTreeIterator.h"

namespace engine
{
namespace vessel
{
   hybridTreeIterator::~hybridTreeIterator()
   {
      
   }

   INT32 hybridTreeIterator::init(const lsmColumnFamily &cf,
                                  const globalLogicalClId &cl,
                                  const indexObject *obj)
   {
      INT32 rc = SDB_OK;
      reset();

      rc = _lsm.init(cf, cl, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void hybridTreeIterator::reset()
   {
      _lsm.reset();
   }

   INT32 hybridTreeIterator::seek(const VEC_ELE_CMP &eles,
                                  const inclusiveVec &iv,
                                  const options &o)
   {
      INT32 rc = SDB_OK;
      rc = _lsm.seek(eles, iv, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::seek(const bson::BSONObj &key,
                                  const inclusiveVec &iv,
                                  const options &o)
   {
      return _lsm.seek(key, iv, o);
   }

   INT32 hybridTreeIterator::equal(const bson::BSONObj &key)
   {
      return _lsm.equal(key);
   }

   INT32 hybridTreeIterator::equal(const VEC_ELE_CMP &matchEles)
   {
      return _lsm.equal(matchEles);
   }

   INT32 hybridTreeIterator::locateNext(const indexEntryLocation *location,
                                        const options &o)
   {
      return _lsm.locateNext(location, o);
   }

   INT32 hybridTreeIterator::next()
   {
      INT32 rc = SDB_OK;
      do
      {
         rc = _lsm.next();
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (!_lsm.isReadyToRead())
         {
            break;
         }
         else if (_lsm.isMarkedRemoved())
         {
            continue;
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::advance(const bson::BSONObj &prevKey,
                                     INT32 fieldCountToCmpInPrev,
                                     const VEC_ELE_CMP &matchEles,
                                     const inclusiveVec &iv)
   {
      INT32 rc = SDB_OK;
      rc = _lsm.advance(prevKey, fieldCountToCmpInPrev,
                        matchEles, iv);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to advance:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN hybridTreeIterator::isReadyToRead()const
   {
      return _lsm.isReadyToRead();
   }

   INT32 hybridTreeIterator::pause()
   {
      return _lsm.pause();
   }

   bson::BSONObj hybridTreeIterator::getKeyObj(BOOLEAN withFieldName,
                                               bson::BufBuilder *buf)const
   {
      return _lsm.getKeyObj(withFieldName, buf);
   }

   DPS_LSN_OFFSET hybridTreeIterator::getLSN()const
   {
      return _lsm.getLSN();
   }

   DPS_TRANS_ID hybridTreeIterator::getTransID()const
   {
      return _lsm.getTransID();
   }

   recordID hybridTreeIterator::getRid()const
   {
      return _lsm.getRid();
   }

   INT32 hybridTreeIterator::initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location)const
   {
      return _lsm.initOrUpdateLocation(location);
   }

} // namespace vessel

} // namespace engine
