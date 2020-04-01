/****************************************************
@description:	create/drop index, basic case
         testlink cases:   seqDB-7413/7415
@input:        1 insert into records
               2 drop index when it does not exist, errorno: -47
               3 create/drop nomal index, success
               4 create the same index repeat, errorno: -247
               5 create/drop index, the indexKey contains two fields, success
               6 create/drop unique index, success
               7 drop the same index repeat, errorno: -47       
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_7413", testConf.clOpt = { ReplSize: 0 };

main( test );

function test ()
{
   indexName = CHANGEDPREFIX + "_ix";

   for( var i = 0; i < 20; i++ )
   {
      db.execUpdate( "insert into " + testConf.csName + "." + testConf.clName + " (name,age) values (\"Tom\"," + i + ")" );
   }

   try
   {
      db.execUpdate( "drop index " + indexName + " on " + testConf.csName + "." + testConf.clName );
   }
   catch( e )
   {
      if( e.message != -47 )
      {
         throw new Error( e );
      }
   }

   db.execUpdate( "create index " + indexName + " on " + testConf.csName + "." + testConf.clName + " (age desc)" );

   try
   {
      db.execUpdate( "create index " + indexName + " on " + testConf.csName + "." + testConf.clName + " (age desc)" );
   }
   catch( e )
   {
      if( e.message != -247 )
      {
         throw new Error( e );
      }
   }

   db.execUpdate( "drop index " + indexName + " on " + testConf.csName + "." + testConf.clName );

   db.execUpdate( "create index " + indexName + " on " + testConf.csName + "." + testConf.clName + " (age,name)" );

   db.execUpdate( "drop index " + indexName + " on " + testConf.csName + "." + testConf.clName );

   db.execUpdate( "create unique index " + indexName + " on " + testConf.csName + "." + testConf.clName + " (age)" );

   db.execUpdate( "drop index " + indexName + " on " + testConf.csName + "." + testConf.clName );

   try
   {
      db.execUpdate( "drop index " + indexName + " on " + testConf.csName + "." + testConf.clName );
   }
   catch( e )
   {
      if( e.message != -47 )
      {
         throw new Error( e );
      }
   }

}