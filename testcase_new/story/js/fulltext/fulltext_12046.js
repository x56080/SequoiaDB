/************************************
*@Description: 查询接口参数验证 
*@author:      liuxiaoxuan
*@createdate:  2018.10.09
*@testlinkCase: seqDB-12046
**************************************/
function main ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_12046";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_12046";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   dbcl.insert( { "a": "testa" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   // check result
   var dbOperator = new DBOperator();
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var expResult = [{ "a": "testa" }];
   var actResult = dbOperator.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   checkResult( expResult, actResult );

   // find with wrong search command
   try
   {
      var rec = dbcl.find( { "": { "$text": { "query": { "match_all": {} } } } } ); // should fail
      rec.next();
      throw new Error( "find with wrong command" );
   }
   catch( e )
   {
      if( -6 !== e )  
      {
         throw new Error( e );
      }

   }

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;
