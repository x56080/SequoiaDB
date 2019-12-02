/*******************************************************************************
*@Description : seqDB-7555:rest_输入strict格式，查询显示
seqDB-7557:rest_strict格式的边界值校验
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main(); 

function main()
{
   try
   {
      var csName = COMMCSNAME; 
      var clName = COMMCLNAME; 
      
      var clObj = new Collection( csName, clName, {ReplSize:0} ); 
      var cl = clObj.create(); 
      
      var recsInBoundary = []; 
      recsInBoundary.push( {_id:2, a:{$numberLong:"-9223372036854775808"}} ); //long min
      recsInBoundary.push( {_id:3, a:{$numberLong: "9223372036854775807"}} ); //long max
      insert( csName, clName, recsInBoundary ); 
      
      var recsOutBoundary = []; 
      recsOutBoundary.push( {_id:1, a:{$numberLong:"-9223372036854775809"}} ); //out of min value
      recsOutBoundary.push( {_id:4, a:{$numberLong: "9223372036854775808"}} ); //out of max value
      insertErrorRec( csName, clName, recsOutBoundary ); 
      
      var expRecs = []; 
      expRecs.push( "{ \"_id\": 2, \"a\": -9223372036854775808 }" ); 
      expRecs.push( "{ \"_id\": 3, \"a\": 9223372036854775807 }" ); 
      checkByQuery( csName, clName, expRecs ); 
   }
   catch( e )
   {
      throw e; 
   }
}

function insert( csName, clName, recs )
{
   println( "---begin to insert" ); 
   
   for( var i in recs )
   {
      var recStr = JSON.stringify( recs[i] ); 
      
      var curlPara = [ 'cmd=insert', 
      'name=' + csName + '.' + clName, 
      'insertor=' + recStr ]; 
      var expErrno = 0; 
      var curlInfo = runCurl( curlPara, expErrno ); 
   }
}

function insertErrorRec( csName, clName, recs )
{
   println( "---begin to insert error record" ); 
   
   for( var i in recs )
   {
      var recStr = JSON.stringify( recs[i] ); 
      
      var curlPara = [ 'cmd=insert', 
      'name=' + csName + '.' + clName, 
      'insertor=' + recStr ]; 
      var expErrno = -6; 
      var curlInfo = runCurl( curlPara, expErrno ); 
   }
}

function checkByQuery( csName, clName, expRecs )
{
   println( "---begin to query" ); 
   
   //run query command
   var curlPara = [ 'cmd=query', 
   'name=' + csName + '.' + clName, 
   'sort={_id:1}' ]; 
   var expErrno = 0; 
   var curlInfo = runCurl( curlPara, expErrno ); 
   var actRecs = curlInfo.rtnJsn; 
   
   //check count
   if( actRecs.length !== expRecs.length )
   {
      throw buildException( "checkByQuery(), check count", null, curlInfo.curlCommand, 
      expRecs.length, actRecs.length ); 
   }
   
   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i]; 
      var expRec = expRecs[i]; 
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] )!== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " +( parseInt( i )+ 1 )+ "th record, in field '" + f + "'" ); 
            println( "\nactual recs in cl= " + JSON.stringify( actRecs[i] )+ "\n\nexpect recs= " + JSON.stringify( expRecs[i] ) ); 
            throw buildException( "checkByQuery(), check fields", "rec ERROR" ); 
         }
      }
   }
   
}
