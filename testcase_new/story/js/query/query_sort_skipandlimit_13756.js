/*******************************************************************************
*@Description: find().sort({a:1|-1}).skip(number).limit(number)
*@Modify list:
*   2014-5-5 wenjing wang  Init
*   2019-08-23 wangkexin Modified
*******************************************************************************/

var clName = "cl13756";

function loadData ( cl, number )
{
   var funname = "loadData";
   try
   {
      var records = [];
      for( i = 0; i < number; ++i )
      {
         records.push( { _id: i, a: i } );
      }
      cl.insert( records );
   }
   catch( e )
   {
      buildException( funname, e );
   }
}

function checkResult ( records, isAsc, totalnum, skipnum, limitnum )
{
   var funname = "checkResult";
   try
   {
      var realnum = 0;
      for( i = 0; i < records.length; ++i )
      {
         var obj = eval( "(" + records[i] + ")" );
         if( isAsc )
         {
            //println("obj.a "+ obj.a + " real " + (skipnum + realnum));
            if( obj.a != skipnum + realnum )
            {
               throw 1;
            }
         }
         else
         {
            //println("obj.a"+ obj.a + " real " + ((totalnum-1) - (skipnum + realnum)));
            if( obj.a != ( totalnum - 1 ) - ( skipnum + realnum ) )
            {
               throw 1;
            }
         }

         realnum = realnum + 1;
      }

      if( totalnum - skipnum > limitnum && realnum != limitnum )
      {
         throw 2;
      }
      else if( totalnum - skipnum < limitnum && totalnum - skipnum != realnum )
      {
         throw 2;
      }
   }
   catch( e )
   {
      if( 1 == e )
      {
         var val = isAsc ? 1 : -1;
         var op = "find().sort({a+:" + val + "}).skip(" + skipnum + ").limit(" + limitnum + ")";
         throw buildException( funname, e, op, skipnum + realnum, obj.d );
      }
      else( 2 == e )
      {
         var val = isAsc ? 1 : -1;
         var op = "find().sort({a:" + val + "}).skip(" + skipnum + ").limit(" + limitnum + ").count()";
         var expectnum = totalnum - skipnum > limitnum ? limitnum : totalnum - skipnum;
         throw buildException( funname, e, op, expectnum, realnum );
      }

      throw buildException( funname, e );

   }
}

/*******************************************************************************
*@Description：测试find().sort({a:1})结合不同的skip值和limit值
*@Input：totalnum 总的记录数量, skipnum skip的值 limitnum limit值
*@Expectation：skip skipnum条记录后，返回limitnum条记录按升序输出
********************************************************************************/
function test_FindForAscending ( cl, totalnum, skipnum, limitnum )
{
   try
   {
      loadData( cl, totalnum );
      var rs = cl.find().sort( { a: 1 } ).skip( skipnum ).limit( limitnum ).toArray();
      checkResult( rs, true, totalnum, skipnum, limitnum );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      cl.remove();
   }
}

/*******************************************************************************
*@Description：测试find().sort({a:-1})结合不同的skip值和limit值
*@Input：totalnum 总的记录数量, skipnum skip的值 limitnum limit值
*@Expectation：skip skipnum条记录后，返回limitnum条记录按降序输出
********************************************************************************/
function test_FindForDescending ( cl, totalnum, skipnum, limitnum )
{
   try
   {
      loadData( cl, totalnum );
      var rs = cl.find().sort( { a: -1 } ).skip( skipnum ).limit( limitnum ).toArray();
      checkResult( rs, false, totalnum, skipnum, limitnum );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      cl.remove();
   }
}

/*******************************************************************************
*@Description：构建建表的选项对象
*@Input：keyField指定分区键字段， isMainCL 是否是主表，true代表创建主表
         replSize 写副本的份数
         groupName 表初始位于groupName组上
         
*@Expectation：skip skipnum条记录后，返回limitnum条记录按降序输出
********************************************************************************/
function buildOption ( keyField, isMainCL, replSize, shardType, groupName )
{
   var option = new Object();
   if( undefined != groupName )
   {
      option.Group = groupName;
   }

   if( undefined != shardType )
   {
      option.ShardingType = shardType;
   }
   else
   {
      option.ShardingType = "range";
   }

   var shardingkey = new Object();
   if( undefined != isMainCL &&
      true == isMainCL )
   {
      option.ShardingType = "range";
      option.IsMainCL = isMainCL
      shardingkey[keyField] = 1;
   }
   else
   {
      shardingkey[keyField] = 1;
   }

   if( undefined != replSize )
   {
      option.ReplSize = replSize;
   }
   option.ShardingKey = shardingkey;
   return option;
}

function splitTable ( db, cl, collName, keyField, startval )
{
   try
   {
      if( undefined == startval )
      {
         startval = 0;
      }

      if( undefined == collName )
      {
         collName = clName;
      }

      var fullname = COMMCSNAME + "." + collName;
      var srcgroups = commGetCLGroups( db, fullname );
      if( srcgroups.length > 1 )
      {
         println( "exit split" )
         return srcgroups.length;
      }

      var datagroups = commGetGroups( db, true );
      dataGroupNum = datagroups.length;

      var startid = startval;
      var endid = startval + 5;
      var startobj = new Object();
      var endobj = new Object();
      for( var i = 0; i < datagroups.length; ++i )
      {
         if( datagroups[i][0].GroupName != srcgroups[0] )
         {
            startobj[keyField] = startid;
            endobj[keyField] = endid;
            cl.split( srcgroups[0], datagroups[i][0].GroupName, startobj, endobj );
            startid = endid;
            endid += 5;
         }
      }

      return datagroups.length;
   }
   catch( e )
   {
      throw buildException( "splitTable", e );
   }
}

/*******************************************************************************
*@Description：普通表上测试 sort({a:1})结合(skip(1)、limit(1)、skip all records, limit all records)
*@Input：db 连接对象  
*@Expectation：能够正常按顺序输出
********************************************************************************/
function test_OnOrdinaryTbl ( db )
{
   try
   {
      var cl = commCreateCL( db, COMMCSNAME, clName, 0, false, true, true );

      // 普通表上测试sort({a:1|-1}).limit(1)
      test_FindForAscending( cl, 10, 0, 1 );
      test_FindForDescending( cl, 10, 0, 1 );

      // 普通表上测试sort({a:1|-1}).skip(1).limit(1)
      test_FindForAscending( cl, 10, 1, 1 );
      test_FindForDescending( cl, 10, 1, 1 );

      // 普通表上测试sort({a:1|-1}).skip(10).limit(1)
      test_FindForAscending( cl, 10, 10, 1 );
      // 普通表上测试sort({a:1|-1}).skip(1).limit(10)
      test_FindForDescending( cl, 10, 1, 10 );
   }
   catch( e )
   {
      println( "test_OnOrdinaryTbl" );
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName );
   }
}

/*******************************************************************************
*@Description：水平分区表上测试 sort({a:1|-1})结合(skip(1)、limit(1)、skip one datagroup's records, limit one datagroup's records)
*@Input：db 连接对象  
*@Expectation：能够正常按顺序输出
********************************************************************************/
function test_OnHorizontalTbl ( db )
{
   try
   {
      var cl = commCreateCLByOption( db, COMMCSNAME, clName, buildOption( "a", false, 0 ), true );
      var groupsnum = splitTable( db, cl, clName, "a" );
      var totalnumber = 5 * groupsnum;
      // 水平分区表上测试sort({a:1|-1}).limit(1)
      test_FindForAscending( cl, totalnumber, 0, 1 );
      test_FindForDescending( cl, totalnumber, 0, 1 );

      // skip一个组的记录
      // 水平分区表上测试sort({a:1|-1}).skip(5).limit(1)
      test_FindForAscending( cl, totalnumber, 5, 1 );
      test_FindForDescending( cl, totalnumber, 5, 1 );

      // limit一个组的记录
      // 水平分区表上测试sort({a:1|-1}).skip(0).limit(5)
      test_FindForAscending( cl, totalnumber, 0, 5 );
      test_FindForDescending( cl, totalnumber, 0, 5 );

      // skip一个组的记录后，limit一个组的记录
      // 水平分区表上测试sort({a:1|-1}).skip(5).limit(5)
      test_FindForAscending( cl, totalnumber, 5, 5 );
      test_FindForDescending( cl, totalnumber, 5, 5 );

   }
   catch( e )
   {
      println( "test_OnHorizontalTbl" );
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName );
   }
}

/*******************************************************************************
*@Description：垂直分区表上测试 sort({a:1|-1})结合(skip(1)、limit(1)、
               skip one datagroup's records, limit one datagroup's records)
               skip one subCL's records, limit one subCL's records
*@Input：db 连接对象  
*@Expectation：能够正常按顺序输出
********************************************************************************/
function test_onVerticalTbl ( db )
{
   try
   {
      var subclName1 = "cl13756_subcl1";
      var subclName2 = "cl13756_subcl2";
      var mainCL = commCreateCLByOption( db, COMMCSNAME, clName, buildOption( "a", true, 0 ), true );
      var subcl1 = commCreateCLByOption( db, COMMCSNAME, subclName1, buildOption( "a", false, 0 ), true );
      var groupsnum = splitTable( db, subcl1, subclName1, "a" );
      var subcl2 = commCreateCLByOption( db, COMMCSNAME, subclName2, buildOption( "a", false, 0 ), true );
      var totalnumber = 5 * groupsnum * 2;
      splitTable( db, subcl2, subclName2, "a", 5 * groupsnum );
      mainCL.attachCL( COMMCSNAME + "." + subclName1, { LowBound: { a: 0 }, UpBound: { a: totalnumber / 2 } } );
      mainCL.attachCL( COMMCSNAME + "." + subclName2, { LowBound: { a: totalnumber / 2 }, UpBound: { a: totalnumber } } );

      // 垂直分区表上测试sort({a:1|-1}).limit(1)
      test_FindForAscending( mainCL, totalnumber, 0, 1 );
      test_FindForDescending( mainCL, totalnumber, 0, 1 );

      // skip一个组的记录
      // 垂直分区表上测试sort({a:1|-1}).limit(1)
      test_FindForAscending( mainCL, totalnumber, 5, 1 );
      test_FindForDescending( mainCL, totalnumber, 5, 1 );

      // limit一个组的记录
      // 垂直分区表上测试sort({a:1|-1}).limit(5)
      test_FindForAscending( mainCL, totalnumber, 0, 5 );
      test_FindForDescending( mainCL, totalnumber, 0, 5 );

      // skip一个组的记录后，limit一个组的记录
      // 垂直分区表上测试sort({a:1|-1}).skip(5).limit(5)
      test_FindForAscending( mainCL, totalnumber, 5, 5 );
      test_FindForDescending( mainCL, totalnumber, 5, 5 );

      // skip一个子表的记录
      // 垂直分区表上测试sort({a:1|-1}).skip(totalnumber/2).limit(1)
      test_FindForAscending( mainCL, totalnumber, totalnumber / 2, 1 );
      test_FindForDescending( mainCL, totalnumber, totalnumber / 2, 1 );

      // skip一个子表的记录后,limit一个子表的记录
      // 垂直分区表上测试sort({a:1|-1}).skip(totalnumber/2).limit(totalnumber/2)
      test_FindForAscending( mainCL, totalnumber, totalnumber / 2, totalnumber / 2 );
      test_FindForDescending( mainCL, totalnumber, totalnumber / 2, totalnumber / 2 );

   }
   catch( e )
   {
      println( "test_onVerticalTbl" );
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, subclName1 );
      commDropCL( db, COMMCSNAME, subclName2 );
      commDropCL( db, COMMCSNAME, clName );
   }
}

function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      test_OnOrdinaryTbl( db );

      if( commIsStandalone( db ) )
      {
         return;
      }
      test_OnHorizontalTbl( db );
      test_onVerticalTbl( db )
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      db.close();
   }
}

main()
