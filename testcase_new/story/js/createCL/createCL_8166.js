/* *****************************************************************************
@discretion: 测试建表时的AutoIndexId选项 
@modify list:
   2014-2-24 wenjing wang  Init
***************************************************************************** */
var domainname = CHANGEDPREFIX + "tdomain";
function getDataGroupNames ( db )
{
   try
   {
      var datagroups = commGetGroups( db, true );
      if( datagroups.length == 0 )
      {
         throw "getDataGroupNames failed expect not equal 0";
      }

      var groupnames = new Array();
      for( var i = 0; i < datagroups.length; ++i )
      {
         groupnames.push( datagroups[i][0].GroupName );
      }
      return groupnames;
   } catch( e )
   {
      throw "getDataGroupNames failed unknown exception " + e;
   }
}

// 选项字段为全小写，不被允许
function createCLwithErrField ( cs, clname )
{
   try
   {
      var cl = cs.createCL( clname, { autoindexid: false } );
   } catch( e )
   {
      if( e != -6 )
      {
         throw "createCL('" + clname + "',{autoindexid:false})" + "expect -6 real is " + e;
      }

      return;
   }
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   cs.dropCL( clname );
   if( false == commIsStandalone( db ) )
   {
      throw "createCL('" + clname + "',{autoindexid:0}) expect -6 real is 0";
   }
}

// 选项字段值为数字，不被允许
function createCLwithErrFieldVal ( cs, clname )
{
   try
   {
      var cl = cs.createCL( clname, { AutoIndexId: 0 } );
   } catch( e )
   {
      if( e != -6 )
      {
         throw "createCL('" + clname + "',{AutoIndexId:0})" + "expect -6 real is " + e;
      }
      return;
   }
   cs.dropCL( clname );
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   if( false == commIsStandalone( db ) )
   {
      throw "createCL(" + clname + ",{AutoIndexId:0}) expect -6 real is 0";
   }
}

// 选项字段在主表上使用，不被允许
function createMainCL ( cs, clname )
{
   try
   {
      var cl = cs.createCL( clname, { IsMainCL: true, AutoIndexId: false, ShardingKey: { date: 1 } } );
   } catch( e )
   {
      if( e != -6 )
      {
         throw "createCL('" + clname + "',{IsMainCL:true,AutoIndexId:false,ShardingKey:{date:1}})" + "expect -6 real is " + e;
      }
      return;
   }
   cs.dropCL( clname );
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   if( false == commIsStandalone( db ) )
   {
      throw "createCL('" + clname + "',{IsMainCL:true,AutoIndexId:false,ShardingKey:{date:1}}) expect -6 real is 0";
   }
}

function checkIndexnumber ( cl )
{
   var indexes = cl.listIndexes().toArray();
   if( indexes.length != 0 )
   {
      throw "{AutoIndexId:false}'test listIndexes expect 0 real is " + indexes.length;
   }
}

// 普通表上使用该选项
function createCLwithOrdinaryTable ( cs, clname )
{
   try
   {
      var cl = cs.createCL( clname, { AutoIndexId: false, ReplSize: 0 } );
      cl.insert( { _id: 1, a: 1 } );
      var num = cl.count();
      if( num != 1 )
      {
         throw "{AutoIndexId:false}'test insert failure expect 1 real is" + num;
      }
   }
   catch( e )
   {
      throw "createCLwithOrdinaryTable step 1 unknow exception " + e;
   }

   try
   {
      checkIndexnumber( cl );
   }
   catch( e )
   {
      throw "createCLwithOrdinaryTable step 2 checkIndexnumber unknow exception " + e;
   }

   try
   {
      cl.update( { $set: { a: 2 } }, { _id: 1 } )
   } catch( e )
   {
      if( e != -279 )
      {
         throw "{AutoIndexId:false}'s test update failure expect -279 real is " + e;
      }
   }

   try
   {
      cl.remove( { _id: 1 } )
   } catch( e )
   {
      if( e != -279 )
      {
         throw "{AutoIndexId:false}'s test remove failure expect -279 real is" + e;
      }
   }
   cs.dropCL( clname );
}

function loaddata ( cl, recordnumber )
{
   for( i = 0; i < recordnumber; ++i )
   {
      cl.insert( { _id: i, a: i, date: 20150000 + 100 * ( i % 2 + 1 ) + i % 29 + 1 } );
   }
}

function checkupdate ( cl, recordnumber )
{
   for( i = 0; i < recordnumber; ++i )
   {
      try
      {
         cl.update( { $inc: { a: 1000 } }, { _id: i } );
      }
      catch( e )
      {
         if( e == -279 )
         {
            continue;
         }
         println( "update failed" );
      }
   }

   for( i = 0; i < recordnumber; ++i )
   {
      var cursor = cl.find( { a: i } );
      var cnt = 0;
      while( cursor.next() )
      {
         cnt++;
      }
      if( cnt != 1 )
      {
         throw "after update({$inc:{a:1000}},{_id:" + i + "}) expect find({a:" + i + "}) is 1 real is " + cnt;
      }
   }
}

function checkremove ( cl, recordnumber )
{
   for( i = 0; i < recordnumber; ++i )
   {
      try
      {
         cl.remove( { a: i } )
      }
      catch( e )
      {
         if( e == -279 )
         {
            continue;
         }
         println( "remove failed" );
      }
   }

   var currecordnum = cl.count();
   if( currecordnum != recordnumber )
   {
      throw "after remove({a:i}) expect count() is " + recordnumber + "real is " + currecordnum;
   }
}

// 水平分区表上使用该选项，让其自动切分，所以建在域中
function createCLwithHorizontalpartitiontable ( db, csname, clname )
{
   commDropDomain( db, domainname );
   try
   {
      var groupnames = getDataGroupNames( db );
      commCreateDomain( db, domainname, groupnames, { AutoSplit: true } );
   }
   catch( e )
   {
      throw "createCLwithHorizontalpartitiontable step 1 unknown exception " + e;
   }

   try
   {
      db.dropCS( csname );
   }
   catch( e )
   {
      if( e != -34 )
      {
         throw "dropCS('" + csname + "') expect -34 real is " + e
      }
   }

   try
   {
      var opt = new Object();
      opt.Domain = domainname;
      var cs = db.createCS( csname, opt );
      var cl = cs.createCL( clname, { ShardingKey: { _id: 1 }, ReplSize: 0, ShardingType: 'hash', AutoSplit: true, AutoIndexId: false, EnsureShardingIndex: false } );
   }
   catch( e )
   {
      throw "createCLwithHorizontalpartitiontable step 2 unknown exception " + e;
   }

   try
   {
      checkIndexnumber( cl );
      var recordnumber = 1000;
      loaddata( cl, recordnumber );
      checkupdate( cl, recordnumber );
      checkremove( cl, recordnumber );

      cs.dropCL( clname );
      db.dropCS( csname );
      commDropDomain( db, domainname );
   }
   catch( e )
   {
      throw "createCLwithHorizontalpartitiontable step 3 unknown exception " + e;
   }

}

// 垂直分区表使用该选项
function createCLwithVerticalpartitiontable ( db, csname, clname )
{
   try
   {
      var subopt = new Object();
      subopt.date = 1;

      var opt = new Object();
      opt.IsMainCL = true;
      opt.ShardingKey = subopt;

      var cl = commCreateCLByOption( db, csname, clname, opt, true );
      //var cl = cs.createCL(clname, {IsMainCL:true,ShardingKey:{date:1}});
      var subclname1 = CHANGEDPREFIX + "t01";
      var subclname2 = CHANGEDPREFIX + "t02";

      var subcl1 = db.getCS( csname ).createCL( subclname1, { ReplSize: 0, ShardingType: "hash", ShardingKey: { _id: 1 }, AutoIndexId: false, EnsureShardingIndex: false } );
      var subcl2 = db.getCS( csname ).createCL( subclname2, { ReplSize: 0, ShardingType: "hash", ShardingKey: { _id: 1 }, AutoIndexId: false, EnsureShardingIndex: false } );
      cl.attachCL( csname + '.' + subclname1, { LowBound: { date: 20150101 }, UpBound: { date: 20150201 } } );
      cl.attachCL( csname + '.' + subclname2, { LowBound: { date: 20150201 }, UpBound: { date: 20150301 } } );
   }
   catch( e )
   {
      throw "createCLwithVerticalpartitiontable step 1 unknown exception " + e;
   }

   try
   {
      checkIndexnumber( cl );
      checkIndexnumber( subcl1 );
      checkIndexnumber( subcl2 );
   }
   catch( e )
   {
      throw "createCLwithVerticalpartitiontable step 2 unknown exception " + e;
   }

   try
   {
      var recordnumber = 1000;
      loaddata( cl, recordnumber );
      checkupdate( cl, recordnumber );
      checkremove( cl, recordnumber );
   }
   catch( e )
   {
      throw "createCLwithVerticalpartitiontable step 3 unknown exception " + e;
   }

   db.getCS( csname ).dropCL( clname );
}

function clean ( db, csname )
{
   commDropCS( db, csname, true );
   db.close();
}

function main ()
{
   var TESTCSNAME = CHANGEDPREFIX + "test";
   var TESTCLNAME = CHANGEDPREFIX + "test";
   db.setSessionAttr( { "PreferedInstance": 'M' } );
   var cs = commCreateCS( db, TESTCSNAME, false );
   try
   {
      createCLwithErrField( cs, TESTCLNAME );
      createCLwithErrFieldVal( cs, TESTCLNAME );
      createMainCL( cs, TESTCLNAME );

      // 不使用AutoIndexId选项的用例没有加，因为原来的测试用例已经覆盖
      createCLwithOrdinaryTable( cs, TESTCLNAME );
      if( false == commIsStandalone( db ) )
      {
         createCLwithHorizontalpartitiontable( db, TESTCSNAME, TESTCLNAME );
         createCLwithVerticalpartitiontable( db, TESTCSNAME, TESTCLNAME );
      }

   } catch( e )
   {
      clean( db, TESTCSNAME );
      throw e;
   }

   clean( db, TESTCSNAME )
}

main();
