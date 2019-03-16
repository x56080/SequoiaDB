/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18010:插入数据_id索引键冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main()
{  
	println("\n---Begin to run test");
	var clName = "insertFlag_18010";
	var idxName = "idx";	
   var cl = readyCL( clName );
   cl.insert( {_id:1} );
   
   // test
   // SDB_INSERT_CONTONDUP
	println("\n---Begin to insert, flag[SDB_INSERT_CONTONDUP]");
	var recsArray = [{_id:1,c:1},{_id:2}];
	cl.insert( recsArray, SDB_INSERT_CONTONDUP );
	var expRecs = [{"_id":1},{"_id":2}];
   checkRecords( cl, expRecs );
   
   // SDB_INSERT_REPLACEONDUP
	println("\n---Begin to insert, flag[SDB_INSERT_REPLACEONDUP]");
	var recsArray = [{_id:3},{_id:1,c:2},{_id:4}];
	cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
	var expRecs = [{"_id":1,"c":2},{"_id":2},{"_id":3},{"_id":4}];
   checkRecords( cl, expRecs );
   
   cleanCL( clName );
}

function checkRecords( cl, recs ) 
{
   var rc = cl.find( {} ).sort({a:1} );
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