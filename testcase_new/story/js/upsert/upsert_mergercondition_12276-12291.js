/*******************************************************************************
*@Description: JavaScript common function library
*@Modify list:
*   2014-4-20 wenjing wang Init
*******************************************************************************/

var clName = COMMCLNAME + "_upsert";


/*******************************************************************************
*@Description：条件为 $et、$all、$and等时，执行upsert操作
*@Input：rule为更新规则文档
cond为查询条件
res为结果文档
*@Expectation：upsert后生成的新文档为res
********************************************************************************/
function upsertandmerger ( cl, rule, cond, res, errres )
{
   var funname = "upsertandmerger";
   if( undefined == rule )
   {
      return;
   }

   if( undefined == cond )
   {
      var cond = {};
   }

   try
   {
      cl.upsert( rule, cond )
      docnum = cl.find( res ).count();
      if( 1 != docnum )
      {
         throw -1;
      }

      if( undefined != errres )
      {
         docnum = cl.find( errres ).count();
         if( 0 != docnum )
         {
            throw -2;
         }
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         println( "upsert( " + BuildObjStr( rule ) + ", " + BuildObjStr( cond ) + " )" );
         throw buildException( funname, e, "find( " + BuildObjStr( res ) + " )", 1, docnum );
      }
      else if( -2 == e && undefined != errres )
      {
         println( "upsert( " + BuildObjStr( rule ) + ", " + BuildObjStr( cond ) + " )" );
         throw buildException( funname, e, "find( " + BuildObjStr( errres ) + " )", 0, docnum );
      }
      else
      {
         throw buildException( funname, e )
      }
   }
   finally
   {
      cl.remove();
   }
}

function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      commDropCL( db, COMMCSNAME, clName );
      var cl = commCreateCL( db, COMMCSNAME, clName );
      db.setSessionAttr( { PreferedInstance: "M" } );
      println( "**************************************" );
      upsertandmerger( cl, { $set: { a: 1 } }, { a: 1 }, { a: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { _id: 1, a: 1 }, { _id: 1, a: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { a: 2 }, { a: 1 } );
      upsertandmerger( cl, { $set: { b: 1, a: 1 } }, { b: 2 }, { a: 1, b: 1 } );
      upsertandmerger( cl, { $set: { b: 1, a: 1 } }, {}, { a: 1, b: 1 } );
      upsertandmerger( cl, { $set: { a: { b: 1 } } }, { a: { b: 1 } }, { a: { b: 1 } } );
      upsertandmerger( cl, { $inc: { c: 1 } }, { c: 1 }, { c: 2 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { x: { $et: 1 } }, { x: 1, a: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { x: { $all: [1] } }, { a: 1, x: [1] } );
      upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ x: { $et: 1 } }] }, { x: 1, a: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ x: { $all: [1] } }] }, { x: [1], a: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ x: { $all: [1] } }, { b: { $et: 1 } }] }, { x: [1], a: 1, b: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ d: { $gte: 1 } }, { $and: [{ x: { $all: [1] } }] }] }, { x: [1], a: 1 }, { x: [1], a: 1, d: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ d: { $gte: 1 } }, { $and: [{ x: { $in: [1] } }] }] }, { a: 1 }, { x: [1], a: 1, d: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { x: { $all: ["Tom", "Mike"] } }, { x: ["Tom", "Mike"], a: 1 } );
      //upsertandmerger( cl, { $set : { a : 1 } }, { $or : [{ x : { $et : 1 } }] }, { x:1, a:1} ); 
      upsertandmerger( cl, { $set: { a: 1 } }, { $or: [{ x: { $et: 1 } }, { $and: [{ b: { $et: 1 } }] }] }, { a: 1 } );
      upsertandmerger( cl, { $set: { a: 1, x: 1 } }, { c: { $gte: 1 } }, { x: 1, a: 1 }, { x: 1, a: 1, c: 1 } );
      upsertandmerger( cl, { $set: { a: 1, x: 1 } }, { c: { $in: [1] } }, { a: 1, x: 1 }, { x: 1, a: 1, c: 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { x: [1, 2] }, { "x": [1, 2], "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { x: { $et: /abc/ } }, { "x": /abc/, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": 1 }, { "x": { "x": 1 }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": { $et: 1 } }, { "x": { "x": 1 }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": { $all: [1] } }, { "x": { "x": [1] }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ "x.x": { $et: 1 } }] }, { "x": { "x": 1 }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": 1, "x.y": 1 }, { "x": { "x": 1, "y": 1 }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": 1, "x.y.z": 1 }, { "x": { "x": 1, "y": { "z": 1 } }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": [] }, { "x": { "x": [] }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { x: { x: [] } }, { "x": { "x": [] }, "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { x: [{ x: 1 }] }, { "x": [{ "x": 1 }], "a": 1 } );
      upsertandmerger( cl, { $set: { a: 1 } }, { "x.x.x": { $et: 1 } }, { "x": { "x": { "x": 1 } }, "a": 1 } );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName );
      db.close();
   }
}

main()
