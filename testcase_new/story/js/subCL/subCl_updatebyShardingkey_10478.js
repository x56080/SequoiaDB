/**************************************
 * @Description: 更新主子表中分区键字段
 * @author: ouyangzhongnan 
 * @Date: 2016-11-22
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCl_updatebyShardingkey_10478.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

main();
function main()
{
	//设置变量初始化值
	var mainCl = null;
	var subCls = [];
	var data = [];
	var mainClName =  CHANGEDPREFIX+"_maincl_10478" ;
	var subClNames = [
		CHANGEDPREFIX+"_subcl_10478_0",
		CHANGEDPREFIX+"_subcl_10478_1",
	];

	//check test environment before split
	try 
	{
	   //standalone can not split
	   if( true == commIsStandalone( db ) ) 
	   {
	      println( "LOG:run mode is standalone" );
	      return;
	   }     
	   //less two groups,can not split
	   if( 1 === getGroupName( db ).length ) 
	   {
	      println("LOG:least two groups");
	      return ;
	   }
	} 
	catch( e ) 
	{
	   throw buildException( "ERR:failed init environment" ,e);
	}

	//clean the environment, drop subcl and maincl
	commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );	
	//create maincl and subcl, attach subcl
	db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCLOption = { IsMainCL:true, ShardingKey:{ a:1 }, ShardingType: "range", ReplSize:0, Compressed:true };
   var subCLOption = { ShardingKey:{ b:1 }, ShardingType: "hash", ReplSize:0, Compressed:true, Partition:16 };
   mainCl = commCreateCLByOption( db, COMMCSNAME, mainClName, mainCLOption, true, true );
   subCls.push( commCreateCL( db, COMMCSNAME, subClNames[0], 0) );
	subCls.push( commCreateCLByOption( db, COMMCSNAME, subClNames[1], subCLOption, true, true ) );
   ClSplitOneTimes( COMMCSNAME, subClNames[1], {Partition:8}, null );
   attachCL( mainCl, COMMCSNAME +"."+ subClNames[0], { LowBound:{a:0},UpBound:{a:10} } );
   attachCL( mainCl, COMMCSNAME +"."+ subClNames[1], { LowBound:{a:10},UpBound:{a:20} } );
   //init data
	for (var i = 0; i < 100; i++) 
	{
		var skValue = i%20;
		data.push( { a:skValue, b:i } );
	}
	try
	{
		mainCl.insert(data);
	}
	catch(e) 
	{
		throw buildException("ERR:failed to init data", e );	
	}
   

   // update maincl shardingkey test
   try
   {
	   mainCl.update( {$set:{a:"updatemainclSk_name"}}, {a:15} );
	   var cursor = mainCl.find( {a:15} );
   	for( var i=0; i<cursor.count(); i++)
   	{
   		if( cursor.next().toObj().a == "updatemainclSk_name")
   		{
   			throw new Error();
   		}
   	}
   }
   catch(e)
   {
   	throw buildException("failed to update test", e, "update by condition a=15 set name='updatemainclSk_name'", "can not be update but not error", "update error or have some record update success");
   }

   // update subcl shardingkey test
   try
   {
	   mainCl.update( {$set:{b:"updatesubclSk_name"}}, {b:15} );
	   var cursor = mainCl.find( {b:15} );
   	for( var i=0; i<cursor.count(); i++)
   	{
   		if( cursor.next().toObj().a == "updatesubclSk_name")
   		{
   			throw new Error();
   		}
   	}
   }
   catch(e)
   {
   	throw buildException("failed to update test", e, "update by condition b=15 set name='updatesubclSk_name'", "can not be update but not error", "update is error");
   }

	//clean the environment, drop subcl and maincl
	commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
}