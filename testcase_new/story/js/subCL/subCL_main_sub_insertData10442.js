/************************************
*@Description: 数据落在相同组上的相同子表中，批量插入数据
*@author:      zengxianquan
*@createDate:  2016.11.24
*@testlinkCase: seqDB-10442
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

   //在测试前清除环境中冲突的表
   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, 
                  "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, 
                  "clean sub collection" );
   //获取所有的数据组
   db.setSessionAttr( {PreferedInstance:"M"} );
   var groupsArray=commGetGroups(db, false, "", false, true, true );
   //创建主表
   var mainCLOption = { ShardingKey:{"a":1},ShardingType:"range",IsMainCL:true};
   var mainCL=commCreateCLByOption( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true);
   //创建普通子表
   var groupName=groupsArray[1][0].GroupName;
   var subClOption1 = {Group:groupName};
   commCreateCLByOption( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );
   //创建分区表
   var subClOption2 = {Group:groupName,ShardingKey:{"c":1},ShardingType:"range", ReplSize:0};
   commCreateCLByOption( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );
   try{
      //attach 普通的表
      mainCL.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound:{a:0  },UpBound:{a:100} } ) ;
      //attach分区表
      mainCL.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound:{a:100},UpBound:{a:200} } ) ;
   }catch( e ){
      throw buildException("main()",e,"attach subcl", "attach subcl success","attach subcl fail");
   }
   //插入数据
   insertData( mainCL );
   //检验数据的一致性
   checkData( db, groupName, COMMCSNAME, subCL_Name1, subCL_Name2 )
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
function insertData( mainCL )
{
    //向表1插入数据
   var doc1 = [];
   var doc2 = [];
   for( var j = 0; j < 100; j++ )
   {
      for( var k = 0; k < 100; k++ )
      {
         doc1.push({a:j,b:k,test:"testData"+k});
      }
   }
   for( var j = 100; j < 200; j++ )
   {
      for( var k = 0; k < 100; k++ )
      {
         doc2.push({a:j,b:k,test:"testData"+k});
      }
   }
   try{
   //批量向表1插入数据
   mainCL.insert(doc1);
   mainCL.insert(doc2);
   }catch(e){
      throw buildException("insertData()", e, "insert data", "insert data success","insert data fail");
   }  
}

function checkData( db, groupName, csName, clName1, clName2 )
{
//检测插入的数据是否一致
   var expectDataArray1=[];
   for (var i=0;i<100;i++)
   {
      for(var k=0;k<100;k++)
      {
         expectDataArray1.push({a:i,b:k,test:"testData"+k});
      }
   }
   var rg = db.getRG(groupName);
   var dataStr = rg.getMaster();
   var data1=new Sdb(dataStr);
   var realData1=data1.getCS(COMMCSNAME).getCL(subCL_Name1).find().sort({a:1,b:1});	
   zxqCheckRec(realData1,expectDataArray1);
   var expectDataArray2=[];
   for (var i=100;i<200;i++)
   {
      for(var k=0;k<100;k++)
      {
         expectDataArray2.push({a:i,b:k,test:"testData"+k});
      }
   }
   var rg = db.getRG(groupName);
   var dataStr = rg.getMaster();
   var data2=new Sdb(dataStr);
   var realData2=data2.getCS(COMMCSNAME).getCL(subCL_Name2).find().sort({a:1,b:1});	
   zxqCheckRec(realData2,expectDataArray2);   
}
main();