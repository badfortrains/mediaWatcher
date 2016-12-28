#include <node.h>
#include <uv.h>

#include "watcher.h"

Nan::Persistent<v8::Function> Watcher::constructor;
uv_async_t Watcher::async;
PLT_UPnP Watcher::upnp;
PLT_CtrlPointReference Watcher::ctrlPoint;

NAN_MODULE_INIT(Watcher::Init) {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Watcher").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "browse", Browse);
  Nan::SetPrototypeMethod(tpl, "getTracks", GetTracks);
  Nan::SetPrototypeMethod(tpl, "getPosition", GetTrackPosition);
  Nan::SetPrototypeMethod(tpl, "setRenderer", SetRenderer);
  Nan::SetPrototypeMethod(tpl, "openTrack", OpenTrack);
  Nan::SetPrototypeMethod(tpl, "openNextTrack", OpenNextTrack);
  Nan::SetPrototypeMethod(tpl, "play", Play);
  Nan::SetPrototypeMethod(tpl, "stop", Stop);
  Nan::SetPrototypeMethod(tpl, "pause", Pause);
  Nan::SetPrototypeMethod(tpl, "next", Next);
  Nan::SetPrototypeMethod(tpl, "setVolume", SetVolume);
  Nan::SetPrototypeMethod(tpl, "seek", Seek);
  Nan::SetPrototypeMethod(tpl, "getRenderers", GetRenderers);
  Nan::SetPrototypeMethod(tpl, "getServers", GetServers);


  NPT_LogManager::GetDefault().Configure("plist:.level=FINE;.handlers=ConsoleHandler;.ConsoleHandler.colors=off;.ConsoleHandler.filter=24");
  Watcher::ctrlPoint = new PLT_CtrlPoint();
  Watcher::upnp.AddCtrlPoint(Watcher::ctrlPoint);
  Watcher::upnp.Start();

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Watcher").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

void Watcher::AsyncCB(uv_async_t *handle, int status /*UNUSED*/){
	Watcher* watcher = (Watcher*) handle->data;
    watcher->mc->FlushQueue(watcher);
}

NAN_METHOD(Watcher::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowTypeError("Use the new operator to create new Watcher objects");
  }

  Watcher* mw = new Watcher();
  mw->Wrap(info.This());
  

  uv_async_init(uv_default_loop(),&(Watcher::async), (uv_async_cb)Watcher::AsyncCB);
  Watcher::async.data = (void*) mw;

  mw->mc = new MediaController(Watcher::ctrlPoint,&Watcher::async);

  info.GetReturnValue().Set(info.This());
}

//uuid, dir, callback
NAN_METHOD(Watcher::Browse){
    if (info.Length() < 3) {
        return Nan::ThrowTypeError("Expected 3 arguments");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

    String::Utf8Value uuid(info[0]);
    String::Utf8Value dir(info[1]);

    watcher->mc->BrowseDirectory(callback,*uuid,*dir);
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::GetTracks){
    

    if (info.Length() < 3) {
        return Nan::ThrowTypeError("Expected 3 arguments");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

    String::Utf8Value uuid(info[0]);
    String::Utf8Value dir(info[1]);

    watcher->mc->GetTracks(callback,*uuid,*dir);
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::GetTrackPosition){
    

    if( info.Length() < 1 || !info[0]->IsFunction()){
        return Nan::ThrowTypeError("Expected a callback function");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[0].As<Function>());

    GetTrackPositionAction* action = new GetTrackPositionAction(callback);
    NPT_Result res = watcher->mc->GetTrackPosition(action);

    if(!NPT_SUCCEEDED(res)){
        action->ErrorCB(res);
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::SetRenderer){
    

    if( info.Length() < 1 ){
        return Nan::ThrowTypeError("Expected at least 1 argument (uuid)");
    }
    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    String::Utf8Value uuid(info[0]);

    NPT_Result res = watcher->mc->SetRenderer(*uuid);

    if( info.Length() > 1 && info[1]->IsFunction()){
        Local<Function> callbackHandle = info[1].As<Function>();
        Handle<Value> argv[1];
        if(NPT_SUCCEEDED(res))
            argv[0] = Nan::Null();
        else
            argv[0] = Nan::Error("Renderer not found");

         Nan::MakeCallback(Nan::GetCurrentContext()->Global(), callbackHandle,1, argv);
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::OpenTrack){
    

    if( info.Length() < 1 ){
        return Nan::ThrowTypeError("Expected at least 1 argument (track object)");
    }

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    CBAction* action = NULL;

    if( info.Length() > 1 && info[1]->IsFunction()){
        Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
        action = new CBAction(callback);
    }


    Local<Object> track = info[0]->ToObject();
    Local<String> key = Nan::New<String>("Resources").ToLocalChecked();
    Local<Array> resArray = Local<Array>::Cast(track->Get(key));
    NPT_Array<PLT_MediaItemResource> resource(resArray->Length());

    for(unsigned int i=0; i < resArray->Length(); i++){
        PLT_MediaItemResource curRes;
        Local<String> key = Nan::New<String>("Uri").ToLocalChecked();
        curRes.m_Uri = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(key));
        Local<String> protocolInfo = Nan::New<String>("ProtocolInfo").ToLocalChecked();
        curRes.m_ProtocolInfo = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(protocolInfo));
        resource.Add(curRes);
    }
    Local<String> didlKey = Nan::New<String>("ProtocolInfo").ToLocalChecked();
    NPT_String didl(*String::Utf8Value(track->Get(didlKey)));

    NPT_Result res = watcher->mc->OpenTrack(resource,didl,action);

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::OpenNextTrack){
    

    if( info.Length() < 1 ){
        return Nan::ThrowTypeError("Expected at least 1 argument (track object)");
    }

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    CBAction* action = NULL;

    OPTIONAL_CB_ACTION(1,action);


    Local<Object> track = info[0]->ToObject();
    Local<String> key = Nan::New<String>("Resources").ToLocalChecked();
    Local<Array> resArray = Local<Array>::Cast(track->Get(key));
    NPT_Array<PLT_MediaItemResource> resource(resArray->Length());

    for(unsigned int i=0; i < resArray->Length(); i++){
        PLT_MediaItemResource curRes;
        Local<String> uriKey = Nan::New<String>("Uri").ToLocalChecked();
        curRes.m_Uri = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(uriKey));
        Local<String> pIkey = Nan::New<String>("ProtocolInfo").ToLocalChecked();
        curRes.m_ProtocolInfo = *String::Utf8Value(resArray->Get(i)->ToObject()->Get(pIkey));
        resource.Add(curRes);
    }
    Local<String> didlKey = Nan::New<String>("Didl").ToLocalChecked();
    NPT_String didl(*String::Utf8Value(track->Get(didlKey)));

    NPT_Result res = watcher->mc->OpenNextTrack(resource,didl,action);

    if(!NPT_SUCCEEDED(res) && action != NULL){
        action->ErrorCB(res);
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::Play){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
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
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::Stop){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
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
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::Pause){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
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
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::Next){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
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
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}


NAN_METHOD(Watcher::SetVolume){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
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
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::Seek){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
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
        delete action;
    }
    info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(Watcher::GetRenderers){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    PLT_DeviceMap devices = watcher->mc->GetMRs();
    const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
    Local<Array> res = Nan::New<Array>(entries.GetItemCount());
    NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
    int i =0;
    while(entry){
        Local<Object> tempDevice = Nan::New<Object>();
        PLT_DeviceDataReference device = (*entry)->GetValue();
        tempDevice->Set(Nan::New("uuid").ToLocalChecked(),Nan::New<String>(device->GetUUID()).ToLocalChecked());
        tempDevice->Set(Nan::New("name").ToLocalChecked(),Nan::New<String>(device->GetFriendlyName()).ToLocalChecked());
        res->Set(Nan::New<Integer>(i),tempDevice);
        i++;
        entry++;
    }

    info.GetReturnValue().Set(res);
}

NAN_METHOD(Watcher::GetServers){
    

    Watcher* watcher = ObjectWrap::Unwrap<Watcher>(info.Holder());
    PLT_DeviceMap devices = watcher->mc->GetMSs();
    const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
    Local<Array> res = Nan::New<Array>(entries.GetItemCount());
    NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
    int i =0;
    while(entry){
        Local<Object> tempDevice = Nan::New<Object>();
        PLT_DeviceDataReference device = (*entry)->GetValue();
        tempDevice->Set(Nan::New("uuid").ToLocalChecked(),Nan::New<String>(device->GetUUID()).ToLocalChecked());
        tempDevice->Set(Nan::New("name").ToLocalChecked(),Nan::New<String>(device->GetFriendlyName()).ToLocalChecked());
        res->Set(Nan::New<Integer>(i),tempDevice);
        i++;
        entry++;
    }

    info.GetReturnValue().Set(res);
}


extern "C" {
  static void init (Handle<Object> target)
  {
    Watcher::Init(target);
  }

  NODE_MODULE(watcher, init);
}
