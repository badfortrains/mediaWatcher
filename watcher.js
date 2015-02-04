var Watcher = require("./build/Debug/watcher.node").Watcher;
var EventEmitter = require('events').EventEmitter;

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}


inherits(Watcher, EventEmitter);
var mw = new Watcher();
var renderer;
var track;

mw.on("serverAdded",function(ev){
	console.log("serverAdded",ev)
	if(ev.uuid == "7076436f-6e65-1063-8074-4ce6766160b7"){
		console.log("get tracks")
		mw.getTracks(ev.uuid,"1$12$1013212280",function(err,tracks){
			console.log("got results")
			console.log(arguments)
			console.log("open",tracks[0])
			if(renderer){
				mw.openTrack(tracks[0],function(){
					console.log("open res,",arguments);
				})
			}else{
				track = tracks[0]
			}
		})
	}
})

mw.on("rendererAdded",function(ev){
	renderer = ev;
	console.log("rendererAdded",ev)
	console.log("setRenderer")
	mw.setRenderer(ev.uuid,function(){
		console.log("setRenderer result", arguments);
		console.log("getTrackPosition")
		mw.getTrackPosition(function(){
			console.log("get track position",arguments);
		});

		if(track){
			console.log("use resources",track.Resources)
			mw.openTrack(track,function(){
				console.log("open res,",arguments);
			})
		}
		console.log("fired get track position")
	})
})

