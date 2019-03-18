/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18002:主子表/分区表，插入数据索引键冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main()
{  
   if( commIsStandalone( db ) )
   {
   	println(" Deploy mode is standalone!");
		return;
   }
   
	println("\n---Begin to run test");
   var mainCLName = "insertFlag_mcl18002" ;
   var subCLName  = "insertFlag_scl18002" ;
	var cs = db.getCS( COMMCSNAME );
	
	commDropCL( db, COMMCSNAME, mainCLName, true, true, "Failed to drop maincl in the pre-condition." );
	commDropCL( db, COMMCSNAME, subCLName, true, true, "Failed to drop subcl in the pre-condition." );
               
   // ready cl, the subcl is sharding cl
   var cl = cs.createCL( mainCLName, {ShardingKey:{a:1},IsMainCL:true} );  
   cs.createCL( subCLName, {ShardingKey:{a:1}} );
   cl.attachCL( csName +"."+ subCLName, {LowBound:{a:1},UpBound:{a:100}} );
	cl.createIndex( "idx", {a:1}, true, true );
	
   // test
   cl.insert( {a:1} );
   // SDB_INSERT_CONTONDUP
	println("\n---Begin to insert, flag[SDB_INSERT_CONTONDUP]");
	var recsArray = [{a:1,b:1},{a:2}];
	var rc = cl.insert( recsArray, SDB_INSERT_CONTONDUP );
	var expRecs = [{"a":1},{"a":2}];
   checkRecords( cl, expRecs );
   
   // SDB_INSERT_REPLACEONDUP
	println("\n---Begin to insert, flag[SDB_INSERT_REPLACEONDUP]");
	var recsArray = [{a:1,b:2},{a:3}];
	var rc = cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
	var expRecs = [{"a":1,"b":2},{"a":2},{"a":3}];
   checkRecords( cl, expRecs );
   
   cleanCL( mainCLName );
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