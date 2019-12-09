/*******************************************************************************
*@Description: findandremove basic testcases
*@Modify list:
*   2014-3-4 wenjing wang  Init
*******************************************************************************/

var clName = CHANGEDPREFIX + "_findAndmodify";
var dataGroupNum = 0;


if( "undefined" == typeof ( errCode ) )
{
   var errCode =
   {
      SDB_RTN_QUERYMODIFY_SORT_NO_IDX: -288,
      SDB_RTN_QUERYMODIFY_MULTI_NODES: -289
   }
}

if( "undefined" == typeof ( errMsg ) )
{
   var errMsg =
   {
      FISRTPARAM: "SdbQuery.update(): the 1st param should be non-empty object",
      SECONDPARAM: "SdbQuery.update(): the 2nd param should be boolean",
      WITHCOUNT: "count() cannot be executed with update() or remove()"
   }
}


//获取CL
function getCL ( db )
{
   try
   {
      var cs = db.getCS( COMMCSNAME );
      var cl = cs.getCL( clName );
   }
   catch( e )
   {
      throw buildException( "getCL", e );
   }
   return cl;
}

// 装载单条数据
function loadSingleDoc ( cl )
{
   try
   {
      cl.insert( { a: 1 } );
   }
   catch( e )
   {
      throw buildException( "loadSingleDoc", e );
   }
}

// 装载多条数据
function loadMultipleDoc ( cl, number )
{
   if( undefined == number )
   {
      number = 1000;
   }

   try
   {
      var j = 0;
      var subcldocnum = 5;
      if( 0 != getDataGroupNum() )
      {
         subcldocnum = subcldocnum * getDataGroupNum()
      }

      for( var i = 0; i < number; ++i )
      {
         if( i % subcldocnum == 0 &&
            i != 0 )
         {
            j = j + 1;
         }
         cl.insert( { _id: i, a: i, date: 20150000 + ( j % 4 + 1 ) * 100 + i % 28 + 1 } )
      }
   }
   catch( e )
   {
      throw buildException( "loadMultipleDoc", e );
   }
}

//删除所有数据
function removeAllDoc ( cl )
{
   try
   {
      cl.truncate()
   }
   catch( e )
   {
      throw buildException( "removeAllDoc", e );
   }
}

function buildoption ( ismaincl, groupname )
{
   var option = new Object();
   if( undefined != groupname )
   {
      option.Group = groupname;
   }

   option.ShardingType = "range";
   var shardingkey = new Object();
   if( undefined != ismaincl &&
      true == ismaincl )
   {
      option.IsMainCL = ismaincl
      shardingkey.date = 1;
   }
   else
   {
      shardingkey._id = 1;
   }

   option.ShardingKey = shardingkey;
   return option;
}

if( "undefined" == typeof ( createMode ) )
{
   var createMode =
   {
      ordinary: 0,
      horizontal: 1,
      vertical: 2
   }
}

function setUp ( replsize, createmode )
{
   try
   {
      if( undefined == replsize )
      {
         replsize = 1;
      }

      if( undefined == createmode )
      {
         createmode = createMode.ordinary;
      }

      //var db = new Sdb( COORDHOSTNAME, COORDSVCNAME ); 
      commDropCL( db, COMMCSNAME, clName );
      //db.getCS( COMMCSNAME ).createCL( clName, {ReplSize:0} ); 
      // 指定ReplSize为0，同时不设置setSessionAttr就能够测试到副本有无被modify
      if( createMode.ordinary == createmode )
      {
         commCreateCL( db, COMMCSNAME, clName, replsize, false, true, true );
      }
      else
      {
         var ismaincl = false;
         if( createMode.vertical == createmode )
         {
            ismaincl = true;
         }
         var curcl = commCreateCLByOption( db, COMMCSNAME, clName, buildoption( ismaincl ), true );
         if( false == ismaincl )
         {
            splittable( db, curcl );
         }
      }
   }
   catch( e )
   {
      throw buildException( "setUp", e );
   }

   return db;
}

function tearDown ( db )
{
   if( undefined == db )
   {
      return;
   }

   try
   {
      commDropCL( db, COMMCSNAME, clName );
      //db.getCS( COMMCSNAME ).dropCL( clName ); 
      db.close();
   }
   catch( e )
   {
      throw buildException( "tearDown", e );
   }
}

// 切分表
function splittable ( db, cl, clname )
{
   try
   {
      if( undefined == clname )
      {
         clname = clName;
      }
      var fullname = COMMCSNAME + "." + clname;
      var srcgroups = commGetCLGroups( db, fullname );
      if( srcgroups.length > 1 )
      {
         return srcgroups.length;
      }

      var datagroups = commGetGroups( db, true );
      dataGroupNum = datagroups.length;
      var startid = 0;
      var endid = 5;
      for( var i = 0; i < datagroups.length; ++i )
      {
         if( datagroups[i][0].GroupName != srcgroups[0] )
         {
            cl.splitAsync( srcgroups[0], datagroups[i][0].GroupName, { _id: startid }, { _id: endid } );
            startid = endid;
            endid += 5;
         }
      }
      sleep( 10 );
      var isExistTask = false;
      do
      {
         isExistTask = false;
         var cursor = db.listTasks();
         while( cursor.next() )
         {
            isExistTask = true;
         }
         if( !isExistTask )
         {
            sleep( 10 );
         }
      }
      while( isExistTask );
      return datagroups.length;
   }
   catch( e )
   {
      buildException( "splittable", e );
   }
}

//检查删除结果
function checkRemoveResult ( cl, arr )
{
   for( var i = 0; i < arr.length; ++i )
   {
      var doc = eval( "( " + arr[i] + " )" );
      recordnumber = cl.find( { _id: doc["_id"], b: 1 } ).count();
      if( 0 != parseInt( recordnumber ) )
      {
         println( "find( {_id:" + doc["_id"] + ", b:1} ).count() expect 0 real " + recordnumber );
         return false;
      }
   }

   return true;
}

//检查更新结果
function checkUpdateResult ( cl, arr )
{
   for( var i = 0; i < arr.length; ++i )
   {
      var doc = eval( "( " + arr[i] + " )" );
      recordnumber = cl.find( { _id: doc["_id"], b: 1 } ).count();
      if( 1 != parseInt( recordnumber ) )
      {
         println( "find( {_id:" + doc["_id"] + ", b:1} ).count() expect 1 real " + recordnumber );
         return false;
      }
   }

   return true;
}

// 创建子表，并且挂到相应主表下
function init ( db, maincl, isneedsplit, groupname )
{
   try
   {
      var ismaincl = false;
      var start = 20150101;
      dataGroupNum = 0; // 不切分的情况下，不考虑多个组，在这重置

      for( var i = 0; i < 4; ++i )
      {
         var subclname = clName + "sub";
         subclname = subclname + i;
         var subcl = commCreateCLByOption( db, COMMCSNAME, subclname, buildoption( ismaincl, groupname ), true );
         var fullname = COMMCSNAME + "." + subclname;
         maincl.attachCL( fullname, { LowBound: { date: start + ( i * 100 ) }, UpBound: { date: start + ( ( i + 1 ) * 100 ) } } );

         if( undefined != isneedsplit &&
            true == isneedsplit )
         {
            splittable( db, subcl, subclname );
         }
      }
   }
   catch( e )
   {
      throw buildException( "init", e );
   }
}

// 结束时，释放所有子表
function fini ( db )
{
   try
   {
      for( var i = 0; i < 4; ++i )
      {
         var subclname = clName + "sub"
         subclname = subclname + i;
         commDropCL( db, COMMCSNAME, subclname );
      }
   }
   catch( e )
   {
      throw buildException( "fini", e );
   }
}

function getDataGroupNum ( db )
{
   if( 0 != dataGroupNum )
   {
      return dataGroupNum;
   }
   try
   {
      if( undefined == db )
      {
         return 0;
      }
      var datagroups = commGetGroups( db, true );
      return datagroups.length;
   }
   catch( e )
   {
      throw buildException( "getDataGroupNum", e );
   }
}

