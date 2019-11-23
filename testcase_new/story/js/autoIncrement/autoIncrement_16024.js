/***************************************************************************
@Description :seqDB-16024 :同时修改自增字段及其他属性 
@Modify list :
              2018-10-26  zhaoyu  Create
****************************************************************************/
var sortField=0;
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
	  return;
   };
   
   var clName = COMMCLNAME + "_16024"; 
   var fieldName = "id";  
   commDropCL(db, COMMCSNAME, clName, true, true);
   
   var increment = 12;
   var acquireSize = 11;
   var dbcl = commCreateCLByOption(db, COMMCSNAME, clName, {AutoIncrement:{Field:fieldName, Increment:increment, AcquireSize:acquireSize}});
   
   var coordNodes = getCoordNodeNames();
   var coordNum = coordNodes.length;
   var expR = [];
   for(var k=0; k<coordNum; k++ )
   {
      var coord = new Sdb(coordNodes[k]);
      var cl = coord.getCS(COMMCSNAME).getCL(clName);
      var doc = [];
      for(var i=0;i<100;i++)
      {
         doc.push({a:sortField});
         expR.push({a:sortField, id:1 + Math.ceil(100/acquireSize)*acquireSize*increment*k + increment*i});
         sortField++;
      }
      cl.insert(doc);
      coord.close();
   }
   var actR = dbcl.find().sort({a:1});
   checkRec(actR, expR);
   println("---check insert success");
   
   var cacheSize = 32;
   var acquireSize = 11;
   var generated = "strict";
   var currentValue = 1000*increment + 1;
   dbcl.setAttributes({AutoIncrement:{Field:fieldName, CacheSize:cacheSize, AcquireSize:acquireSize, Generated:generated}, ShardingKey: { a: 1 }, CompressionType:'lzw'});
   var clID = getCLID(COMMCSNAME, clName);
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{Field:fieldName, SequenceName:clSequenceName, Generated:generated}];
   checkAutoIncrementonCL(COMMCSNAME, clName, expIncrementArr);
   println("---check cl autoIncrement success");
   
   var clExpSequenceObj = {Increment:increment, CacheSize:cacheSize, AcquireSize:acquireSize, CurrentValue:currentValue};
   checkSequence(clSequenceName, clExpSequenceObj);
   println("---check cl sequence success");
   
   checkSnapshot8onCL(COMMCSNAME, clName);
   println("---check cl shardingType and compressType success");
   
   for(var k=0; k<coordNum; k++ )
   {
      var coord = new Sdb(coordNodes[k]);
      var cl = coord.getCS(COMMCSNAME).getCL(clName);
      //alter操作会变更集合版本号，插入时会取2次seqence值，SEQUOIADBMAINSTREAM-3895,通过find操作更新版本号
      var cursor = cl.find();
      while(cursor.next()){}
      var doc = [];
      for(var i=0;i<100;i++)
      {
         doc.push({a:sortField});
         expR.push({a:sortField, id: 1 + Math.ceil(100/acquireSize)*acquireSize*increment*coordNum + Math.ceil(100/acquireSize)*acquireSize*increment*k + increment*i});
         sortField++;
      }
      cl.insert(doc);
      coord.close();
   }
   var actR = dbcl.find().sort({_id:1});
   checkRec(actR, expR);
   println("---check insert after alter autoIncrement success");
   
   try
   {
      dbcl.insert({id:"a"});
      throw ( "NEED_INSERT_ERR" );  
   }catch(e)
   {
      if(-6 !== e)
      {
         throw new Error(e);
      }
   }
   println("---check insert after alter generated success");
   commDropCL(db, COMMCSNAME, clName, true, true); 
}

try
{
   main();
}
catch(e)
{
   if ( e.constructor === Error )
   {
      println(e.stack) ;  
   }
   throw e ;
}

function checkSnapshot8onCL(csName, clName)
{
   try
   {
      var obj = db.snapshot(8,{Name:csName + "." + clName}).next().toObj();
      var shardingType = obj.ShardingType;
      var compressionType = obj.CompressionTypeDesc;
      if(shardingType !== "hash" || compressionType !== "lzw")
      {
         println("shardingType:" + shardingType + ",compressionType:" + compressionType + "\n");
         throw "ALTER_CL_ERR";
      }
   }
   catch(e)
   {
      throw new Error(e);
   }  
}
