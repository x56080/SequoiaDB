/************************************
*@Description: Domain存在cs执行addGroups/setGroups/removeGroups修改组
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15062，seqDB-15063，seqDB-15064
**************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 2 >= allGroupName.length )
   {
      println( "--least two groups" );
      return;
   }
   println( "---begin test---" );
   var csName = CHANGEDPREFIX + "_cs_15062";
   var clName = CHANGEDPREFIX + "_cl_15062";
   var domainName = CHANGEDPREFIX + "_domain_15062";
   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   var group3 = allGroupName[2];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [group1, group2], { AutoSplit: true } );
   db.createCS( csName, { Domain: domainName } )
   //commCreateCL( db, csName, clName, 1, false, true, false, "create CL in the begin" ); 
   var clOption = { ShardingKey: { a: 1 }, ShardingType: 'hash' };
   commCreateCLByOption( db, csName, clName, clOption, true, true );

   println( "---domain add groups---" );
   domain.addGroups( { Groups: [group3] } );
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );

   println( "---domain remove groups---" );
   //delete domainGroup where cl all group
   domainRemoveGroups( domain, { Groups: [group1, group2] } );
   //delete domainGroup where cl soma group
   domainRemoveGroups( domain, { Groups: [group1, group3] } );
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );
   //delete domainGroup where cl not in group
   domain.removeGroups( { Groups: [group3] } );
   checkDomain( db, domainName, [group1, group2], true, undefined );

   println( "---domain set groups---" );
   //alter domain to all group
   domain.setAttributes( { Groups: [group1, group2, group3] } );
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );
   //alter domain where cl not in group
   domainSetAtt( domain, { Groups: [group3] } );
   //alter domain where cl not all in group
   domainSetAtt( domain, { Groups: [group1, group3] } )
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );

   db.dropCS( csName );
   commDropDomain( db, domainName );
   println( "---end the test---" );
}

function domainSetAtt ( domain, alterOption )
{
   try
   {
      domain.setAttributes( alterOption );
      throw "ALTER_ERROR";
   }
   catch( e )
   {
      if( e !== -256 )
      {
         throw buildException( "domainSetAtt", "alter domain group error", "alter domain group error", -256, e );
      }
   }
}

function domainRemoveGroups ( domain, groups )
{
   try
   {
      domain.removeGroups( groups );
      throw "ALTER_ERROR";
   }
   catch( e )
   {
      if( e !== -256 )
      {
         throw buildException( "domainRemoveGroups", "", "alter domain group error", -256, e );
      }
   }
}

function checkDomain ( db, domainName, expGroups, expAutoSplit, expAutoRebalance )
{
   var domainMsg = db.listDomains( { Name: domainName } ).current().toObj();
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
      if( expGroups.indexOf( groupName ) === -1 )
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

