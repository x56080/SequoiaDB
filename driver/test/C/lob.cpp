#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"
#include <iostream>
TEST(lob, lob_global_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   INT32 counter = 0 ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   sdbCursorHandle cur               = 0 ;
   sdbLobHandle lob                  = 0 ;
   INT32 NUM                         = 10 ;
   SINT64 count                      = 0 ;
   bson_oid_t oid ;
   bson obj ;
   #define BUFSIZE1 (1024 * 1024 * 3)
   //#define BUFSIZE1 ( 1024 * 2 )
   #define BUFSIZE2 (1024 * 1024 * 2)
   SINT64 lobSize = -1 ;
   UINT64 createTime = -1 ;
   CHAR buf[BUFSIZE1] = { 0 } ;
   CHAR readBuf[BUFSIZE2] = { 0 } ;
   UINT32 readCount = 0 ;
   CHAR c = 'a' ;
   for ( INT32 i = 0; i < BUFSIZE1; i++ )
   {
      buf[i] = c ;
   }
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // open lob 
   bson_oid_gen( &oid ) ; 
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size 
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, lobSize ) ;
   // get lob create time
//   rc = sdbGetLobCreateTime( lob, &createTime ) ;
//   ASSERT_EQ( 0, createTime ) ;
//   ASSERT_EQ( 0, createTime ) ;
   // write lob 
   rc = sdbWriteLob( lob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size 
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BUFSIZE1, lobSize ) ;
   // write lob
   rc = sdbWriteLob( lob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 2 * BUFSIZE1, lobSize ) ;
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // open lob with the mode SDB_LOB_READ
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   // read lob
   rc = sdbReadLob( lob, 1000, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) ;
   }
   // read lob
   rc = sdbReadLob( lob, BUFSIZE2, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) << "readCount is: " << readCount 
         << ", c is: " << c << ", i is: " 
         << i << ", readBuf[i] is: " << readBuf[i] ;
   }
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // reopen it, and read all the content
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = SDB_OK ;
   counter = 0 ;
   while( SDB_EOF != ( rc = sdbReadLob( lob, BUFSIZE2, readBuf, &readCount ) ) )
   {
       eof = sdbLobIsEof( lob ) ;
       ASSERT_EQ( SDB_OK, rc ) ;
       if ( TRUE == eof )
       {
          counter = 1 ;
       }
   }
   ASSERT_EQ( 1, counter ) ;
   //eof = sdbLobIsEof( lob ) ;
   //ASSERT_EQ( SDB_OK, rc ) ;
   //ASSERT_EQ( TRUE, eof ) ;
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // remove lob
   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCursor ( cur ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(lob, lob_createLOBID_test)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson_oid_t oid ;
   CHAR * pTimeStamp                 = NULL;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl) ;
   CHECK_MSG("%s%d\n"," getCollection rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // createLOBID

   //case 1, pTimeStamp = NULL;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //case 2, pTimeStamp = "2019-07-23-18.04.07";   
   pTimeStamp = "2019-07-23-18.04.07" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //case 3, pTimeStamp = "2019-07";   
   pTimeStamp = "2019-07" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   //case 4, pTimeStamp = "2019-07-23-18";   
   pTimeStamp = "2019-07-23-18" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;


   //case 5, sdbCreateLobID
   rc = sdbCreateLobID( cl, &oid) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;

}


TEST(lob,lob_createLob_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson_oid_t oid ;
   sdbLobHandle lob1                 = 0 ;
   sdbLobHandle lob2                 = 0 ;
   sdbLobHandle lob3                 = 0 ;
   sdbLobHandle lob4                 = 0 ;
   CHAR * pTimeStamp                 = NULL;
   sdbCursorHandle cursor            = 0;
   
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection

   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl) ;
   ASSERT_EQ( SDB_OK, rc ) ;


  // case 1, use bson_oid_gen()
   bson_oid_gen( &oid );
   //rc = sdbCreateLob(cl, &oid, &lob1);
   rc = sdbOpenLob(cl, &oid, SDB_LOB_CREATEONLY, &lob1);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_oid_t lob_oid1 ;
   rc = sdbGetLobId(lob1, &lob_oid1) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   CHECK_MSG("%s%d\n","oid = ", oid) ;
   CHECK_MSG("%s%d\n","lob_oid1 = ", lob_oid1) ;

   ASSERT_STREQ(oid.bytes,lob_oid1.bytes);
   rc = sdbCloseLob(&lob1);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbOpenLob(cl, &oid, SDB_LOB_READ, &lob1);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob(&lob1);
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 2, oid is NULL 
   //rc = sdbCreateLob(cl, NULL, &lob2);
   rc = sdbOpenLob(cl, NULL, SDB_LOB_CREATEONLY, &lob2);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_oid_t lob_oid ;
   rc = sdbGetLobId(lob2, &lob_oid) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   CHECK_MSG("%s%d\n","lob_oid = ", lob_oid) ;

   rc = sdbCloseLob(&lob2);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbOpenLob(cl, &lob_oid, SDB_LOB_READ, &lob2);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob(&lob2);
   ASSERT_EQ( SDB_OK, rc ) ;


   // case 3, use sdbCreateLobID to gen oid  
   pTimeStamp = "2019-07-23-18.04.07" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //rc = sdbCreateLob(cl, &oid , &lob3);
   rc = sdbOpenLob(cl, &oid, SDB_LOB_CREATEONLY, &lob3);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob(&lob3);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbOpenLob(cl, &oid, SDB_LOB_READ, &lob3);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob(&lob3);
   ASSERT_EQ( SDB_OK, rc ) ;


   // case 4, use sdbCreateLobID to gen oid 
   rc = sdbCreateLobID(cl, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //rc = sdbCreateLob(cl, &oid , &lob4);
   rc = sdbOpenLob(cl, &oid, SDB_LOB_CREATEONLY, &lob4);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob(&lob4);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbOpenLob(cl, &oid, SDB_LOB_READ, &lob4);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob(&lob4);
   ASSERT_EQ( SDB_OK, rc ) ;

   // display had create Lob
   rc = sdbListLobs(cl, &cursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   
   printf("#######display had create Lob########\n");
   displayRecord( &cursor ) ;
   sdbReleaseCursor( cursor ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;

}


TEST(lob,lob_listLobs_test)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson condition                    ;
   bson selected                     ;
   bson orderBy                      ;
   bson hint                         ;
   INT64 numToSkip                   = 0 ;
   INT64 numToReturn                 = -1 ;
   sdbCursorHandle cursor            = 0;
   sdbCursorHandle lobPiecesCursor   = 0;

   
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl) ;

   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1, without options. 
   rc = sdbListLobs(cl, &cursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cursor ) ;
   sdbReleaseCursor ( cursor ) ;

   rc = sdbListLobPieces(cl, &lobPiecesCursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &lobPiecesCursor ) ;
   sdbReleaseCursor ( lobPiecesCursor ) ;

   bson_init( &condition );
   bson_init( &selected );
   bson_init( &orderBy );
   bson_init( &hint );
   
   bson_append_code(&selected, "oid", "");

   bson_finish( &condition );
   bson_finish( &selected );
   bson_finish( &orderBy );
   bson_finish( &hint );

   // case 2, have options.
   rc = sdbListLobs1(cl, &condition, &selected, &orderBy, &hint, numToSkip, numToReturn, &cursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cursor ) ;
   sdbReleaseCursor ( cursor ) ;


   rc = sdbListLobPieces1(cl, &condition, &selected, &orderBy, &hint, numToSkip, numToReturn, &lobPiecesCursor);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &condition ) ;
   bson_destroy( &selected ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;

   displayRecord( &lobPiecesCursor ) ;
   sdbReleaseCursor ( lobPiecesCursor ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}


TEST(lob,lob_primaryAndSubLob_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCSHandle cs                    = 0 ;
   sdbCollectionHandle primaryCL     = 0 ;
   sdbCollectionHandle subACL        = 0 ;
   sdbCollectionHandle subBCL        = 0 ;
   sdbCursorHandle cur               = 0 ;
   sdbLobHandle primaryLob           = 0 ;
   sdbLobHandle subLob               = 0 ;
   bson_oid_t oidB;
   sdbLobHandle subBLOb              = 0 ;
   INT32 NUM                         = 10 ;
   SINT64 count                      = 0 ;
   bson_oid_t oid ;
   bson options ;
   #define BUFSIZE1 (1024 * 1024 * 3)
   //#define BUFSIZE1 ( 1024 * 2 )
   #define BUFSIZE2 (1024 * 1024 * 2)
   SINT64 lobSize = -1 ;
   UINT64 createTime = -1 ;
   CHAR buf[BUFSIZE1] = { 0 } ;
   CHAR readBuf[BUFSIZE2] = { 0 } ;
   UINT32 readCount = 0 ;
   CHAR c = 'a' ;
   for ( INT32 i = 0; i < BUFSIZE1; i++ )
   {
      buf[i] = c ;
   }
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get sub cl.
   rc = getCollection ( db, SUB_A_COLLECTION_FULL_NAME, &subACL) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollection ( db, SUB_B_COLLECTION_FULL_NAME, &subBCL) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // create primary cl.
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, &cs) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &options );

   bson shardingKey;
   bson_init( &shardingKey );
   bson_append_int( &shardingKey, "date", 1);
   bson_finish( &shardingKey) ;
   
   bson_append_string( &options, "LobShardingKeyFormat", "YYYYMMDD") ;
   bson_append_bson( &options, "ShardingKey", &shardingKey);
   bson_append_string( &options, "ShardingType", "range") ;
   bson_append_bool( &options, "IsMainCL", TRUE);
   bson_finish( &options) ;
   
   rc = sdbCreateCollection1( cs, PRIMARY_COLLECTION_NAME, &options, &primaryCL);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson attach_options;
   bson LowBound;
   bson UpBound;
   bson_init( &attach_options );
   bson_init( &LowBound );
   bson_init( &UpBound );

   bson_append_string( &LowBound, "date", "20190701") ;
   bson_append_string( &UpBound , "date", "20190801") ;
   
   bson_finish( &LowBound) ;
   bson_finish( &UpBound) ;

   bson_append_bson( &attach_options, "LowBound", &LowBound);
   bson_append_bson( &attach_options, "UpBound", &UpBound);

   bson_finish( &attach_options) ;

   // attach subACL
   rc = sdbAttachCollection( primaryCL, SUB_A_COLLECTION_FULL_NAME, &attach_options);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &attach_options );
   bson_destroy( &LowBound );
   bson_destroy( &UpBound );
   
   bson_init( &attach_options );
   bson_init( &LowBound );
   bson_init( &UpBound );

   bson_append_string( &LowBound, "date", "20190801") ;
   bson_append_string( &UpBound , "date", "20190901") ;
   
   bson_finish( &LowBound) ;
   bson_finish( &UpBound) ;

   bson_append_bson( &attach_options, "LowBound", &LowBound);
   bson_append_bson( &attach_options, "UpBound", &UpBound);

   bson_finish( &attach_options ) ;

   // attach subBCL
   rc = sdbAttachCollection( primaryCL, SUB_B_COLLECTION_FULL_NAME, &attach_options);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &attach_options ) ;
   bson_destroy( &LowBound );
   bson_destroy( &UpBound );

   CHAR * pTimeStamp = "2019-07-23-18.04.07";

   rc = sdbCreateLobID1(primaryCL,pTimeStamp, &oid);
   ASSERT_EQ( SDB_OK, rc ) ;

   // createLOB in primaryCL
   printf("#####createLob in primaryCL #####\n");
   //rc = sdbCreateLob(primaryCL, &oid, &primaryLob);
   rc = sdbOpenLob(primaryCL, &oid, SDB_LOB_CREATEONLY, &primaryLob);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob( &primaryLob);
   CHECK_MSG("%s%d\n"," sdbCloseLob rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // openLob in subACL
   printf("#####openLob in subACL #####\n");
   rc = sdbOpenLob(subACL, &oid, SDB_LOB_WRITE, &subLob);
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // write in subACL
   printf("#####write in subACL #####\n");
   rc = sdbGetLobSize( subLob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, lobSize ) ;
   
   rc = sdbWriteLob( subLob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( subLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get subACL size 
   rc = sdbGetLobSize( subLob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BUFSIZE1, lobSize ) ;
   // close subACL
   rc = sdbCloseLob ( &subLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // read in primaryCL
   printf("#####read in primaryLob #####\n");

   rc = sdbOpenLob(primaryCL, &oid, SDB_LOB_READ, &primaryLob);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbReadLob( primaryLob, 1000, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( primaryLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) ;
   }
   // read subACL
   rc = sdbReadLob( primaryLob, BUFSIZE2, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( primaryLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) << "readCount is: " << readCount 
         << ", c is: " << c << ", i is: " 
         << i << ", readBuf[i] is: " << readBuf[i] ;
   }
   // close subCL
   rc = sdbCloseLob ( &primaryLob) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   pTimeStamp = "2019-08-23-18.04.07";
   rc = sdbCreateLobID1(subBCL,pTimeStamp, &oidB);
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("#####CreateLob in subBCL #####\n");
   //rc = sdbCreateLob(subBCL, &oidB, &subBLOb);
   rc = sdbOpenLob(subBCL, &oidB, SDB_LOB_CREATEONLY, &subBLOb);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob( &subBLOb );
   ASSERT_EQ( SDB_OK, rc ) ;

   printf("#####sdbListLobs in primaryCL #####\n");
   rc = sdbListLobs(primaryCL, &cur);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cur ) ;
   sdbReleaseCursor ( cur ) ;

   printf("#####sdbRemoveLob in primaryCL #####\n");
   rc = sdbRemoveLob(primaryCL, &oidB);
   ASSERT_EQ( SDB_OK, rc ) ;

   printf("#####sdbOpenLob in subBCL #####\n");
   rc = sdbOpenLob(subBCL , &oidB, SDB_LOB_READ , &subBLOb);
   ASSERT_EQ( SDB_FNE, rc ) ;

   printf("#####sdbListLobs in primaryCL #####\n");
   rc = sdbListLobs(primaryCL, &cur);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cur ) ;
   sdbReleaseCursor ( cur ) ;


   bson_destroy( &options);
   bson_destroy( &shardingKey);
   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( primaryCL) ;
   sdbReleaseCollection ( subACL ) ;
   sdbReleaseCollection ( subBCL ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection ( db ) ;

}

