/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18000:批量插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据不冲突 
*               seqDB-18001:批量插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main()
{  
	println("\n---Begin to run test");
	var clName = "insertFlag_18000";
	var idxName = "idx";	
   var cl = readyCL( clName );
	cl.createIndex( idxName, {a:1, b:1}, true, true );
   cl.insert( {a:1,b:1} );
   
   // key not conflict
	println("\n---Begin to insert, key not conflict");
	var recsArray = [{c:1},{a:2,c:1},{a:2,b:1,c:1}];
	cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
	var expRecs = [{"c":1},{"a":1,"b":1},{"a":2,"c":1},{"a":2,"b":1,"c":1}];
   checkRecords( cl, expRecs );
   
   // key conflict
	println("\n---Begin to insert, key conflict");
	var firstRecord = {c:2};
	var middRecord  = {a:2,c:3};
	var lastRecord  = {a:2,b:1,c:5};
	var recsArray   = [firstRecord, middRecord, lastRecord];
	keyConflict( cl, recsArray );	
	
   // first record conflict
	println("   first record conflict.");
	var recsArray = [ firstRecord,{a:3} ];
	cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
	
	// middle record conflict
	println("   middle record conflict.");
	var recsArray = [{a:4}, middRecord, {a:4,b:1}];
	cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
	
	// last record conflict	
	println("   last record conflict.");
	var recsArray = [ {a:5}, lastRecord ];
	cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
	
	var expRecs = [{"c":2},{"a":1,"b":1},{"a":2,"c":3},{"a":2,"b":1,"c":5},{"a":3},{"a":4},{"a":4,"b":1},{"a":5}];
   checkRecords( cl, expRecs );
   
   cleanCL( clName );
}

function keyConflict( cl, recsArray )
{
	println("\n---Begin to insert, key conflict.");
	// key conflict, not set flag
	for( var i = 0; i < recsArray.length; i++ )
	{
	   try
	   {
	      cl.insert( recsArray[i] );
         throw "expect fail, but actual succ."
	   }
      catch(e)
      {
         if( -38 !== e )
         {
      	   throw e;
      	}
      }
	}
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