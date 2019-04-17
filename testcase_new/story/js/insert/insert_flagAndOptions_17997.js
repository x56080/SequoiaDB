/******************************************************************************
*@Description : insert, test flags and options                                 
*               seqDB-17997:insert，原有基本功能验证 
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main()
{  
   println("\n---Begin to run test");
   var clName = "insertFlag_17997";
   var idxName = "idx";   
   var cl = readyCL( clName );
   cl.createIndex( idxName, {a:1, b:1}, true, true );
   
   // test
   insertNotSetFlag( cl );
   insertSetFlag_ReturnOid( cl );
   insertSetFlag_ContOnDup( cl );
   
   cleanCL( clName );
}

function insertNotSetFlag( cl )
{
   println("\n---Begin to insert docs, not set flag");
   // index key not conflict
   var recs = [{"a":1},{"a":2}];
   cl.insert( recs );
   
   // index key conflict
   try
   {
      cl.insert( {a:1,c:1} );
      throw "expect fail, but actual succ." 
   }
   catch(e)
   {
      if( -38 !== e )
      {
         throw e;
      }
   }
   
   checkRecords( cl, recs );
   
   cl.remove();
}

function insertSetFlag_ReturnOid( cl )
{
   println("\n---Begin to insert docs, set flag[SDB_INSERT_RETURN_ID]");
   cl.insert({a:1,b:1});
   
   // index key not conflict
   var rc = cl.insert( {a:1,b:2}, SDB_INSERT_RETURN_ID );
   if( null === rc )
   {
      throw buildException( "insertSetFlag_ReturnOid", null, "", "return oid", "  " + null );
   } 
   
   // index key conflict
   try
   {
      var rc = cl.insert( {a:1,b:1,c:1}, SDB_INSERT_RETURN_ID );
      throw "expect fail, but actual succ." 
   }
   catch(e)
   {
      if( -38 != e )
      {
         throw e;
      }
   }
   
   var expRecs = [{"a":1,"b":1},{"a":1,"b":2}];
   checkRecords( cl, expRecs );
   
   cl.remove();
}

function insertSetFlag_ContOnDup( cl )
{
   println("\n---Begin to insert docs, set flag[SDB_INSERT_CONTONDUP]");
   // index key not conflict
   cl.insert([{a:1,b:1}]);
   
   // index key conflict
   // SDB_INSERT_CONTONDUP
   cl.insert( [{a:1,b:1,c:1},{a:2}], SDB_INSERT_CONTONDUP );
   
   // insert one doc, flag: SDB_INSERT_CONTONDUP
   cl.insert( {a:1,b:1,c:2}, SDB_INSERT_CONTONDUP );  
   cl.insert( {a:3}, SDB_INSERT_CONTONDUP );   
   
   var expRecs = [{"a":1,"b":1},{"a":2},{"a":3}];
   checkRecords( cl, expRecs );
   
   cl.remove();
}

function checkRecords( cl, recs ) 
{
   var rc = cl.find( {}, {_id:{$include:0}} ).sort({a:1} );
   var rcRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      rcRecs.push( tmpRecs.toObj() );
   }   
   
   var expRecs = JSON.stringify( recs );
   var actRecs = JSON.stringify( rcRecs );
   if( expRecs !== actRecs )
   {
      throw buildException( "checkResult", null, "", expRecs, "  " + actRecs );
   }
}