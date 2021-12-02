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

   Source File Name = sequoiaFSHashBucket.cpp

   Descriptive Name = sequoiafs fuse file-lob manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        10/30/2020  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSMetaHashBucket.hpp"

using namespace sequoiafs; 

INT32 sequoiaFSMetaHashBucket::hashDirPid(INT64 parentId, const CHAR* dirName)
{
   UINT32 hashKey = 0;
   //struct dirNameId name(dirName, parentId);
   CHAR _hashStr[OSS_MAX_NAMESIZE + 1];
   ossMemset(_hashStr, 0, OSS_MAX_NAMESIZE + 1);
   ossStrncpy(_hashStr, dirName, OSS_MAX_NAMESIZE);

   //hashKey = ossHash((CHAR *)&name, sizeof(dirNameId), 5); //ossHash(char ,size ,)

   hashKey = ossHash( ( const BYTE * )_hashStr, OSS_MAX_NAMESIZE,
                      ( const BYTE * )( &parentId ),
                      sizeof( parentId ) ) ;
   
   return hashKey/_size;
}

void sequoiaFSMetaHashBucket::lockBucketR(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.get_shared();
}

void sequoiaFSMetaHashBucket::unLockBucketR(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.release_shared();
}

void sequoiaFSMetaHashBucket::lockBucketW(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.get();
}

void sequoiaFSMetaHashBucket::unLockBucketW(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.release(); 
}

BOOLEAN sequoiaFSMetaHashBucket::check(UINT32 key, dirMetaNode* node)
{
   BOOLEAN find = FALSE;
   
   dirMetaNode* cur = _buckets[ key % _size ].hashHead;
   while ( cur )
   {
      if(cur == node)
      {
         find = TRUE;
         break;
      }
      cur = cur->bucketNext;
   }

   return find;
}

dirMetaNode* sequoiaFSMetaHashBucket::get(UINT32 key, const CHAR* name, INT64 pid)
{
   dirMetaNode* cur = _buckets[ key % _size ].hashHead;
   INT32 visit = 0;

   while ( cur )
   {
      visit++;
      if(cur->meta.pid() == pid 
            && !ossStrncmp(cur->meta.name(), name, OSS_MAX_NAMESIZE))
      {
         break;
      }

      cur = cur->bucketNext;
   }

  // timesMutex.get();
   _getCount++;
   if(visit > 1)
   {
      _getConflictCount++;
   }
  // timesMutex.release();

   return cur;
}

void sequoiaFSMetaHashBucket::add(UINT32 key, dirMetaNode* node )
{
   dirMetaNode* cur = NULL;
   UINT32 hashKey = key % _size;

   PD_LOG(PDDEBUG, "hashbucket add(), key:%d, node:%d", key, node);

   _addCount++;
   if(_buckets[ hashKey ].hashHead == NULL)
   {
      _buckets[ hashKey ].hashHead = node;
      goto done;
   }
   _addConflictCount++;

   cur = _buckets[ hashKey ].hashHead;

   #ifdef _DEBUG
      while ( cur->bucketNext)
      {
         if(cur->meta.pid() == node->meta.pid()
               && !ossStrncmp(cur->meta.name(), node->meta.name(), OSS_MAX_NAMESIZE))
         {
            SDB_ASSERT( FALSE, "dup node" ) ; //TODO:异常场景下，要释放old或者new
         }//TOOD:加一个debug宏，在debug版本做检查是否有冲突，release版本不用做检查，
         //无论debug还是release都从链表头加起

         cur = cur->bucketNext;  
      }
   #endif

   cur = _buckets[ hashKey ].hashHead;

   cur->bucketPre = node;
   node->bucketNext = cur;

   _buckets[ hashKey ].hashHead = node;
/*
   cur->bucketNext = node; 
   node->bucketPre = cur;
*/
done:
   return;
}

dirMetaNode* sequoiaFSMetaHashBucket::del(UINT32 key, dirMetaNode* node )
{
   PD_LOG(PDDEBUG, "hashbucket del(), key:%d, node:%d", key, node);
   dirMetaNode* cur = _buckets[ key % _size ].hashHead;
   INT32 visit = 0;

   while ( cur)
   {
      visit++;
      if(cur->meta.pid() == node->meta.pid() 
            && !ossStrncmp(cur->meta.name(), node->meta.name(), OSS_MAX_NAMESIZE))
      {
         break;
      }

      cur = cur->bucketNext;
   }

   if(NULL == cur)
   {
      PD_LOG(PDERROR, "bucket del null, key:%d, node:%d", key, node);
      goto done;
      //SDB_ASSERT( FALSE, "node is null" ) ;
   }
      
   if(cur == _buckets[ key % _size ].hashHead)
   {
      if(cur->bucketNext)
      {
         cur->bucketNext->bucketPre = NULL;
         _buckets[ key % _size ].hashHead = cur->bucketNext;
      }
      else
      {
         _buckets[ key % _size ].hashHead = NULL;
      }
   }
   else
   {
      if(cur->bucketPre != NULL)
      {
         cur->bucketPre->bucketNext = cur->bucketNext;
      }
      if(cur->bucketNext != NULL)
      {
         cur->bucketNext->bucketPre = cur->bucketPre;
      }
   }

   cur->bucketPre = NULL;
   cur->bucketNext = NULL;

  // timesMutex.get();
   _delCount++;
   if(visit > 1)
   {
      _delConflictCount++;
   }
  // timesMutex.release();
  
done:
   return cur;
}


