/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18012:批量插入存在多个索引键冲突的记录
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main();

function main()
{  
   println("\n---Begin to run test");
   var clName = "insertFlag_18012";
   var idxName = "idx";   
   var cl = readyCL( clName );
   cl.createIndex( idxName, {a:1}, true, true );
   cl.insert( [{a:1},{a:2}] );
   
   // test
   println("\n---Begin to insert, flag[SDB_INSERT_REPLACEONDUP]");
   var recsArray = [{a:1,b:1},{a:1,b:2},{a:2,c:1},{a:3}];
   var rc = cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
   var expRecs = [{"a":1,"b":2},{"a":2,"c":1},{"a":3}];
   checkRecords( cl, expRecs );
   
   cleanCL( clName );
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