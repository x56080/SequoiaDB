/******************************************************************************
@Description :  seqDB-15546:指定AutoIndexId:false创建hash分区表，alter为自动切分
@Modify list :  2018-8-9  xiaoni Zhao  Init
******************************************************************************/
function main()
{
   if( commIsStandalone( db ) )
   {
      println( "standalone environment!" ); 
      return; 
   }
   
   var groupNames = getGroupNames(); 
   if( groupNames.length < 2 )
   {
      println( "Only one group or standalone environment!" ); 
      return; 
   }
   
   var csName = COMMCSNAME + "_15546"; 
   var clName = COMMCLNAME + "_15546"; 
   var domName = "mydomain" + "_15546"
   var srcGroup = groupNames[0]; 
   var desGroup = groupNames[1]; 
   
   commDropDomain( db, domName ); 
   commCreateDomain( db, domName, [srcGroup, desGroup] ); 
   
   var dbcl = db.createCS( csName, {Domain:domName} ).createCL( clName, {ShardingKey:{a:1}, ShardingType:"hash", AutoIndexId:false} )
   
   //check id index not existed; 
   checkIdIndex( clName, false, csName ); 
   
   //set CL attributes
   try
   {
      dbcl.setAttributes( {AutoSplit:true} ); 
      throw "NEED_ERROR"; 
   }
   catch( e )
   {
      if( e != -279 )
      {
         throw e; 
      }
   }
   
   commDropCS( db, csName )
   commDropDomain( db, domName ); 
}

main(); 
