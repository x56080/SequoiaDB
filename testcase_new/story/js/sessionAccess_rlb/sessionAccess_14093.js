/******************************************************************************
@discretion:seqDB-14093:设置会话访问属性，指定preferedinstance为instanceid存在节点和[M/S/A/m/s/a/-M/-S/-A/-m/-s/-a]，preferedinstanceMode覆盖不同值 
@author: 2020-4-9 zhaoxiaoni  Init
******************************************************************************/
main();

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var nodeNum = 3;
   var groupName = "rg_14093";
   var instanceidList = [ 30, 124, 8 ];
   var clName = CHANGEDPREFIX + "_14093";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeNames = createRGAndNodes( db, groupName, hostName, nodeNum, instanceidList );
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 });
   insertData( cl );

   var expAccessNodes = [ nodeNames[0] ];
   //SEQUOIADBMAINSTREAM-5283待开发修改问题单后此用例需要整体进行优化及调试
   var options = { PreferedInstance: [124, 8, 30, "M"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   var options = { PreferedInstance: [124, 8, 30, "M"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "m"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "m"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = [ nodeNames[1], nodeNames[2] ];
   options = { PreferedInstance: [124, 8, 30, "S"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );
   options = { PreferedInstance: [124, 8, 30, "s"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes.pop();
   options = { PreferedInstance: [ 30, 124, 8, "S"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [ 30, 124, 8, "s"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = nodeNames;
   options = { PreferedInstance: [124, 8, 30, "A"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "a"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-M"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-m"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );
   options = { PreferedInstance: [124, 8, 30, "-s"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-A"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-a"], PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = [ nodeNames[1] ];
   options = { PreferedInstance: [124, 8, 30, "A"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "a"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-M"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-m"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-S"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-s"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-A"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [124, 8, 30, "-a"], PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false );
   db.removeRG( groupName );
}


