var Watcher = require("./build/Debug/watcher.node").Watcher;
var EventEmitter = require('events').EventEmitter;

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}


console.log(Watcher)
inherits(Watcher, EventEmitter);
var mw = new Watcher();

mw.on("serverAdded",function(ev){
	if(ev.uuid == "7076436f-6e65-1063-8074-4ce6766160b7"){
		console.log("get tracks")
		mw.getTracks(ev.uuid,"1$268435466",function(){
			console.log("got results")
			console.log(arguments)
		})
	}
})

mw.on("rendererAdded",function(ev){
	console.log("rendererAdded",ev)
})