/*******************************************************************************
*@Description : seqDB-7556:rest_strict格式的参数校验
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
      
      var recs = [ {a:{$numberLong: "123a" }} ]; 
      istErrFormat( csName, clName, recs ); 
      
      var recs = [ {a:{$numberLong:"1.1"}} ]; 
      istErrFormat( csName, clName, recs ); 
      
      checkByQuery( csName, clName, [] )
   }
   catch( e )
   {
      throw e; 
   }
}

function istErrFormat( csName, clName, recs )
{
   println( "---begin to insert error format: " + JSON.stringify( recs ) ); 
   
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
   'sort={a:1}' ]; 
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
