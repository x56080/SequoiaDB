/******************************************************************************
 * @Description   : ft disk detect public function
 * @Author        : fangjiabin
 * @CreateTime    : 2025.08.29
 * @LastEditTime  : 2025.08.29
 * @LastEditors   : fangjiabin
 ******************************************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

function checkParameter( parameter, expect )
{
   try{
      var fileJson = {} ;
      fileJson[parameter] = "" ;
      var cursor = db.snapshot( 13, {}, fileJson );
      while( cursor.next() )
      {
         assert.equal( cursor.current().toObj()[parameter], expect );
      }
      cursor.close();
   }catch(e){
      throw new Error("expect error:" + e);
   }
}

function checkUpdateParameter( parameter, value, expect, err)
{
   try{
      var fileJson1 = {} ;
      fileJson1[parameter] = value ;
      var fileJson2 = {} ;
      fileJson2[parameter] = "" ;
      db.updateConf( fileJson1 );
      var cursor = db.snapshot( 13, {}, fileJson2 );
      while( cursor.next() )
      {
         assert.equal( cursor.current().toObj()[parameter], expect );
      }
      cursor.close();
      if ( typeof(err) !== "undefined" )
      {
         throw new Error("expect error:" + getErr(err));
      }
   }catch(e){
     if (typeof(err) !== "undefined"){
          assert.equal( e, err );
     }else{
        throw e;
     }
  }
}