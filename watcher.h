#ifndef NODE_WATCHER_H
#define NODE_WATCHER_H

#include <nan.h>

#include <v8.h>
#include <node.h>

#include "MediaController.h"

using namespace node;
using namespace v8;


class Watcher : public ObjectWrap{
public:
	static Persistent<FunctionTemplate> constructor_template;
	static uv_async_t async;
	static PLT_UPnP upnp;
    static PLT_CtrlPointReference ctrlPoint;
    static void Init(Handle<Object> target);
    static void AsyncCB(uv_async_t *handle, int status /*UNUSED*/);


protected:
    MediaController* mc;

	

	Watcher() : ObjectWrap() {};
	~Watcher(){};


	static NAN_METHOD(New);
	static NAN_METHOD(Browse);
	static NAN_METHOD(GetTracks);
	static NAN_METHOD(GetTrackPosition);
	static NAN_METHOD(SetRenderer);
	static NAN_METHOD(OpenTrack);
	static NAN_METHOD(OpenNextTrack);

};

#endif