var tmpBSONArray = {
   _formatStr: BSONArray.prototype._formatStr,
   arrayAccess: BSONArray.prototype.arrayAccess,
   help: BSONArray.prototype.help,
   index: BSONArray.prototype.index,
   more: BSONArray.prototype.more,
   next: BSONArray.prototype.next,
   pos: BSONArray.prototype.pos,
   size: BSONArray.prototype.size,
   toArray: BSONArray.prototype.toArray,
   toString: BSONArray.prototype.toString
};
var funcBSONArray = BSONArray;
var funcBSONArrayhelp = BSONArray.help;
BSONArray=function(){try{return funcBSONArray.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
BSONArray.help = function(){try{ return funcBSONArrayhelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
BSONArray.prototype._formatStr=function(){try{return tmpBSONArray._formatStr.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.arrayAccess=function(){try{return tmpBSONArray.arrayAccess.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.help=function(){try{return tmpBSONArray.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.index=function(){try{return tmpBSONArray.index.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.more=function(){try{return tmpBSONArray.more.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.next=function(){try{return tmpBSONArray.next.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.pos=function(){try{return tmpBSONArray.pos.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.size=function(){try{return tmpBSONArray.size.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.toArray=function(){try{return tmpBSONArray.toArray.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
BSONArray.prototype.toString=function(){try{return tmpBSONArray.toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
