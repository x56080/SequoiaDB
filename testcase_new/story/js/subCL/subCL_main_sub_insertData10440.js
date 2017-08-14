/************************************
*@Description: 数据落在不同组上的不同子表中，批量插入数据
*@author:      zengxianquan
*@createDate:  2016.11.23
*@testlinkCase: seqDB-10440
**************************************/
function main()
{
   //check test environment before split
   try
	{
	   //standalone can not split
	   if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }     
      //less two groups,can not split
      var allGroupName = getGroupName( db );       
      if( 1 === allGroupName.length )
      {
         println("--least two groups");
         return ;
      }
   }
   catch( e )
   {
      throw e;
   }
   
   mainCL_Name = CHANGEDPREFIX + "_maincl" ;
   subCL_Name1 = CHANGEDPREFIX + "_subcl1";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2";
   
   //clean environment before test
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, 
                  "clean sub collection" ); 
   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, 
                  "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, 
                  "clean sub collection" ); 
   //get all groups to groupsArray
   db.setSessionAttr( {PreferedInstance:"M"} );
   var groupsArray = commGetGroups(db, false, "", false, true, true );
   //获取其中两个数据组的名字
   var groupName1 = groupsArray[1][0].GroupName;
   var groupName2 = groupsArray[2][0].GroupName;
   //create maincl 
   var mainCLOption = { ShardingKey:{"a":1}, ShardingType:"range", IsMainCL:true };
   var dbcl = commCreateCLByOption( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );
   //create subcl
   var subClOption1 = {Group:groupName1};
   commCreateCLByOption( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );
   var subClOption2 = {Group:groupName2};
   commCreateCLByOption( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );
   try
   {
      //attach subcl
      dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound:{a:0},UpBound:{a:100} } ) ;
      dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound:{a:100},UpBound:{a:200} } ) ;
   }
   catch( e )
   {
      throw buildException("main()",e,"attach subcl", "attach subcl success","attach subcl fail");
   }

   //插入数据
   insertData(dbcl);
   //检验数据的正确性
   checkData( db, groupName1, groupName2, COMMCSNAME, subCL_Name1, subCL_Name2 );
   
   //清除环境
   try
   {
      db.getCS(COMMCSNAME).dropCL(mainCL_Name);
   }
   catch(e)
   {
      throw buildException("main()",e,"clean sub", "clean sub success","cleam sub fail");	
   }
}

function insertData(dbcl)
{
   //构造数据
   var doc = [];
   for ( var i = 0; i < 200; i++ )
   {
      for( var k = 0; k < 100; k++ )
      {
         doc.push({a:i,b:k,test:"testData"+k});
      }
   }
   //insert data to dbcl
   try
   {
      dbcl.insert(doc);
   }
   catch(e)
   {
      throw buildException("insertData()",e,"insert data", "insert data success","insert data fail");
   }   
}

function checkData( db, groupName1, groupName2, csName, clName1, clName2 )
{
   //测试数据组中的数据是否与预测插入的数据是否一致
   //实现步骤：把每个组中的数据取出来，分别检验每个组的数据与预期插入的数据是否一致。
   var expectDataArray1 = [];
   for ( var i = 0; i<100; i++ )
   {
      for( var k = 0; k < 100; k++ )
      {
         expectDataArray1.push({a:i,b:k,test:"testData"+k});
      }
   }
   var rg = db.getRG(groupName1);
   var dataStr = rg.getMaster();
   var data1 = new Sdb(dataStr);
   var realData1 = data1.getCS(COMMCSNAME).getCL(subCL_Name1).find().sort({a:1,b:1});
   zxqCheckRec( realData1, expectDataArray1 );
   var expectDataArray2 = [];
   for ( var i = 100; i < 200; i++ )
   {
      for( var k = 0; k < 100; k++ )
      {
         expectDataArray2.push({a:i,b:k,test:"testData"+k});
      }
   }
   var rg = db.getRG(groupName2);
   var dataStr = rg.getMaster();
   var data2 = new Sdb(dataStr);
   var realData2 = data2.getCS(COMMCSNAME).getCL(subCL_Name2).find().sort({a:1,b:1});
   zxqCheckRec( realData2, expectDataArray2 );   
}
main();