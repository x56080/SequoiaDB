/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18002:主子表/分区表，插入数据索引键冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }

   println( "\n---Begin to run test" );
   var mainCLName = "mcl18002";
   var subCLName = "scl18002";
   var cs = db.getCS( COMMCSNAME );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Failed to drop maincl in the pre-condition." );
   commDropCL( db, COMMCSNAME, subCLName, true, true, "Failed to drop subcl in the pre-condition." );

   // ready cl, the subcl is sharding cl
   var cl = cs.createCL( mainCLName, { ShardingKey: { a: 1 }, IsMainCL: true } );
   cs.createCL( subCLName, { ShardingKey: { a: 1 } } );
   cl.attachCL( csName + "." + subCLName, { LowBound: { a: 1 }, UpBound: { a: 100 } } );
   cl.createIndex( "idx", { a: 1 }, true, true );

   // test
   cl.insert( { a: 1 } );
   // SDB_INSERT_CONTONDUP
   println( "\n---Begin to insert, flag[SDB_INSERT_CONTONDUP]" );
   var recsArray = [{ a: 1, b: 1 }, { a: 2 }];
   cl.insert( recsArray, SDB_INSERT_CONTONDUP );
   var expRecs = [{ "a": 1 }, { "a": 2 }];
   checkRecords( cl, expRecs );

   // SDB_INSERT_REPLACEONDUP
   println( "\n---Begin to insert, flag[SDB_INSERT_REPLACEONDUP]" );
   var recsArray = [{ a: 1, b: 2 }, { a: 3 }];
   cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
   var expRecs = [{ "a": 1, "b": 2 }, { "a": 2 }, { "a": 3 }];
   checkRecords( cl, expRecs );

   // SDB_INSERT_RETURN_ID
   println( "\n---Begin to insert, flag[SDB_INSERT_RETURN_ID]" );
   var rc = cl.insert( { a: 1, b: 3 }, { ReturnOID: true, ReplaceOnDup: true } );
   if( null === rc )
   {
      throw buildException( "insertSetFlag_ReturnOid", null, "", "return oid", "  " + null );
   }
   var expRecs = [{ "a": 1, "b": 3 }, { "a": 2 }, { "a": 3 }];
   checkRecords( cl, expRecs );

   cleanCL( mainCLName );
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

function checkReturnOid ( cl, rc, recs )
{
   var expRC = cl.find( recs ).sort( { a: 1 } );
   var expOid = expRC.next().toObj()["_id"]["$oid"];
   var actOid = rc.toObj()["_id"][0]["$oid"];
   if( expOid !== actOid )
   {
      throw buildException( "checkReturnOid", null, "", expOid, "  " + actOid );
   }
}