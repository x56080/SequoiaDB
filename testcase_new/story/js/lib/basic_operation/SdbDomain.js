var tmpSdbDomain = {
   addGroups: SdbDomain.prototype.addGroups,
   alter: SdbDomain.prototype.alter,
   help: SdbDomain.prototype.help,
   listCollectionSpaces: SdbDomain.prototype.listCollectionSpaces,
   listCollections: SdbDomain.prototype.listCollections,
   listGroups: SdbDomain.prototype.listGroups,
   removeGroups: SdbDomain.prototype.removeGroups,
   setAttributes: SdbDomain.prototype.setAttributes,
   setGroups: SdbDomain.prototype.setGroups,
   toString: SdbDomain.prototype.toString
};
var funcSdbDomain = SdbDomain;
var funcSdbDomainhelp = SdbDomain.help;
SdbDomain=function(){try{return funcSdbDomain.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
SdbDomain.help = function(){try{ return funcSdbDomainhelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
SdbDomain.prototype.addGroups=function(){try{return tmpSdbDomain.addGroups.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.alter=function(){try{return tmpSdbDomain.alter.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.help=function(){try{return tmpSdbDomain.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.listCollectionSpaces=function(){try{return tmpSdbDomain.listCollectionSpaces.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.listCollections=function(){try{return tmpSdbDomain.listCollections.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.listGroups=function(){try{return tmpSdbDomain.listGroups.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.removeGroups=function(){try{return tmpSdbDomain.removeGroups.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.setAttributes=function(){try{return tmpSdbDomain.setAttributes.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.setGroups=function(){try{return tmpSdbDomain.setGroups.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
SdbDomain.prototype.toString=function(){try{return tmpSdbDomain.toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
