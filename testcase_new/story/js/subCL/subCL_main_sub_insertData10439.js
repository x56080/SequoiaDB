/************************************
*@Description: 数据落在不同组上的相同子表中，批量插入数据
*@author:      zengxianquan
*@createDate:  2016.11.23
*@testlinkCase: seqDB-10439
**************************************/
function main()
{
   mainCL_Name = CHANGEDPREFIX + "_maincl" ;
   subCL_Name = CHANGEDPREFIX + "_subcl1";
   //检查环境，是否可以拆分
   //standalone can not split
   if(true == commIsStandalone( db ))
   {
      println( "run mode is standalone");
      return;
   }
   //至少两个组才可以进行拆分
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      println("--least two groups");
      return ;
   }
   //在测试前清理环境
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, 
                  "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name, true, true, 
                  "clean sub collection" ); 
   //创建 maincl 
   db.setSessionAttr( {PreferedInstance:"M"} );
   var mainCLOption = { ShardingKey:{"a":1},ShardingType:"range",IsMainCL:true};
   var dbcl = commCreateCLByOption( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true);
   //获取所有的数据组
   var groupsArray = commGetGroups(db, false, "", false, true, true );
   //获取其中两个数据组的名字
   var groupName1 = groupsArray[1][0].GroupName;
   var groupName2 = groupsArray[2][0].GroupName;
   //创建 subcl
   var subClOption1 = {ShardingKey:{"b":1},ShardingType:"range", ReplSize:0,Group:groupName1};
   commCreateCLByOption( db, COMMCSNAME, subCL_Name, subClOption1, true, true );
     
   try
   {
      //attach subcl
      dbcl.attachCL( COMMCSNAME + "." + subCL_Name, { LowBound:{a:0},UpBound:{a:100} } ) ;
   }
   catch( e )
   {
      println( "failed to attch sub cl");
      throw buildException("main()",e,"attach subc1", "attach subc1 success","attach subc1 fail");
   }
   try
   {
      //sprilt subcl
      db.getCS(COMMCSNAME).getCL(subCL_Name).split(groupName1, groupName2, {b:50},{b:100});
   }
   catch(e)
   {
      println( "failed to sprilt subcl");
      throw buildException("main()",e,"split subc1", "split subc1 success","split subc1 fail");
   }
   //插入数据
   insertData( db, COMMCSNAME, mainCL_Name );
   //检验数据的正确性
   checkData( db, groupName1, groupName2 , COMMCSNAME, subCL_Name );
  
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

function insertData( db, csName, clName )
{
   //构造数据
   var doc=[];
   for (var i=0;i<100;i++)
   {
      for(var k=0;k<100;k++)
      {
         doc.push({a:i,b:k,test:"testData"+k});
      }
   }
    //insert data
   try
   {
      db.getCS(COMMCSNAME).getCL(mainCL_Name).insert(doc);
   }
   catch(e)
   {
      throw buildException("insertData()",e,"insert data", "insert data success","insert data fail");
   }   
}

function checkData( db, groupName1, groupName2 , csName, clName )
{
   //检测数据的正确性
   var expectDataArray1 = [];
   for ( var i = 0; i < 100; i++ )
   {
      for(var k = 0 ;k < 50; k++ )
      {
         expectDataArray1.push( {a:i,b:k,test:"testData"+k} );
      }
   }
   var rg = db.getRG(groupName1);
   var dataStr = rg.getMaster();
   var data1 = new Sdb(dataStr);
   var realData1 = data1.getCS(COMMCSNAME).getCL(subCL_Name).find().sort({a:1,b:1});
   zxqCheckRec( realData1, expectDataArray1 );
   var expectDataArray2 = [];
   for ( var i = 0; i < 100; i++ )
   {
      for( var k = 50; k < 100; k++ )
      {
         expectDataArray2.push({a:i,b:k,test:"testData"+k});
      }
   }
   var rg = db.getRG(groupName2);
   var dataStr = rg.getMaster();
   var data1 = new Sdb(dataStr);
   var realData2 = data1.getCS(COMMCSNAME).getCL(subCL_Name).find().sort({a:1,b:1});
   zxqCheckRec( realData2, expectDataArray2 );
   
}

main();












