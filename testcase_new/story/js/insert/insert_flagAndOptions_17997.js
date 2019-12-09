/******************************************************************************
*@Description : insert, test flags and options                                 
*               seqDB-17997:insert，原有基本功能验证 
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main ()
{
   println( "\n---Begin to run test" );
   var clName = "insertFlag_17997";
   var idxName = "idx";
   var cl = readyCL( clName );
   cl.createIndex( idxName, { a: 1, b: 1 }, true, true );

   // test
   insertNotSetFlag( cl );
   insertSetFlag_ReturnOid( cl );
   insertSetFlag_ContOnDup( cl );

   cleanCL( clName );
}

function insertNotSetFlag ( cl )
{
   println( "\n---Begin to insert docs, not set flag" );
   // index key not conflict
   var recs = [{ "a": 1 }, { "a": 2 }];
   cl.insert( recs );

   // index key conflict
   try
   {
      cl.insert( { a: 1, c: 1 } );
      throw "expect fail, but actual succ."
   }
   catch( e )
   {
      if( -38 !== e )
      {
         throw e;
      }
   }

   checkRecords( cl, recs );

   cl.remove();
}

function insertSetFlag_ReturnOid ( cl )
{
   println( "\n---Begin to insert docs, set flag[SDB_INSERT_RETURN_ID]/options[ReturnOID]" );
   cl.insert( { a: 1, b: 1 } );

   // index key not conflict
   var rc = cl.insert( { _id: 1, a: 1, b: 2 }, SDB_INSERT_RETURN_ID );
   if( 1 === rc.toObj()["_id"]["$oid"] )
   {
      throw new Error( "expResult is 1, actResult is" + rc.toObj()["_id"]["$oid"] );
   }

   var rc = cl.insert( { _id: 2, a: 1, b: 3 }, { ReturnOID: true } );
   if( 2 === rc.toObj()["_id"]["$oid"] )
   {
      throw new Error( "expResult is 2, actResult is" + rc.toObj()["_id"]["$oid"] );
   }

   var rc = cl.insert( { a: 1, b: 4 }, { ReturnOID: false } );
   if( null != rc && rc.toObj()["_id"] != null )
   {
      throw buildException( "checkReturnOid", null, "", "not return oid", "  " + "return oid" );
   }

   // index key conflict
   try
   {
      var rc = cl.insert( { a: 1, b: 1, c: 1 }, SDB_INSERT_RETURN_ID );
      throw "expect fail, but actual succ."
   }
   catch( e )
   {
      if( -38 != e )
      {
         throw e;
      }
   }

   var expRecs = [{ "a": 1, "b": 1 }, { "a": 1, "b": 2 }, { "a": 1, "b": 3 }, { "a": 1, "b": 4 }];
   checkRecords( cl, expRecs );

   cl.remove();
}

function insertSetFlag_ContOnDup ( cl )
{
   println( "\n---Begin to insert docs, set flag[SDB_INSERT_CONTONDUP]/options[ContOnDup]" );
   // index key not conflict
   cl.insert( [{ a: 1, b: 1 }] );

   // index key conflict
   // SDB_INSERT_CONTONDUP
   cl.insert( [{ a: 1, b: 1, c: 1 }, { a: 2 }], SDB_INSERT_CONTONDUP );
   // ContOnDup:true
   cl.insert( [{ a: 1, b: 1, c: 2 }, { a: 3 }], { ContOnDup: true } );

   // ContOnDup:false
   try
   {
      cl.insert( [{ a: 1, b: 1, c: 3 }, { a: 4 }], { ContOnDup: false } );
      throw "expect fail, but actual succ."
   }
   catch( e )
   {
      if( -38 !== e )
      {
         throw e;
      }
   }

   // insert one doc, flag: SDB_INSERT_CONTONDUP
   cl.insert( { a: 1, b: 1, c: 4 }, SDB_INSERT_CONTONDUP );
   cl.insert( { a: 5 }, SDB_INSERT_CONTONDUP );

   // insert one doc, options：ContOnDup
   cl.insert( { a: 1, b: 1, c: 5 }, { ContOnDup: true } );
   cl.insert( { a: 6 }, { ContOnDup: true } );

   cl.insert( { a: 7 }, { ContOnDup: false } );
   try
   {
      cl.insert( { a: 1, b: 1, c: 7 }, { ContOnDup: false } );
      throw "expect fail, but actual succ."
   }
   catch( e )
   {
      if( -38 !== e )
      {
         throw e;
      }
   }

   var expRecs = [{ "a": 1, "b": 1 }, { "a": 2 }, { "a": 3 }, { "a": 5 }, { "a": 6 }, { "a": 7 }];
   checkRecords( cl, expRecs );

   cl.remove();
}

function checkRecords ( cl, recs ) 
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
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
