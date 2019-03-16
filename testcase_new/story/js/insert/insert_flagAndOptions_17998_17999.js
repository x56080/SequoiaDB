/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-17998:单条插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据不冲突 
*               seqDB-17999:单条插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main()
{  
	println("\n---Begin to run test");
	var clName = "insertFlag_17998";
	var idxName = "idx";	
   var cl = readyCL( clName );
	cl.createIndex( idxName, {a:1, b:1}, true, true );
   cl.insert( {a:1,b:1} );
   
   // key not conflict
	var recsArray = [{c:1},{a:2,c:2},{a:3,b:3,c:3}];
	keyNotConflict( cl, recsArray );
	var expRecs = [{"c":1},{"a":1,"b":1},{"a":2,"c":2},{"a":3,"b":3,"c":3}];
   checkRecords( cl, expRecs );
   
   // key conflict
	var recsArray = [{c:2},{a:2,c:3},{a:3,b:3,c:4}];
	keyConflict( cl, recsArray );	
	var expRecs = [{"c":2},{"a":1,"b":1},{"a":2,"c":3},{"a":3,"b":3,"c":4}];
   checkRecords( cl, expRecs );
   
   cleanCL( clName );
}

function keyNotConflict( cl, recsArray )
{
	println("\n---Begin to insert, key not conflict.");
	for( var i = 0; i < recsArray.length; i++ )
	{
	   cl.insert( recsArray[i], SDB_INSERT_REPLACEONDUP );
	}
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
	
	// key conflict, set flag[SDB_INSERT_REPLACEONDUP]
	for( var i = 0; i < recsArray.length; i++ )
	{
	   cl.insert( recsArray[i], SDB_INSERT_REPLACEONDUP );
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