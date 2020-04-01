/****************************************************
@description:	create/drop index by SQL, verify parameters
         testlink cases:   seqDB-7414/7415
@input:        1 insert into records
               2 create/drop index, the name cotains invalid characters, errorno: -6
               3 create/drop index, the name is " ", errorno: -6
               4 create index, the indexKey without fields, errorno: -195
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_7414", testConf.clOpt = { ReplSize: 0 };

main( test );

function test ()
{
   indexName = CHANGEDPREFIX + "_ix";

   for( var i = 0; i < 20; i++ )
   {
      db.execUpdate( "insert into " + testConf.csName + "." + testConf.clName + " (name,age) values (\"Tom\"," + i + ")" );
   }

   var aa = Array( "$", ".", "a." );
   for( var i = 0; i < aa.length; ++i )
   {
      try
      {
         var specIndexName = aa[i] + CHANGEDPREFIX;
         db.execUpdate( "create index " + specIndexName + " on " + testConf.csName + "." + testConf.clName + " (name)" );
      }
      catch( e )
      {
         if( e.message != -6 )
         {
            throw new Error( e );
         }
      }
   }

   for( var i = 0; i < aa.length; ++i )
   {
      try
      {
         var specIndexName = aa[i] + CHANGEDPREFIX;
         db.execUpdate( "drop index " + specIndexName + " on " + testConf.csName + "." + testConf.clName );
      }
      catch( e )
      {
         if( e.message != -47 )
         {
            throw new Error( e );
         }
      }
   }

   try
   {
      db.execUpdate( "create index " + "  on " + testConf.csName + "." + testConf.clName + " (name)" );
   }
   catch( e )
   {
      if( e.message != -195 )
      {
         throw new Error( e );
      }
   }

   try
   {
      db.execUpdate( "drop index " + "  on " + testConf.csName + "." + testConf.clName );
   }
   catch( e )
   {
      if( e.message != -195 )
      {
         throw new Error( e );
      }
   }

   try
   {
      db.execUpdate( "create index " + indexName + " on " + testConf.csName + "." + testConf.clName );
   }
   catch( e )
   {
      if( e.message != -195 )
      {
         throw new Error( e );
      }
   }
}