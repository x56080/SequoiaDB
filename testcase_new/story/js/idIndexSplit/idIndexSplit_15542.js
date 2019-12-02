/******************************************************************************
@Description :   seqDB-15542:指定AutoIndexId:false创建切分表，执行切分
@Modify list :   2018-8-6  xiaoni Zhao  Init
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
   
   var clName = COMMCLNAME + "_15542"; 
   
   //clean before
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" ); 
   
   //create CL
   var varCL = commCreateCLByOption( db, COMMCSNAME, clName, {ShardingKey:{a:1}, 
   ShardingType:"range", AutoIndexId:false}, true, false, "create CL" ); 
   
   //check id index not existed
   checkIdIndex( clName, false ); 
   
   //insert data
   for( var i = 1; i <= 50; i++ )
   {
      varCL.insert( {a:i} ); 
   }
   
   //get expRecs
   var expRecs = varCL.find().toArray(); 
   
   //get srcGroup
   var srcGroup = getSrcGroup( clName ); 
   
   //get desGroup
   var desGroup = getDesGroup( groupNames, srcGroup ); 
   
   //split
   try
   {
      varCL.split( srcGroup, desGroup, {partition:1}, {partition:26} ); 
      throw "NEED_ERROR"; 
   }
   catch( e )
   {
      if( e !== -279 )
      {
         throw e; 
      }
   }
   
   //check catalog information
   checkCataInfo( clName, srcGroup, 1 ); 
   
   //check data
   checkData( expRecs, clName ); 
}

main(); 

