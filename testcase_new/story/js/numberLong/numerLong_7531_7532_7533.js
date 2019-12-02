/*******************************************************************************
*@Description : seqDB-7531:shell_切分范围使用numberLong
seqDB-7533:shell_切分的数据类型为numberLong
seqDB-7532:shell_挂载子表的attach范围使用numberLong
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main(); 

function main()
{
   try
   {
      if( commIsStandalone( db ) )
      {
         println( " Deploy mode is standalone!" ); 
         return; 
      }
      if( commGetGroupsNum( db )< 2 )
      {
         println( "This testcase needs at least 2 groups to split cl!" ); 
         return; 
      }
      
      var csName = COMMCSNAME; 
      var mainclName = COMMCLNAME + "maincl"; 
      var subclName = COMMCLNAME + "subcl"; 
      
      var groups = select2RG(); 
      
      //create main cl
      var clOpt = {IsMainCL:true, ShardingKey:{mk:1}, ShardingType:"range", ReplSize:0}; 
      var maincl = new Collection( csName, mainclName, clOpt ).create(); 
      
      //create sub cl( splited )
      var opt = {ShardingKey:{sk:1}, ShardingType:"range", Group:groups.srcRG, ReplSize:0}; 
      new Collection( csName, subclName, opt ).create(); 
      
      istNumberRecs( csName, subclName ); 
      splitByLong( csName, subclName, groups.srcRG, groups.tgtRG ); 
      
      //attch subcl
      attachByLong( maincl, csName, subclName ); 
   }
   catch( e )
   {
      throw e; 
   }
}

function istNumberRecs( csName, clName )
{
   println( "---begin to insert long/int/float records" ); 
   var recs = []; 
   recs.push( {sk:{$numberLong:"9223372036854775807"}} ); 
   recs.push( {sk:{$numberLong:"1"}} ); 
   recs.push( {sk:2} ); 
   recs.push( {sk:1.95} ); 
   
   db.getCS( csName ).getCL( clName ).insert( recs ); 
}

function splitByLong( csName, clName, srcRG, tgtRG )
{
   println( "---begin to split" ); 
   db.getCS( csName ).getCL( clName ).
   split( srcRG, tgtRG, {sk:{$numberLong:"9223372036854775001"}}, {sk:MaxKey()} ); 
   
   println( "---begin to check split result" ); 
   var srcNode = db.getRG( srcRG ).getMaster(); 
   var tgtNode = db.getRG( tgtRG ).getMaster(); 
   
   var srcRC = new Sdb( srcNode ).getCS( csName ).getCL( clName ).find().sort( {sk:1} ); 
   var tgtRC = new Sdb( tgtNode ).getCS( csName ).getCL( clName ).find().sort( {sk:1} ); 
   checkRec( srcRC, [ {sk:1}, {sk:1.95}, {sk:2} ] ); 
   checkRec( tgtRC, [ {sk:{$numberLong:"9223372036854775807"}} ] ); 
}

function attachByLong( maincl, csName, subclName )
{
   println( "---begin to attach cl" ); 
   maincl.attachCL( csName + "." + subclName, {LowBound:{mk:MinKey()}, 
   UpBound: {mk:{$numberLong:"9223372036854775806"}}} ); 
   
   println( "---begin to insert record out of subcl's range" ); 
   try
   {
      var rec = {mk:{$numberLong:"9223372036854775807"}}; 
      maincl.insert( rec ); 
      throw "did not throw error"; 
   }
   catch( e )
   {
      if( e !== -135 )
      {
         throw buildException( "check return code", "", 
         'cl.insert( {a:{$numberLong:"9223372036854775807"}} )', 
         "throw -135", e ); 
      }
   }
   
   println( "---begin to insert record in subcl's range" ); 
   var rec = {mk:{$numberLong:"9223372036854775805"}}; 
   maincl.insert( rec ); 
   var rc = maincl.find( rec ); 
   checkRec( rc, [rec] ); 
}
