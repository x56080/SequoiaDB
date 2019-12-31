/****************************************************
@description:   split, normal case
                testlink cases: seqDB-7223
@input:         create a cl with [Partition:1024]
                split cl by [splitquery={Partition:0}]
@expectation:   lackSplitendquery: expect: return 0
@modify list:
                2015-4-7 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7223";
var sourceGroup;
var targetGroup;

function lackSplitendquery ()
{
   var word = "split";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'source=' + sourceGroup, 'target=' + targetGroup, 'splitquery={Partition:0}'], [0], "splitAndCheck: fail to run rest cmd=" + word );

   var obj = db.snapshot( 8, { Name: csName + '.' + clName } ).current().toObj().CataInfo;
   for( var i = 0; i < obj.length; i++ )
   {
      switch( obj[i].GroupName )
      {
         case targetGroup:
            for( var p in obj[i]["UpBound"] )
            {
               if( obj[i]["UpBound"][p] != 1024 ) { throw "Fail to split by rest cmd=" + word + ",\ntargetGroup UpBound expect: 128, actual: " + obj[i]["UpBound"][p]; }
               break;
            }
            break;
         case sourceGroup:
            for( var p in obj[i]["LowBound"] )
            {
               if( obj[i]["LowBound"][p] != 1024 ) { throw "Fail to split by rest cmd=" + word + ",\nsourceGroup LowBound expect: 1, actual: " + obj[i]["LowBound"][p]; }
               break;
            }
            break;
         default:
            throw new Error( "cl[" + csName + '.' + clName + "] is in group[" + obj[i].GroupName + "],\nexpect: it is in sourceGroup or targeGroup, actual: targeGroup[" + targetGroup + "] sourceGroup[" + sourceGroup + "]" );
      }
   }
}

function main ()
{
   var groupInfos = commGetGroups( db );
   if( commIsStandalone( db ) || groupInfos.length < 2 )
   {
      println( "Mode is standalone or one group mode" );
      return;
   }
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   sourceGroup = groupInfos[0][0].GroupName;
   targetGroup = groupInfos[1][0].GroupName;
   var opt = { ShardingKey: { age: 1 }, ShardingType: "hash", Partition: 1024, Group: sourceGroup };
   var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
   lackSplitendquery();
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
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
