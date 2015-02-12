#include <node.h>
#include <uv.h>

#include "watcher.h"


void Watcher::Init(Handle<Object> target){
	NanScope();

	Local<FunctionTemplate> t = NanNew<FunctionTemplate>(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(NanNew("Watcher"));

    NanAssignPersistent(constructor_template, t);

    NPT_LogManager::GetDefault().Configure("plist:.level=FINE;.handlers=ConsoleHandler;.ConsoleHandler.colors=off;.ConsoleHandler.filter=24");
    Watcher::ctrlPoint = new PLT_CtrlPoint();
    Watcher::upnp.AddCtrlPoint(Watcher::ctrlPoint);
    Watcher::upnp.Start();

    NODE_SET_PROTOTYPE_METHOD(t, "browse", Browse);
    NODE_SET_PROTOTYPE_METHOD(t, "getTracks", GetTracks);
    NODE_SET_PROTOTYPE_METHOD(t, "getPosition", GetTrackPosition);
    NODE_SET_PROTOTYPE_METHOD(t, "setRenderer", SetRenderer);
    NODE_SET_PROTOTYPE_METHOD(t, "openTrack", OpenTrack);
    NODE_SET_PROTOTYPE_METHOD(t, "openNextTrack", OpenNextTrack);
    NODE_SET_PROTOTYPE_METHOD(t, "play", Play);
    NODE_SET_PROTOTYPE_METHOD(t, "stop", Stop);
    NODE_SET_PROTOTYPE_METHOD(t, "pause", Pause);
    NODE_SET_PROTOTYPE_METHOD(t, "next", Next);
    NODE_SET_PROTOTYPE_METHOD(t, "setVolume", SetVolume);
    NODE_SET_PROTOTYPE_METHOD(t, "seek", Seek);
    NODE_SET_PROTOTYPE_METHOD(t, "getRenderers", GetRenderers);

    target->Set(NanNew("Watcher"),
        t->GetFunction());
}

void Watcher::AsyncCB(uv_async_t *handle, int status /*UNUSED*/){
	Watcher* watcher = (Watcher*) handle->data;
    watcher->mc->FlushQueue(watcher);
}

NAN_METHOD(Watcher::New) {
    NanScope();

    if (!args.IsConstructCall()) {
        return NanThrowTypeError("Use the new operator to create new Watcher objects");
    }

    Watcher* mw = new Watcher();
    mw->Wrap(args.This());

    uv_async_init(uv_default_loop(),&(Watcher::async), (uv_async_cb)Watcher::AsyncCB);
    Watcher::async.data = (void*) mw;

    mw->mc = new MediaController(Watcher::ctrlPoint,&Watcher::async);

    NanReturnValue(args.This());
}

//uuid, dir, callback
NAN_METHOD(Watcher::Browse){
    NanScope();

    if (args.Length() < 3) {
        return NanThrowTypeError("Expected 3 arguments");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    Local<Function> callbackHandle = args[2].As<Function>();
    NanCallback *callback = new NanCallback(callbackHandle);

    String::Utf8Value uuid(args[0]);
    String::Utf8Value dir(args[1]);

    watcher->mc->BrowseDirectory(callback,*uuid,*dir);
    NanReturnUndefined();
}

NAN_METHOD(Watcher::GetTracks){
    NanScope();

    if (args.Length() < 3) {
        return NanThrowTypeError("Expected 3 arguments");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    Local<Function> callbackHandle = args[2].As<Function>();
    NanCallback *callback = new NanCallback(callbackHandle);

    String::Utf8Value uuid(args[0]);
    String::Utf8Value dir(args[1]);

    watcher->mc->GetTracks(callback,*uuid,*dir);
    NanReturnUndefined();
}

NAN_METHOD(Watcher::GetTrackPosition){
    NanScope();

    if( args.Length() < 1 || !args[0]->IsFunction()){
        return NanThrowTypeError("Expected a callback function");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    Local<Function> callbackHandle = args[0].As<Function>();
    NanCallback *callback = new NanCallback(callbackHandle);

    GetTrackPositionAction* action = new GetTrackPositionAction(callback);
    NPT_Result res = watcher->mc->GetTrackPosition(action);

    if(!NPT_SUCCEEDED(res)){
        action->ErrorCB(res);
        delete action;
    }   
    NanReturnUndefined();
}

NAN_METHOD(Watcher::SetRenderer){
    NanScope();

    if( args.Length() < 1 ){
        return NanThrowTypeError("Expected at least 1 argument (uuid)");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    String::Utf8Value uuid(args[0]);

    NPT_Result res = watcher->mc->SetRenderer(*uuid);

    if( args.Length() > 1 && args[1]->IsFunction()){
        Local<Function> callbackHandle = args[1].As<Function>();
        Handle<Value> argv[1];
        if(NPT_SUCCEEDED(res))
            argv[0] = NanNull();
        else
            argv[0] = NanError("Renderer not found");

         NanMakeCallback(NanGetCurrentContext()->Global(),NanNew(callbackHandle),1, argv); 
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::OpenTrack){
    NanScope();

    if( args.Length() < 1 ){
        return NanThrowTypeError("Expected at least 1 argument (track object)");
    }

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    if( args.Length() > 1 && args[1]->IsFunction()){
        Local<Function> callbackHandle = args[1].As<Function>();
        NanCallback *callback = new NanCallback(callbackHandle);
        action = new CBAction(callback);
    }


    Local<Object> track = args[0]->ToObject();
    Local<Array> resArray = Local<Array>::Cast(track->Get(NanNew("Resources")));
    NPT_Array<PLT_MediaItemResource> resource(resArray->Length());

    for(unsigned int i=0; i < resArray->Length(); i++){
        PLT_MediaItemResource curRes;
        curRes.m_Uri = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(NanNew("Uri")));
        curRes.m_ProtocolInfo = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(NanNew("ProtocolInfo")));
        resource.Add(curRes);
    }
    NPT_String didl(*String::Utf8Value(track->Get(NanNew("Didl"))));

    NPT_Result res = watcher->mc->OpenTrack(resource,didl,action);

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::OpenNextTrack){
    NanScope();

    if( args.Length() < 1 ){
        return NanThrowTypeError("Expected at least 1 argument (track object)");
    }

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    OPTIONAL_CB_ACTION(1,action);


    Local<Object> track = args[0]->ToObject();
    Local<Array> resArray = Local<Array>::Cast(track->Get(NanNew("Resources")));
    NPT_Array<PLT_MediaItemResource> resource(resArray->Length());

    for(unsigned int i=0; i < resArray->Length(); i++){
        PLT_MediaItemResource curRes;
        curRes.m_Uri = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(NanNew("Uri")));
        curRes.m_ProtocolInfo = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(NanNew("ProtocolInfo")));
        resource.Add(curRes);
    }
    NPT_String didl(*String::Utf8Value(track->Get(NanNew("Didl"))));

    NPT_Result res = watcher->mc->OpenNextTrack(resource,didl,action);

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::Play){
    NanScope();

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    OPTIONAL_CB_ACTION(0,action);

    NPT_Result res = NPT_ERROR_NO_SUCH_ITEM;;
    PLT_DeviceDataReference device;
    watcher->mc->GetCurMR(device);
    if (!device.IsNull()) {
        res = watcher->mc->Play(device, 0, "1", action);
    }

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::Stop){
    NanScope();

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    OPTIONAL_CB_ACTION(0,action);

    NPT_Result res = NPT_ERROR_NO_SUCH_ITEM;;
    PLT_DeviceDataReference device;
    watcher->mc->GetCurMR(device);
    if (!device.IsNull()) {
        res = watcher->mc->Stop(device, 0, action);
    }

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::Pause){
    NanScope();

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    OPTIONAL_CB_ACTION(0,action);

    NPT_Result res = NPT_ERROR_NO_SUCH_ITEM;;
    PLT_DeviceDataReference device;
    watcher->mc->GetCurMR(device);
    if (!device.IsNull()) {
        res = watcher->mc->Pause(device, 0, action);
    }

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::Next){
    NanScope();

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    OPTIONAL_CB_ACTION(0,action);

    NPT_Result res = NPT_ERROR_NO_SUCH_ITEM;;
    PLT_DeviceDataReference device;
    watcher->mc->GetCurMR(device);
    if (!device.IsNull()) {
        res = watcher->mc->Next(device, 0, action);
    }

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}


NAN_METHOD(Watcher::SetVolume){
    NanScope();

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    REQUIRE_ARGUMENT_INTEGER(0,volume)
    OPTIONAL_CB_ACTION(1,action);

    NPT_Result res = NPT_ERROR_NO_SUCH_ITEM;;
    PLT_DeviceDataReference device;
    watcher->mc->GetCurMR(device);
    if (!device.IsNull()) {
        res = watcher->mc->SetVolume(device, 0, "Master", volume, action);
    }

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::Seek){
    NanScope();

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    CBAction* action = NULL;

    REQUIRE_ARGUMENT_STRING(0,targetTime)
    OPTIONAL_CB_ACTION(1,action);

    NPT_Result res = NPT_ERROR_NO_SUCH_ITEM;;
    PLT_DeviceDataReference device;
    watcher->mc->GetCurMR(device);
    if (!device.IsNull()) {
        res = watcher->mc->Seek(device, 0,"REL_TIME",*targetTime, action);
    }

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
    }
    NanReturnUndefined();
}

NAN_METHOD(Watcher::GetRenderers){
    NanScope();

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(args.Holder());
    PLT_DeviceMap devices = watcher->mc->GetMRs();
    const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
    Local<Array> res = NanNew<Array>(entries.GetItemCount());
    NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
    int i =0;
    while(entry){
        Local<Object> tempDevice = NanNew<Object>();
        PLT_DeviceDataReference device = (*entry)->GetValue();
        tempDevice->Set(NanNew("uuid"),NanNew<String>(device->GetUUID()));
        tempDevice->Set(NanNew("name"),NanNew<String>(device->GetFriendlyName()));
        res->Set(NanNew<Integer>(i),tempDevice);
        i++;
        ++entry;
    }

    NanReturnValue(res);
}


Persistent<FunctionTemplate> Watcher::constructor_template;
uv_async_t Watcher::async;
PLT_UPnP Watcher::upnp;
PLT_CtrlPointReference Watcher::ctrlPoint;


extern "C" {
  static void init (Handle<Object> target)
  {
    Watcher::Init(target);
  }

  NODE_MODULE(watcher, init);
}