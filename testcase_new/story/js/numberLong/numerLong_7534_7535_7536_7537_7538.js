/*******************************************************************************
*@Description : seqDB-7534:shell_用普通方式查询numberLong数据
seqDB-7535:shell_使用匹配符查询numberLong类型
seqDB-7536:shell_使用选择符查询numberLong类型
seqDB-7537:shell_使用更新符更新numberLong类型
seqDB-7538:shell_索引值为numberLong类型
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main(); 

function main()
{
   try
   {
      var csName = COMMCSNAME; 
      var clName = COMMCLNAME; 
      var indexName = "idx1"; 
      
      var clObj = new Collection( csName, clName, {ReplSize:0} ); 
      var cl = clObj.create(); 
      clObj.createIndex( indexName, {a:1} ); 
      
      istAndQuery( cl, indexName ); 
   }
   catch( e )
   {
      throw e; 
   }
}

function istAndQuery( cl, indexName )
{
   println( "---begin to insert records" ); 
   var recs = []; 
   var longVal = -9007199254740000; 
   recs.push( {a:{$numberLong:longVal.toString()}} ); 
   recs.push( {a:9007199254740992} ); 
   cl.insert( recs ); 
   
   println( "---begin to query without $numberLong" ); 
   var rc = cl.find( {a:longVal} ); 
   var expRecs = [ {a:longVal} ]; 
   checkRec( rc, expRecs ); 
   checkExplain( rc, indexName ); 
   
   println( "---begin to query with $type" ); 
   var rc = cl.find( {a:{$type:1, $et:18}} ); 
   var expRecs = [ {a:longVal} ]; 
   checkRec( rc, expRecs ); 
   
   println( "---begin to query with $subtract" ); 
   var rc = cl.find( {a:{$type:1, $et:18}}, {a:{$subtract:10}} ); 
   var expRecs = [ {a:( longVal-10 )} ]; 
   checkRec( rc, expRecs ); 
   
   println( "---begin to query with $cast" ); 
   var rc = cl.find( {a:9007199254740992}, {a:{$cast:"int64"}} ); 
   var expRecs = [ {a:{$numberLong:"9007199254740992"}} ]; 
   checkRec( rc, expRecs ); 
   
   println( "---begin to update with $inc" ); 
   cl.update( {$inc:{a:1}}, {a:{$lt:0}} ); 
   var rc = cl.find( {a:{$lt:0}} ); 
   var expRecs = [ {a:( longVal + 1 )} ]; 
   checkRec( rc, expRecs ); 
}
