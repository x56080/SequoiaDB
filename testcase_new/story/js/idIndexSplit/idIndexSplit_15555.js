/******************************************************************************
@Description :   seqDB-15555:同时修改AutoIndexId及AutpSplit
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
   
   var clName = COMMCLNAME + "_15555"; 
   
   //clean before
   commDropCL( db, COMMCSNAME, clName, true, true, "drop the CL before!" ); 
   
   var varCL = commCreateCLByOption( db, COMMCSNAME, clName, {ShardingKey:{a:1}, ShardingType:"hash", AutoIndexId:false}, true, false, "create CL" ); 
   
   //insert data
   for( var i = 0; i < 50; i++ )
   {
      varCL.insert( {a:i} ); 
   }
   
   //alter CL AutoIndexId:true, AutoSplit:true; 
   try
   {
      varCL.alter( {AutoIndexId:true, AutoSplit:true} ); 
   }
   catch( e )
   {
      throw e; 
   }
   
   //alter CL AutoIndexId:false, AutoSplit:false; 
   try
   {
      varCL.alter( {AutoIndexId:false, AutoSplit:false} ); 
   }
   catch( e )
   {
      if( e !== -32 )
      {
         throw e; 
      }
   }
   
   //alter CL AutoIndexId:true, AutoSplit:true; 
   try
   {
      varCL.alter( {AutoIndexId:true, AutoSplit:true} ); 
   }
   catch( e )
   {
      if( e !== -32 )
      {
         throw e; 
      }
   }
   
   //alter CL AutoIndexId:true, AutoSplit:false; 
   try
   {
      varCL.alter( {AutoIndexId:true, AutoSplit:false} ); 
   }
   catch( e )
   {
      if( e !== -32 )
      {
         throw e; 
      }
   }
   
   //alter CL AutoIndexId:false, AutoSplit:true; 
   try
   {
      varCL.alter( {AutoIndexId:false, AutoSplit:true} ); 
   }
   catch( e )
   {
      if( e !== -32 )
      {
         throw e; 
      }
   }
}

main(); 
