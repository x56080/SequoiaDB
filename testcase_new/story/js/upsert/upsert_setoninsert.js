/*******************************************************************************
*@Description: JavaScript common function library
*@Modify list:
*   2014-4-21 wenjing wang Init
*******************************************************************************/

/*******************************************************************************
*@Description：setOnInsert存在时，执行upsert操作
*@Input：rule为更新规则文档
cond为查询条件
hint 强制使用的索引名
setoninsert 不存在时插入的对象
res为结果文档
errres 错误的结果
*@Expectation：upsert后生成的新文档为res
********************************************************************************/
function upsertandmerger( cl, rule, cond, hint, setoninsert, res, errres )
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
      cl.upsert( rule, cond, hint, setoninsert ); 
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
         println( "upsert( " + BuildObjStr( rule )+ ", " + BuildObjStr( cond )+ 
         BuildObjStr( hint )+ BuildObjStr( setoninsert )+ " )" ); 
         throw buildException( funname, e, "find( " + BuildObjStr( res )+ " )", 1, docnum ); 
      }
      else if( -2 == e && undefined != errres )
      {
         println( "upsert( " + BuildObjStr( rule )+ ", " + BuildObjStr( cond )+ 
         BuildObjStr( hint )+ BuildObjStr( setoninsert )+ " )" ); 
         throw buildException( funname, e, "find( " + BuildObjStr( errres )+ " )", 0, docnum ); 
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

function main()
{
   try
   {
      var clName = COMMCLNAME + "_upsertsetoninsert"; 
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME ); 
      var cl = commCreateCL( db, COMMCSNAME, clName ); 
      db.setSessionAttr( {PreferedInstance:"M"} ); 
      
      upsertandmerger( cl, {$set:{c:1, d:1}}, {}, {}, {_id:1}, 
      {c:1, d:1, _id:1} ); 
      upsertandmerger( cl, {$set:{a:3}}, {a:2}, {}, {a:1}, 
      {a:1} ); 
      cl.insert( {_id:1, a:1} ); 
      upsertandmerger( cl, {$set:{a:2}}, {_id:1}, {}, {msg:"test"}, 
      {_id:1, a:2}, {_id:1, a:2, msg:"test"} ); 
      upsertandmerger( cl, {$set:{a:1}}, {b:1}, {}, {c:{d:1}}, {a:1, b:1, c:{d:1}} ); 
      upsertandmerger( cl, {$set:{a:1}}, {b:1}, {}, {"c.d":1}, 
      {a:1, b:1, c:{d:1}} ); 
      upsertandmerger( cl, {$set:{a:1}}, {b:1}, {}, {c:[1]}, 
      {a:1, b:1, c:[1]} ); 
      cl.createIndex( 'aidx', {a:1}, false ); 
      upsertandmerger( cl, {$set:{a:1}}, {}, {"":'aidx'}, {_id:1}, 
      {a:1, _id:1} ); 
      upsertandmerger( cl, {$set:{a:1}}, {"_id.b":1}, {}, {"_id.a": new Date()}, 
      {a:1, "_id.b":1} ); 
      upsertandmerger( cl, {$set:{"_id.a":1}}, {"_id.b":1}, {}, {"_id.c": 1}, 
      {_id:{a:1, b:1, c:1}} ); 
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
