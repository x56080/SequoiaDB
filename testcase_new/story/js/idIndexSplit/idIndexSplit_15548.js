/******************************************************************************
@Description :   seqDB-15548:切分任务前后，修改AutoIndexId属性
@Modify list :   2018-8-10  xiaoni Zhao  Init
******************************************************************************/
function main()
{
   if( true === commIsStandalone( db ) )
   {
      println( "Standalone environment!" ); 
      return; 
   }
   
   //get groups from sdb
   var groupNames = getGroupNames(); 
   if( ( 2 > groupNames.length ) )
   {
      println( "Only one group or standalone environment!" ); 
      return; 
   }
   
   var clName = COMMCLNAME + "_15548"; 
   
   //clean before
   commDropCL( db, COMMCSNAME, clName, true, true, "drop the CL before!" ); 
   
   var varCL = commCreateCLByOption( db, COMMCSNAME, clName, {ShardingKey:{a:1}, ShardingType:"hash", AutoIndexId:false}, true, false, "create CL" ); 
   
   //insert data
   for( var i = 0; i < 50; i++ )
   {
      varCL.insert( {a:i} ); 
   }
   
   //get expRecs
   var expRecs = varCL.find().toArray(); 
   
   //alter CL AutoIndexId:true
   varCL.alter( {AutoIndexId:true} ); 
   
   //get srcGroup
   var srcGroup = getSrcGroup( clName ); 
   
   //get desGroup
   var desGroup = getDesGroup( groupNames, srcGroup ); 
   
   //split
   varCL.split( srcGroup, desGroup, 50 ); 
   
   //check the succeed split result
   checkSplitResult( srcGroup, desGroup, clName ); 
   
   //alter CL AutoIndexId:false
   varCL.alter( {AutoIndexId:false} ); 
   
   //get expRecs
   var expRecs = getExpRecs( srcGroup, clName ); 
   
   //split
   try
   {
      varCL.split( srcGroup, desGroup, 50 ); 
      throw "NEED_ERROR"; 
   }
   catch( e )
   {
      if( e !== -279 )
      {
         throw e; 
      }
   }
   
   //check the failed split result
   checkSplitFailResult( clName, srcGroup, expRecs ); 
   
   //alter CL AutoIndexId:true
   varCL.alter( {AutoIndexId:true} ); 
   checkIdIndex( clName, true ); 
   
   //alter CL AutoIndexId:false
   varCL.alter( {AutoIndexId:false} ); 
   checkIdIndex( clName, false ); 
}

main(); 


function getExpRecs( srcGroup, clName )
{
   var db1 = db.getRG( srcGroup ).getMaster().connect(); 
   var cl = db1.getCS( COMMCSNAME ).getCL( clName ); 
   var expRecs = cl.find().sort( {a:1} ).toArray(); 
   return expRecs; 
}

function checkSplitFailResult( clName, srcGroup, expRecs )
{
   //get data from primary node
   var db1 = db.getRG( srcGroup ).getMaster().connect(); 
   var cl = db1.getCS( COMMCSNAME ).getCL( clName ); 
   var actRecs = cl.find().sort( {a:1} ).toArray(); 
   
   //check the count of collection
   if( actRecs.length !== expRecs.length )
   {
      throw "CL's count is different from that of the previous one!"; 
   }
   
   //check records
   for( var i in expRecs )
   {
      var actRec = actRecs[i]; 
      var expRec = expRecs[i]; 
      for( var j in expRec )
      {
         if( JSON.stringify( actRec[j] )!== JSON.stringify( expRec[j] ) )
         {
            println( "error occurs in " +( parseInt( i )+ 1 )+ "th record, in field '" + j + "'; " ); 
            println( "actual record =" + JSON.stringify( actRec )+ "\nexpect record =" + JSON.stringify( expRec ) ); 
            throw buildException( "checkRecs()", "recs ERROR" ); 
         }
         } 
      }
   }
