#include <gtest/gtest.h>
#include "client.hpp"
#include <sys/time.h>
#include "../testcommon.hpp"

using namespace std ;
using namespace sdbclient ;
typedef INT32 BOOL;


INT32 init(sdb &db, INT32 timeLen=0, INT32 size=0)
{
   INT32 rc = SDB_OK;
   sdbClientConf conf;
   conf.enableCacheStrategy = TRUE;
   conf.cacheTimeInterval = timeLen;
   rc = initClient(&conf);
   if (SDB_OK != rc)
   {
      return rc;
   }
   
   return db.connect(HOST, SERVER);
   
}

void fini(sdb &db)
{
   db.disconnect();
}

INT32 createCS(sdb &db, char *name , int size, sdbCollectionSpace& cs)
{
   INT32 rc = SDB_OK;
   snprintf(name, size, "cs_%d", getpid());
   if (SDB_OK == db.getCollectionSpace(name, cs)){
      rc = db.dropCollectionSpace(name);
      if (SDB_OK != rc) return rc;
   }
   rc = db.createCollectionSpace(name, 0, cs);
   
   return rc;
}

INT32 createCL(sdbCollectionSpace& cs, char *name, int size, sdbCollection& cl)
{
   INT32 rc = SDB_OK;
   snprintf(name, size, "cs_%d", getpid());
   rc = cs.createCollection(name, cl);
   
   return rc;
}

INT32 dropCL(sdbCollectionSpace& cs, char *name)
{
   return cs.dropCollection(name);
}

INT32 dropCS(sdb &db, char* name)
{
   return db.dropCollectionSpace(name);
}

INT32 getTimeOfgetCS(sdb &db, char* name, clock_t& diff)
{
   sdbCollectionSpace cs;
   struct timeval begin, end;
   INT32 rc = SDB_OK;
   gettimeofday(&begin, NULL);
   rc = db.getCollectionSpace(name, cs);
   gettimeofday(&end, NULL);
   
   if (end.tv_sec > begin.tv_sec){
      diff = (end.tv_sec - begin.tv_sec)*1000000  + end.tv_usec - begin.tv_usec;
   }else{
      diff = end.tv_usec - begin.tv_usec;
   }
   
   return rc;  
}

INT32 getTimeOfgetCL(sdbCollectionSpace &cs, char* name, clock_t& diff)
{
   sdbCollection cl;
   struct timeval begin, end;
   INT32 rc = SDB_OK;
   gettimeofday(&begin, NULL);
   rc = cs.getCollection(name, cl);
   gettimeofday(&end, NULL);
   if (end.tv_sec > begin.tv_sec){
      diff = (end.tv_sec - begin.tv_sec)*1000000  + end.tv_usec - begin.tv_usec;
   }else{
      diff = end.tv_usec - begin.tv_usec;
   }
   
   return rc;  
}

INT32 getTimeOfgetCLByFullName(sdb &db, char* csName, char* clName, clock_t& diff)
{
   sdbCollection cl;
   struct timeval begin, end;
   INT32 rc = SDB_OK;
   char fullName[256] = {0};
   
   snprintf(fullName, sizeof(fullName), "%s.%s", csName, clName);
   gettimeofday(&begin, NULL);
   rc = db.getCollection(fullName, cl);
   gettimeofday(&end, NULL);
   if (end.tv_sec > begin.tv_sec){
      diff = (end.tv_sec - begin.tv_sec)*1000000  + end.tv_usec - begin.tv_usec;
   }else{
      diff = end.tv_usec - begin.tv_usec;
   }
   
   return rc;  
}

TEST( turnonCache, createCollection)
{
   sdb db;
   sdbCollectionSpace cs;
   sdbCollection cl;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   ASSERT_EQ(SDB_OK, createCL(cs, clName, sizeof(clName), cl));
   ASSERT_EQ(SDB_OK, dropCL(cs, clName));
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
 
   fini(db);
}

TEST( turnonCache, getCollectionSpace )
{
   sdb db,db1;
   sdbCollectionSpace cs;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   
   rc = db1.connect(HOST, SERVER);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(SDB_OK, getTimeOfgetCS(db1, csName, diff1));
   ASSERT_EQ(SDB_OK, getTimeOfgetCS(db, csName, diff2));
   //ASSERT_LT(diff2, diff1);
  
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   ASSERT_EQ(SDB_OK, getTimeOfgetCS(db1, csName, diff1));
   fini(db);
}

TEST( turnonCache, getCollection)
{
   sdb db,db1;
   sdbCollectionSpace cs, cs1;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   ASSERT_EQ(SDB_OK, createCL(cs, clName, sizeof(clName), cl));
   
   rc = db1.connect(HOST, SERVER);
   ASSERT_EQ(SDB_OK, rc);
   rc = db1.getCollectionSpace(csName, cs1);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(SDB_OK, getTimeOfgetCL(cs1, clName, diff1));
   ASSERT_EQ(SDB_OK, getTimeOfgetCL(cs1, clName, diff2));
   //ASSERT_LT(diff2, diff1);
   
   ASSERT_EQ(SDB_OK, dropCL(cs, clName));
   ASSERT_EQ(SDB_OK, getTimeOfgetCL(cs1, clName, diff2));
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   fini(db);
}

TEST( turnonCache, getCollection1)
{
   sdb db,db1;
   sdbCollectionSpace cs, cs1;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   ASSERT_EQ(SDB_OK, createCL(cs, clName, sizeof(clName), cl));
   
   rc = db1.connect(HOST, SERVER);
   ASSERT_EQ(SDB_OK, rc);
  
   ASSERT_EQ(SDB_OK, getTimeOfgetCLByFullName(db1, csName, clName, diff1));
   ASSERT_EQ(SDB_OK, getTimeOfgetCLByFullName(db1, csName, clName, diff2));
   //ASSERT_LT(diff2, diff1);
   
   ASSERT_EQ(SDB_OK, dropCL(cs, clName));
   ASSERT_EQ(SDB_OK, getTimeOfgetCLByFullName(db1, csName, clName, diff2));
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   fini(db);
}

TEST( turnonCache, getCollectionSpaceOfTimeOut)
{
   sdb db;
   sdbCollectionSpace cs;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   
   ASSERT_EQ(SDB_OK, init(db, 1));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   
   ASSERT_EQ(SDB_OK, getTimeOfgetCS(db, csName, diff1));
   usleep(1000000);
   ASSERT_EQ(SDB_OK, getTimeOfgetCS(db, csName, diff2));
   //ASSERT_LT(diff1, diff2);
   
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   fini(db);
}

TEST( turnonCache, getCollectionOfTimeOut)
{
   sdb db,db1;
   sdbCollectionSpace cs, cs1;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db, 1));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   ASSERT_EQ(SDB_OK, createCL(cs, clName, sizeof(clName), cl));
   
   ASSERT_EQ(SDB_OK, getTimeOfgetCL(cs, clName, diff1));
   usleep(1000000);
   ASSERT_EQ(SDB_OK, getTimeOfgetCLByFullName(db, csName, clName, diff2));
   //ASSERT_LT(diff1, diff2);
   
   ASSERT_EQ(SDB_OK, dropCL(cs, clName));
   
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   fini(db);
}

TEST( turnonCache, getCollectionSpaceAfterDrop)
{
   sdb db;
   sdbCollectionSpace cs;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   
   ASSERT_EQ(SDB_OK, init(db, 1));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   ASSERT_EQ(-34, getTimeOfgetCS(db, csName, diff1));
  
   fini(db);
}

TEST( turnonCache, getCollectionAfterDrop)
{
   sdb db;
   sdbCollectionSpace cs;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db, 1));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   ASSERT_EQ(SDB_OK, createCL(cs, clName, sizeof(clName), cl));
   
   ASSERT_EQ(SDB_OK, dropCL(cs, clName));
   ASSERT_EQ(-23, getTimeOfgetCL(cs, clName, diff1));
   ASSERT_EQ(-23, getTimeOfgetCLByFullName(db, csName, clName, diff2));
   
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   fini(db);
}

TEST( turnonCache, testUpdateTimeStamp)
{
   sdb db,db1;
   sdbCollectionSpace cs, cs1;
   sdbCollection cl,cl1;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db, 3));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   ASSERT_EQ(SDB_OK, createCL(cs, clName, sizeof(clName), cl));
   
   rc = db1.connect(HOST, SERVER);
   ASSERT_EQ(SDB_OK, rc);
   
   ASSERT_EQ(SDB_OK, getTimeOfgetCLByFullName(db1, csName, clName, diff1));
   rc = db1.getCollectionSpace(csName, cs1);
   ASSERT_EQ(SDB_OK, rc);
   
   rc = cs1.getCollection(clName, cl1);
   ASSERT_EQ(SDB_OK, rc);
      
   usleep(1000000);
   rc = cl1.insert(BSON("_id"<<0));
   usleep(1000000);
   ASSERT_EQ(SDB_OK, getTimeOfgetCLByFullName(db1, csName, clName, diff2));
   //ASSERT_LT(diff2, diff1);
   
   struct timeval begin, end;
   gettimeofday(&begin, NULL);
   ASSERT_EQ(SDB_OK, dropCL(cs, clName));
   gettimeofday(&end, NULL);
   
   if (end.tv_sec > begin.tv_sec){
      diff1 = (end.tv_sec - begin.tv_sec)*1000000  + end.tv_usec - begin.tv_usec;
   }else{
      diff1 = end.tv_usec - begin.tv_usec;
   }
   
   rc = getTimeOfgetCLByFullName(db1, csName, clName, diff2) ;
   ASSERT_TRUE( rc == SDB_OK || rc == -23) << "drop spend" << diff1 << "Microsecond" << endl;
   /*
   if ( 3000000 - diff1 >  1000000) {
      ASSERT_EQ(SDB_OK, getTimeOfgetCLByFullName(db1, csName, clName, diff2)) << "drop spend" <<  diff1 << "ms";
   }else{
      ASSERT_EQ(-23, getTimeOfgetCLByFullName(db1, csName, clName, diff2)) << "drop spend" <<  diff1 << "ms";
   }
   */
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   fini(db);  
}

TEST( turnonCache, getCLOfTimeOutandDropbyOtherConn)
{
   sdb db,db1;
   sdbCollectionSpace cs, cs1;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   ASSERT_EQ(SDB_OK, init(db, 1));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   ASSERT_EQ(SDB_OK, createCL(cs, clName, sizeof(clName), cl));
   
   rc = db1.connect(HOST, SERVER);
   ASSERT_EQ(SDB_OK, rc);
   rc = db1.getCollectionSpace(csName, cs1);
   
   ASSERT_EQ(SDB_OK, dropCL(cs1, clName));
   usleep(1000000);
   ASSERT_EQ(-23, getTimeOfgetCL(cs, clName, diff1));
   ASSERT_EQ(-23, getTimeOfgetCLByFullName(db, csName, clName, diff2));
   
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
   fini(db); 
}

TEST( turnonCache, getCSOfTimeOutandDropbyOtherConn)
{
   sdb db, db1;
   sdbCollectionSpace cs;
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   
   ASSERT_EQ(SDB_OK, init(db, 1));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   
   rc = db1.connect(HOST, SERVER);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(SDB_OK, dropCS(db1, csName));
   usleep(1000000);
   ASSERT_EQ(-34, getTimeOfgetCS(db, csName, diff1));

   fini(db);
}

BOOL isStandalone(sdb &db)
{
   sdbCursor cursor;
   INT32 rc = db.getList(cursor, SDB_LIST_GROUPS); 
   if (rc == -159){
      return TRUE;
   }else{
      return FALSE;
   }
}

TEST( turnonCache, getMulCLAfterDropCS)
{
   sdb db;
   sdbCollectionSpace cs, cs1;
   sdbCollection cl[5];
   INT32 rc = SDB_OK;
   clock_t diff1, diff2;
   char csName[127] = {0};
   char clName[127] = {0};
   char fullName[256] = {0};
   ASSERT_EQ(SDB_OK, init(db, 1));
   ASSERT_EQ(SDB_OK, createCS(db, csName, sizeof(csName), cs));
   
   for(int i = 0; i <5; ++i)
   {
      snprintf(clName, sizeof(clName), "cl_%d_%d", i, getpid());
      rc = cs.createCollection(clName, cl[i]);
      ASSERT_EQ(SDB_OK, rc);
      
      ASSERT_EQ(SDB_OK, getTimeOfgetCL(cs, clName, diff1));
   }
    
   ASSERT_EQ(SDB_OK, dropCS(db, csName));
  
   int expectRes = -23;
   if (isStandalone(db))
   {
      expectRes = -34;
   }
   for (int i =0; i<5; ++i)
   {
      snprintf(clName, sizeof(clName), "cl_%d_%d", i, getpid());
      ASSERT_EQ(expectRes, getTimeOfgetCL(cs, clName, diff1));
      ASSERT_EQ(expectRes, getTimeOfgetCLByFullName(db, csName, clName, diff2));
   }

   fini(db); 
}

