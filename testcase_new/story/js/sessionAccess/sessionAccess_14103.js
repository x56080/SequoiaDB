/* *****************************************************************************
@description: seqDB-14103: 设置会话属性指定实例为备节点对应instanceid，写入数据后再查询 
@author: 2020-4-15 zhaoxiaoni  Init
***************************************************************************** */
main();

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var clName = CHANGEDPREFIX + "_14103";
   var groupName = commGetGroups( db )[0][0]["GroupName"] ;
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName }, true, true );

   db.setSessionAttr( { PreferedInstance: "S" } )
   insertData( cl );

   var master = db.getRG( groupName ).getMaster();
   var expAccessNode = master.getHostName() + ":" + master.getServiceName();
   var actAccessNode = cl.find().explain().current().toObj().NodeName;
   if( actAccessNode !== expAccessNode )
   {
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + actAccessNode );
   }

   commDropCL( db, COMMCSNAME, clName, false, false );
}
