var tmpObjectId = {
   help: ObjectId.prototype.help,
   toString: ObjectId.prototype.toString
};
var funcObjectId = ObjectId;
var funcObjectIdhelp = ObjectId.help;
ObjectId=function(){try{return funcObjectId.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
ObjectId.help = function(){try{ return funcObjectIdhelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
ObjectId.prototype.help=function(){try{return tmpObjectId.help.apply(this,arguments);}catch(e){commThrowError(e);}};
ObjectId.prototype.toString=function(){try{return tmpObjectId.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
