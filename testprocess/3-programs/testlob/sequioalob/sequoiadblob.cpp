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

   Source File Name = sequoiadblob.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sequoiadblob.hpp"

using namespace std ;

CSdbLob::CSdbLob( const std::string& strSrvAddr, const std::string& strDbName, int startID ) 
:m_strSrvAddr ( strSrvAddr ) ,
 m_dbName ( strDbName ) ,
 m_startID ( startID ) ,
 m_conn ( 0 ) ,
 sourceDir ( "lobsource" ) ,
 outDir ( "lobout" )
{
   m_fullClName = m_dbName + ".testlob" ;
   pSrvAddr = NULL ;
   pCSName = NULL ;
   srand( (unsigned int) time(NULL) ) ;

   DIR* dp = NULL ;
   struct dirent* direntp ;
   fileslist.clear ( ) ;
   dp = opendir ( lobdir.c_str ( ) ) ;
   if ( dp == NULL )
      return ;
   while ( (direntp=readdir(dp)) != NULL )
   {
      if ( *(direntp->d_name) != '.' && direntp->d_type!=DT_DIR )
      {
         fileslist.push_back ( direntp->d_name ) ;
      }
   }

   closedir( dp ) ;
   
}

CSdbLob::~ CSdbLob( )
{
    
}

int CSdbLob::Connect ( )
{
   pSrvAddr = getCharPtr( m_strSrvAddr ) ;
   const char* delim = ":" ;
   char* pSrv = NULL ;
   char* pHost = strtok_r ( pSrvAddr, delim, &pSrv ) ;

   rc = sdbConnect ( pHost, pSrv, "", "", &m_conn ) ;
   if ( SDB_OK != rc )
   {
      printf ( "Fail to connect to %s, rc = %d\n", pSrvAddr, rc ) ;
      return 1 ;
   }
  
   if ( 1 == init(  ) )
   {
      sdbDisconnect ( m_conn ) ;
      sdbReleaseCollection ( m_conn ) ;
      return 1 ;
   }
   
   
   return 0 ;
}

int CSdbLob::Putfile ( int id  ) 
{
   string infile = ChooseFile( id ) ;

   FILE* fp = fopen( infile.c_str(), "r+b" );
   if ( fp == NULL )
   {
      printf( "failed open file %s\n", infile.c_str( ) ) ;
      return 1 ;
   }

   bson_oid_t oid ;
   bson obj ;
   bson_init ( &obj ) ;
   bson_oid_gen( &oid ) ;
   bson_append_oid( &obj, "objectid", &oid ) ;
   bson_append_int( &obj, "id", id ) ;
   bson_append_finish_object( &obj ) ;

   rc = sdbInsert ( m_cl, &obj ) ;
   bson_destroy( &obj ) ;
   if ( SDB_OK != rc )
   {
      printf( "sdbInsert failed!id=%d,  rc=%d\n", id, rc ) ;
      fclose( fp ) ;
      return 1 ;
   }
   
   rc = sdbOpenLob( m_cl, &oid, SDB_LOB_CREATEONLY, &m_lob ) ;
   if ( SDB_OK != rc )
   {
      printf( "sdbOpenLob for writing failed! rc=%d\n", rc ) ;
      fclose( fp ) ;
      return 1 ;
   }
   
   fseek( fp, 0, 2 ) ;
   int fileSize = ftell( fp ) ;
   fseek( fp, 0, 0 ) ;

   int pos = 0 ;
   int readLen = 0 ;

   do {
      readLen = fread ( lobWriteBuf, sizeof(char),  lobSize, fp ) ;
      if ( readLen > 0 )
      {
         pos += readLen ;
         rc = sdbWriteLob ( m_lob, lobWriteBuf, readLen ) ;
         if ( SDB_OK != rc )
         {
            printf( " write lob failed! rc=%d\n", rc ) ;
            break;
         }
      }
   }while ( pos < fileSize) ;

   rc = sdbCloseLob( &m_lob ) ;
    if ( SDB_OK != rc )
   {
      printf( " close writelob failed! rc=%d\n", rc ) ;
   }

   fclose( fp ) ;

   return 0 ;
}

int CSdbLob::Getfile ( int id )
{
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson cond ;
   int getid = rand( ) % (id + 1) ;
   bson_init ( &cond ) ;
   bson_append_int( &cond, "id", getid ) ;
   bson_append_finish_object( &cond ) ;
   
   rc = sdbQuery ( m_cl, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   bson_destroy( &cond ) ;
   
   bson obj ;
   bson_init ( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   if ( SDB_OK !=rc )
   {
      printf( "failed getlob record, id=%d\n", id ) ;
      bson_destroy( &obj ) ;
      return 1 ;
   }
   bson_iterator itr ;
   bson_iterator_init( &itr,  &obj ) ;
   bson_find ( &itr, &obj, "objectid" ) ;
   bson_oid_t* oid = bson_iterator_oid ( &itr ) ;

   rc = sdbOpenLob( m_cl, oid, SDB_LOB_READ, &m_lob ) ;
   if ( SDB_OK != rc )
   {
      printf( "sdbOpenLob for reading failed! rc=%d\n", rc ) ;
      sdbCloseCursor( cursor ) ;
      bson_destroy( &obj ) ;
      return 1 ;
   }

   char outfile[30] ;
   sprintf( outfile, "%s/out_%d", outDir.c_str(), id ) ;
   FILE* fp = fopen( outfile, "w+b" ) ;
   if ( NULL == fp )
   {
      printf( "failed open file %s\n", outfile ) ;
      sdbCloseCursor( cursor ) ;
      bson_destroy( &obj ) ;
      return 1 ;
   }

   UINT32 hasRead = 0 ;
   int writeLen ;
   do{
      rc = sdbReadLob( m_lob, sizeof ( char* ) * lobSize, lobReadBuf, &hasRead ) ;
      if ( SDB_OK != rc )
      {
         printf( " read lob failed! rc=%d\n", rc ) ;
         break;
      }
      if ( hasRead > 0 )
      {
         writeLen = fwrite( lobReadBuf, 1, hasRead, fp ) ;
      }
   }while ( hasRead == 0 ) ;
   

   fclose( fp ) ;
   sdbCloseCursor( cursor ) ;
   bson_destroy( &obj ) ;
   
   
   return 0 ;
}

int CSdbLob::Mix ( int id )
{
   int mode = rand( ) % 10 ;

   if ( mode < 7 )
      return Getfile( id ) ;
   else
      return Putfile( id ) ;
}

int CSdbLob::init ( )
{
    pCSName = getCharPtr( m_dbName ) ;

   rc = sdbGetCollectionSpace( m_conn, pCSName, &m_cs ) ;
   if ( SDB_OK != rc )
   {
      printf( "failed to get collectionspace %s\n", m_dbName.c_str() ) ;
      return 1 ;
   }

   pCLName = getCharPtr( m_fullClName ) ;
   rc = sdbGetCollection ( m_conn, pCLName, &m_cl ) ;
   if ( SDB_OK != rc )
   {
      printf( "failed to get collection %s\n", m_fullClName.c_str() ) ;
      return 1 ;
   }

   lobWriteBuf = (char* )malloc ( sizeof ( char* ) * lobSize ) ;
   lobReadBuf = (char* )malloc ( sizeof (char* ) * lobSize ) ;
   if ( NULL == lobWriteBuf || NULL == lobReadBuf )
   {
      printf( "failed to malloc memory for readBuf or writeBuf" ) ;
      return 1 ;
   }

   m_lob = NULL ;

   return 0 ;
   
}

void CSdbLob::Release ( )
{
   if ( pSrvAddr != NULL ) 
      delete pSrvAddr ;
   if ( pCSName != NULL )
      delete pCSName ;

   if ( lobReadBuf != NULL )
      free ( lobReadBuf) ;
   if ( lobWriteBuf != NULL )
      free ( lobWriteBuf ) ;
}

string CSdbLob::ChooseFile ( int id )
{
   return lobdir + "/" + fileslist[rand( ) % fileslist.size()] ;
}

char* CSdbLob::getCharPtr ( string &str )
         {
            char* cha = new char[str.size ( ) + 1] ;
            memcpy ( cha, str.c_str( ), str.size( ) + 1 ) ;

            return cha ;
         }

IClient* CreateClient(const char * pSrvAddr, const char * pDbName, int startID)
{
   return new CSdbLob ( pSrvAddr, pDbName, startID ) ;
}

