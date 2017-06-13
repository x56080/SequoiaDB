#include "client.h"
#include <gtest/gtest.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "../common/testcommon.hpp"
#define SOCKET int
struct Socket
{
   SOCKET      rawSocket ;
#ifdef SDB_SSL
   SSLHandle*  sslHandle ;
#endif
} ;
typedef struct _htbNode
{
   UINT64 lastTime ;
   CHAR *name ;
} htbNode ;

typedef struct _hashTable
{
   UINT32  capacity ;
   htbNode **node ;
} hashTable ;

struct _Node
{
   ossValuePtr data ;
   struct _Node *next ;
} ;
typedef struct _Node Node ;
typedef INT32 BOOL;

enum BOOLEANVAL
{
    BOOLEAN_FALSE = 0,
    BOOLEAN_TRUE
};

#define ossMutex                  pthread_mutex_t
struct _sdbConnectionStruct
{
   // generic variables, to validate which type does this handle belongs to
   INT32 _handleType ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;
   Node *_cursors ;
   Node *_sockets ;
   hashTable *_tb ;

   UINT64 reserveSpace1 ;
   ossMutex _sockMutex ;
} ;
typedef struct _sdbConnectionStruct sdbConnectionStruct ;

INT32 createCollectionSpace(sdbConnectionHandle conn, char *name, int len, sdbCSHandle *cs,
							const char* prefixname)
{
   pid_t pid;
   INT32 rc = SDB_OK;
   //以PID为后缀，创建CS
   pid = getpid();
   snprintf(name, len, "%s_cs_%d", prefixname, pid);
   
   rc = sdbGetCollectionSpace(conn, name, cs);
   if (rc == SDB_OK){
      rc = sdbDropCollectionSpace(conn, name);
      sdbReleaseCS(*cs);
      if (rc != SDB_OK) return rc;
      
   }
   return sdbCreateCollectionSpace(conn, name, 0, cs);
   
}

INT32 createCollection(sdbCSHandle cs, char *name, int len, sdbCollectionHandle* cl,
					   const char* prefixname)
{
   pid_t pid; 
   //以PID为后缀，创建CL
   pid = getpid();
   snprintf(name, len, "%s_cl_%d", prefixname, pid);
   return sdbCreateCollection(cs, name, cl);
}

INT32 connect(sdbConnectionHandle* conn, INT32 timeLen=0, INT32 size=0)
{
   //sdbConnectionStruct *connStruct
   INT32 rc = SDB_OK;
   
   // 初始化客户端，开启缓存
   sdbClientConf conf;
   conf.enableCacheStrategy = TRUE;
   conf.cacheTimeInterval = timeLen;
   rc = initClient(&conf);
   if (SDB_OK != rc)
   {
      return rc;
   }
   
   //建立连接,判断hashtable是否为空
   getConf() ;
   rc = sdbConnect(HOSTNAME, SVCNAME, USER, PASSWD, conn);
   if (SDB_OK != rc)
   {
      return rc;
   }
   else
   {
      //connStruct = (sdbConnectionStruct*)conn;
      //ASSERT_NE(connStruct->_tb, NULL);
      return rc;
   }
}

INT32 dropCollection(sdbCSHandle cs, sdbCollectionHandle cl, char* name)
{
   INT32 rc = SDB_OK;
   rc = sdbDropCollection(cs, name);
   sdbReleaseCollection(cl);
   return rc;
}

INT32 dropCollectionSpace(sdbConnectionHandle conn, sdbCSHandle cs, char* name)
{
   INT32 rc = SDB_OK;
   rc = sdbDropCollectionSpace(conn, name);
   sdbReleaseCS(cs);
   return rc;
}

void closeConnection(sdbConnectionHandle conn)
{
   sdbDisconnect(conn);
   sdbReleaseConnection(conn);
}

INT32 getTimeofGetCS(sdbConnectionHandle conn, 
                     char* name, 
                     sdbCSHandle *cs,
                     clock_t *diff)
{
   timeval begin, end;
   INT32 rc;
   gettimeofday(&begin, NULL);
   rc = sdbGetCollectionSpace(conn, name, cs);
   gettimeofday(&end, NULL);
   if (end.tv_sec == begin.tv_sec){
      *diff = end.tv_usec - begin.tv_usec;
   }else{
      *diff = (end.tv_sec - begin.tv_sec)*1000000 + end.tv_usec - begin.tv_usec;
   }
   return rc;
}

INT32 getTimeofGetCLByFullName(sdbConnectionHandle conn, 
                               char* csName,
                               char* clName, 
                               sdbCollectionHandle *cl,
                               clock_t *diff)
{
   char fullName[256];
   timeval begin, end;
   INT32 rc;
   snprintf(fullName, sizeof(fullName), "%s.%s", csName, clName);
   gettimeofday(&begin, NULL);
   rc = sdbGetCollection(conn, fullName, cl);
   gettimeofday(&end, NULL);
   if (end.tv_sec == begin.tv_sec){
      *diff = end.tv_usec - begin.tv_usec;
   }else{
      *diff = (end.tv_sec - begin.tv_sec)*1000000 + end.tv_usec - begin.tv_usec;
   }
   return rc;
}

INT32 getTimeofGetCLByName(sdbCSHandle cs, 
                           char* clName, 
                           sdbCollectionHandle *cl,
                           clock_t *diff)
{
   INT32 rc;
   timeval begin, end;
   // 通过上述CS，获取CL，统计耗时
   gettimeofday(&begin, NULL);
   rc = sdbGetCollection1(cs, clName, cl);
   gettimeofday(&end, NULL);
   if (end.tv_sec == begin.tv_sec){
      *diff = end.tv_usec - begin.tv_usec;
   }else{
      *diff = (end.tv_sec - begin.tv_sec)*1000000 + end.tv_usec - begin.tv_usec;
   }
   return rc;
}

TEST( turnonCache, createCollection)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbCSHandle cs;
   sdbCollectionHandle cl;
   char csName[127];
   char clName[127];
   
   ASSERT_EQ(SDB_OK, connect(&conn, 0));
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "createCollection"));
   ASSERT_EQ(SDB_OK, createCollection(cs, clName, sizeof(clName), &cl, "createCollection"));
 
   ASSERT_EQ(SDB_OK, dropCollection(cs, cl, clName));
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   closeConnection(conn);
}

TEST( turnonCache, getCollectionSpace )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbConnectionHandle conn2;
   sdbCSHandle cs, cs1, cs2;
   char csName[127];
   clock_t diff1, diff2;
  
   ASSERT_EQ(SDB_OK, connect(&conn, 0));
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCollectionSpace"));
  
   getConf() ;
   rc = sdbConnect(HOSTNAME, SVCNAME, USER, PASSWD, &conn2);
   ASSERT_EQ(SDB_OK, rc);
   
   ASSERT_EQ(SDB_OK, getTimeofGetCS(conn2, csName, &cs1, &diff1));
   ASSERT_EQ(SDB_OK, getTimeofGetCS(conn2, csName, &cs2, &diff2));
   ASSERT_LT(diff2, diff1);
   
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   
   sdbReleaseCS(cs1);
   sdbReleaseCS(cs2);
   
   closeConnection(conn2);
   closeConnection(conn);  
}

TEST( turnonCache, getCollection)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbConnectionHandle conn2;
   sdbCSHandle cs;
   sdbCollectionHandle cl,cl1,cl2;
   char csName[127];
   char clName[127];
   
   clock_t diff1, diff2;
   
   ASSERT_EQ(SDB_OK, connect(&conn, 0));
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCollection"));
   ASSERT_EQ(SDB_OK, createCollection(cs, clName, sizeof(clName), &cl, "getCollection"));
   
   getConf() ;
   rc = sdbConnect(HOSTNAME, SVCNAME, USER, PASSWD, &conn2);
   ASSERT_EQ(SDB_OK, rc);
   
   rc = getTimeofGetCLByFullName(conn2, csName, clName, &cl1, &diff1);
   ASSERT_EQ(SDB_OK, rc);
   
   rc = getTimeofGetCLByFullName(conn2, csName, clName, &cl2, &diff2);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_LT(diff2, diff1);
  
   ASSERT_EQ(SDB_OK, dropCollection(cs, cl, clName));
   sdbReleaseCollection(cl1);
   sdbReleaseCollection(cl2);
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   
   closeConnection(conn2);
   closeConnection(conn); 
}

TEST( turnonCache, getCollection1)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbConnectionHandle conn2;
   sdbCSHandle cs,cs1;
   sdbCollectionHandle cl,cl1,cl2;
   char csName[127];
   char clName[127];
   clock_t diff1, diff2;
   
   ASSERT_EQ(SDB_OK, connect(&conn, 0));
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCollection1"));
   ASSERT_EQ(SDB_OK, createCollection(cs, clName, sizeof(clName), &cl, "getCollection1"));
   
   // 创建另一连接
   getConf() ;
   rc = sdbConnect(HOSTNAME, SVCNAME, USER, PASSWD, &conn2);
   ASSERT_EQ(SDB_OK, rc);
   
   // 在新连接上获取CS
   rc = sdbGetCollectionSpace(conn2, csName, &cs1);
   ASSERT_EQ(SDB_OK, rc);
   
   // 通过上述CS，再次获取CL，统计耗时
   ASSERT_EQ(SDB_OK,getTimeofGetCLByName(cs1, clName, &cl1, &diff1));
   ASSERT_EQ(SDB_OK,getTimeofGetCLByName(cs1, clName, &cl2, &diff2));
   ASSERT_LT(diff2, diff1);
   
   ASSERT_EQ(SDB_OK, dropCollection(cs, cl, clName));
   sdbReleaseCollection(cl1);
   sdbReleaseCollection(cl2);
   
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   sdbReleaseCS(cs1);
      
   closeConnection(conn2);
   closeConnection(conn); 
}

TEST( turnonCache, getCollectionSpaceOfTimeOut)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbCSHandle cs,cs1,cs2;
   char csName[127];
   clock_t diff1, diff2;
   
   //初始化客户端，启用缓存
   ASSERT_EQ(SDB_OK, connect(&conn, 1, 0)); 
   // 以进程ID为后缀，创建CS
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCollectionSpaceOfTimeOut"));
   
   // 统计获取CS的时间
   ASSERT_EQ(SDB_OK,getTimeofGetCS(conn, csName, &cs1, &diff1));
      
   //睡眠让cache超时
   usleep(1000000);
   // 再次统计获取CS的时间
   ASSERT_EQ(SDB_OK,getTimeofGetCS(conn, csName, &cs2, &diff2));
   ASSERT_LT(diff1, diff2);
   
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   sdbReleaseCS(cs1);
   sdbReleaseCS(cs2);
   closeConnection(conn);

}

TEST( turnonCache, getCollectionOfTimeOut)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbCSHandle cs;
   sdbCollectionHandle cl,cl1,cl2;
   char csName[127];
   char clName[127];
   clock_t diff1, diff2;
   
   //初始化客户端，启用缓存
   ASSERT_EQ(SDB_OK, connect(&conn, 1, 0)); 
   // 以进程ID为后缀，创建CS
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCollectionOfTimeOut"));
   // 以进程ID为后缀，创建CL
   ASSERT_EQ(SDB_OK, createCollection(cs, clName, sizeof(clName), &cl, "getCollectionOfTimeOut"));
   
   // 统计获取CL的时间(processor time)
   ASSERT_EQ(SDB_OK,getTimeofGetCLByName(cs, clName, &cl1, &diff1));
   
   // 让cache超时
   usleep(1000000);
   ASSERT_EQ(SDB_OK,getTimeofGetCLByName(cs, clName, &cl2, &diff2));
   ASSERT_LT(diff1, diff2);
  
   ASSERT_EQ(SDB_OK, dropCollection(cs, cl, clName));
   sdbReleaseCollection(cl1);
   sdbReleaseCollection(cl2);
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   closeConnection(conn);
}

TEST( turnonCache, getCollectionSpaceAfterDrop)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbCSHandle cs;
   char csName[127];
   
   //初始化客户端，启用缓存
   ASSERT_EQ(SDB_OK, connect(&conn, 0, 0)); 
   // 以进程ID为后缀，创建CS
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCollectionSpaceAfterDrop"));
   
   //先drop再获取，验证缓存被清除
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   
   rc = sdbGetCollectionSpace(conn, csName, &cs);
   ASSERT_EQ(-34, rc);
   
   closeConnection(conn);
}

TEST( turnonCache, getCollectionAfterDrop)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbCSHandle cs;
   sdbCollectionHandle cl,cl1,cl2;
   char csName[127];
   char clName[127];
   char fullName[256];
   
   //初始化客户端，启用缓存
   ASSERT_EQ(SDB_OK, connect(&conn, 0, 0)); 
   // 以进程ID为后缀，创建CS
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCollectionAfterDrop"));
   // 以进程ID为后缀，创建CL
   ASSERT_EQ(SDB_OK, createCollection(cs, clName, sizeof(clName), &cl, "getCollectionAfterDrop"));
   
   snprintf(fullName, sizeof(fullName), "%s.%s", csName, clName);
   rc = sdbGetCollection(conn, fullName, &cl1);
   
   ASSERT_EQ(SDB_OK, dropCollection(cs, cl, clName));
   
   rc = sdbGetCollection1(cs, clName, &cl2);
   ASSERT_EQ(-23, rc);
   
   rc = sdbGetCollection(conn, fullName, &cl2);
   ASSERT_EQ(-23, rc);
   
   sdbReleaseCollection(cl1); 
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   closeConnection(conn);
}

TEST( turnonCache, testUpdateTimeStamp)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbConnectionStruct *strConn;
   sdbCSHandle cs;
   sdbCollectionHandle cl;
   char csName[127];
   char clName[127];
   UINT64 csTimeStamp=time(NULL), clTimeStamp=time(NULL);
     
   //初始化客户端，启用缓存
   ASSERT_EQ(SDB_OK, connect(&conn, 1, 10)); 
   // 以进程ID为后缀，创建CS
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "testUpdateTimeStamp"));
   // 以进程ID为后缀，创建CL
   ASSERT_EQ(SDB_OK, createCollection(cs, clName, sizeof(clName), &cl, "testUpdateTimeStamp"));
   
   strConn = (sdbConnectionStruct*)conn;
   hashTable *ht = strConn->_tb;
   for (int i = 0; i<ht->capacity; ++i){
      htbNode *Node = ht->node[i];
      if (Node != NULL){
         if (0 == strncmp(Node->name, csName, strlen(Node->name)))
         {
            csTimeStamp = Node->lastTime;   
         }else{
            clTimeStamp = Node->lastTime; 
         }
      }
   } 
   
   bson obj;
   bson_init(&obj);
   bson_append_int(&obj,"_id", 0);
   bson_finish(&obj);
   
   rc = sdbInsert(cl, &obj);
   ASSERT_EQ(SDB_OK, rc);
   
   for (int i = 0; i<ht->capacity; ++i){
      htbNode *Node = ht->node[i];
      if (Node != NULL){
         if (0 == strncmp(Node->name, csName, strlen(Node->name)))
         {
            ASSERT_LE(csTimeStamp, Node->lastTime); 
         }else{
            ASSERT_LE(clTimeStamp, Node->lastTime);  
         }
      }
   } 
   
   ASSERT_EQ(SDB_OK, dropCollection(cs, cl, clName));
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
      
   closeConnection(conn);
}


TEST( turnonCache, getCLOfTimeOutandDropbyOtherConn)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn,conn1;
   sdbCSHandle cs, cs1;
   sdbCollectionHandle cl,cl1;
   char csName[127];
   char clName[127];
   char fullName[256];
 
   //初始化客户端，启用缓存
   ASSERT_EQ(SDB_OK, connect(&conn, 1)); 
   
   // 以进程ID为后缀，创建CS
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCLOfTimeOutandDropbyOtherConn"));
 
   // 以进程ID为后缀，创建CL
   ASSERT_EQ(SDB_OK, createCollection(cs, clName, sizeof(clName), &cl, "getCLOfTimeOutandDropbyOtherConn"));
   
   //新建连接，删除上述cl
   getConf() ;
   rc = sdbConnect(HOSTNAME, SVCNAME, USER, PASSWD, &conn1);
   ASSERT_EQ(SDB_OK, rc);
   
   rc = sdbGetCollectionSpace(conn, csName, &cs1);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(SDB_OK, dropCollection(cs1, cl, clName));
   
   // 让cache超时
   usleep(1000000);
 
   snprintf(fullName, sizeof(fullName), "%s.%s", csName, clName);
   rc = sdbGetCollection(conn, fullName, &cl1);
   ASSERT_EQ(-23, rc);
   
   rc = sdbGetCollection1(cs, clName, &cl1);
   ASSERT_EQ(-23, rc);
 
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn1, cs, csName));
        
   closeConnection(conn);
   closeConnection(conn1);
}

TEST( turnonCache, getCSOfTimeOutandDropbyOtherConn)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn,conn1;
   sdbCSHandle cs, cs1;
   char csName[127];
   char clName[127];
   
   ASSERT_EQ(SDB_OK, connect(&conn, 1)); 
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getCSOfTimeOutandDropbyOtherConn"));
   
   //新建连接，删除上述cl
   getConf() ;
   rc = sdbConnect(HOSTNAME, SVCNAME, USER, PASSWD, &conn1);
   ASSERT_EQ(SDB_OK, rc);
  
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn1, cs, csName));
   
   // 让cache超时
   usleep(1000000);

   rc = sdbGetCollectionSpace(conn, csName, &cs1);
   ASSERT_EQ(-34, rc);
   
   rc = sdbGetCollectionSpace(conn, csName, &cs1);
   ASSERT_EQ(-34, rc);
   
   closeConnection(conn);
   closeConnection(conn1);
}

TEST( turnonCache, getMulCLAfterDropCS)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn;
   sdbCSHandle cs, cs1;
   sdbCollectionHandle cl[5], cl1[5];
   char csName[127];
   char clName[127];
   char fullName[256];
   
   ASSERT_EQ(SDB_OK, connect(&conn, 1, 0)); 
   ASSERT_EQ(SDB_OK, createCollectionSpace(conn, csName, sizeof(csName), &cs, "getMulCLAfterDropCS"));
   
   for (int i = 0; i <5; ++i)
   {
      snprintf(clName, sizeof(clName), "cl_%d_%d", i, getpid());
      rc = sdbCreateCollection(cs, clName, &cl[i]);
      ASSERT_EQ(SDB_OK, rc);
   }
   
   ASSERT_EQ(SDB_OK, dropCollectionSpace(conn, cs, csName));
   
   int expectRes = -23;
   if (isStandalone(conn)){
      expectRes = -34;
   }
   
   for (int i = 0; i <5; ++i)
   {
      snprintf(fullName, sizeof(fullName), "%s.cl_%d_%d", csName,i, getpid());
      rc = sdbGetCollection(conn, fullName, &cl1[i]);
      ASSERT_EQ(expectRes, rc);
   }
 
   for (int i = 0; i <5; ++i)
   {
      sdbReleaseCollection(cl[i]);
   } 
   closeConnection(conn);
}
