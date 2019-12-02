/************************************
*@Description: 修改AutoSplit属性值
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15067
**************************************/

main(); 

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" ); 
      return; 
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true ); 
   if( 1 === allGroupName.length )
   {
      println( "--least two groups" ); 
      return; 
   }
   println( "---begin test---" ); 
   var csName = CHANGEDPREFIX + "_cs_15067"; 
   var clName = CHANGEDPREFIX + "_cl_15067"; 
   var domainName = CHANGEDPREFIX + "_domain_15067"; 
   var group1 = allGroupName[0]; 
   var group2 = allGroupName[1]; 
   
   commDropDomain( db, domainName ); 
   var domain = commCreateDomain( db, domainName, [group1, group2], {AutoSplit:false} ); 
   db.createCS( csName, {Domain:domainName} )
   //commCreateCL( db, csName, clName, 1, false, true, false, "create CL in the begin" ); 
   var clOption = {ShardingKey:{a:1}, ShardingType:'hash'}; 
   var cl = commCreateCLByOption( db, csName, clName, clOption, true, true ); 
   
   for( i = 0; i < 5000; i++ )
   {
      cl.insert( {a:i, b:"sequoiadh test split cl alter option"} ); 
   }
   
   println( "---domain alter AutoSplit to true---" ); 
   domain.setAttributes( {AutoSplit: true} )
   checkDomain( db, domainName, [group1, group2], true, undefined ); 
   
   checkCL( [group1, group2], csName, clName ); 
   
   db.dropCS( csName ); 
   commDropDomain( db, domainName ); 
   println( "---end the test---" ); 
}

function checkCL( groupNames, csName, clName )
{
   try
   {
      for( var i in groupNames )
      {
         var groupName = groupNames[i]; 
         var nodeName = db.getRG( groupName ).getMaster(); 
         var sdb = new Sdb( nodeName ); 
         sdb.getCS( csName ).getCL( clName ); 
      }
      throw "GET_CSCL_ERR"; 
   }
   catch( e )
   {
      if( e !== -34 && e !== -23 )
      {
         throw buildException( "checkCL", e, "check cl no split", "-34||-23", e ); 
      }
   }
}

function checkDomain( db, domainName, expGroups, expAutoSplit, expAutoRebalance )
{
   var domainMsg = db.listDomains( {Name:domainName} ).current().toObj(); 
   actGroups = domainMsg.Groups; 
   actAutoSplit = domainMsg.AutoSplit; 
   actAutoRebalance = domainMsg.AutoRebalance; 
   
   if( actGroups.length !== expGroups.length )
   {
      throw buildException( "checkDomain1", "check domain group error", "check domain group error", expGroups.length, actGroups.length ); 
   }
   for( var i in expGroups )
   {
      var groupName = actGroups[i].GroupName; 
      if( expGroups.indexOf( groupName )=== -1 )
      {
         throw buildException( "checkDomain2", "check domain groupName error", "check domain groupName error", expGroups, actGroups[i].GroupName ); 
      }
   }
   
   if( actAutoSplit !== expAutoSplit )
   {
      throw buildException( "checkDomain3", "check AutoSplit error", "check AutoSplit error", expAutoSplit, actAutoSplit ); 
   }
   
   if( actAutoRebalance !== expAutoRebalance )
   {
      throw buildException( "checkDomain4", "check expAutoRebalance error", "check expAutoRebalance error", expAutoRebalance, actAutoRebalance ); 
   }
}

