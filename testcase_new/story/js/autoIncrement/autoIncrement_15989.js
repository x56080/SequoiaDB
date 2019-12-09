/***************************************************************************
@Description : seqDB-15989:主子表上创建/删除自增字段，删除主表自增字段
@Modify list :
              2018-10-18  zhaoyu  Create
****************************************************************************/
var sortField = 0;
function main ()
{
   var dataGroupNames = getDataGroupNames();
   println( "dataGroupNames:" + dataGroupNames );
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      println( "Deploy is standalone or only one group" );
      return;
   }

   var maincsName = COMMCSNAME + "_maincs_15989";
   var mainclName = COMMCLNAME + "_maincl_15989";

   var subcsName = COMMCSNAME + "_subcs_15989";
   var subclName1 = COMMCLNAME + "_subcl_15989_1";
   var subclName2 = COMMCLNAME + "_subcl_15989_2";
   var subclName3 = COMMCLNAME + "_subcl_15989_3";

   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName1 = maincsName + "." + subclName1;
   var subclFullName2 = subcsName + "." + subclName2;
   var subclFullName3 = subcsName + "." + subclName3;

   var maincl;
   var subcl1;
   var subcl2;
   var subcl3;
   var fieldName = "id";

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );

   var mainclOption = { IsMainCL: true, ShardingKey: { "a1": 1 }, ShardingType: "range" };
   maincl = commCreateCLByOption( db, maincsName, mainclName, mainclOption );

   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: dataGroupNames[0] };
   subcl1 = commCreateCLByOption( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: dataGroupNames[0] };
   subcl2 = commCreateCLByOption( db, subcsName, subclName2, subclOption2 );
   var subclOption3 = {};
   subcl3 = commCreateCLByOption( db, subcsName, subclName3, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 10 }, { a0: 20 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   maincl.attachCL( subclFullName1, { LowBound: { a1: 1 }, UpBound: { a1: 21 } } );
   maincl.attachCL( subclFullName2, { LowBound: { a1: 21 }, UpBound: { a1: 41 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a1: 41 }, UpBound: { a1: 61 } } );

   var mainclCacheSize = 10;
   var mainclAcquireSize = 1;
   var subclIncrement = 10;
   maincl.createAutoIncrement( { Field: fieldName, CacheSize: mainclCacheSize, AcquireSize: mainclAcquireSize } );
   subcl1.createAutoIncrement( { Field: fieldName, Increment: subclIncrement } );

   var mainclID = getCLID( maincsName, mainclName );
   var mainclSequenceName = "SYS_" + mainclID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: mainclSequenceName }];
   checkAutoIncrementonCL( maincsName, mainclName, expIncrementArr );
   println( "---check maincl autoIncrement success" );

   var subclID = getCLID( maincsName, subclName1 );
   var subclSequenceName = "SYS_" + subclID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: subclSequenceName }];
   checkAutoIncrementonCL( maincsName, subclName1, expIncrementArr );
   println( "---check subcl autoIncrement success" );

   var mainExpSequenceObj = { CacheSize: mainclCacheSize, AcquireSize: mainclAcquireSize };
   checkSequence( mainclSequenceName, mainExpSequenceObj );
   println( "---check maincl sequence success" );
   var subExpSequenceObj = { Increment: subclIncrement };
   checkSequence( subclSequenceName, subExpSequenceObj );
   println( "---check subcl sequence success" );

   var doc = [];
   var expR = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i, id: i } );
      sortField++;
   }
   maincl.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert into maincl success" );

   var doc = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, a0: i } );
      expR.push( { a: sortField, a1: i, a0: i, id: ( i - 1 ) * 10 + 1, } );
      sortField++;
   }
   subcl1.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert into subcl success" );

   maincl.dropAutoIncrement( fieldName );

   var doc = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i } );
      sortField++;
   }
   maincl.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert into maincl after drop increment field success" );

   var doc = [];
   var j = 61;
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, a0: i } );
      expR.push( { a: sortField, a1: i, a0: i, id: ( j - 1 ) * 10 + 1 } );
      sortField++;
      j++;
   }
   subcl1.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert into subcl after drop increment field success" );

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
