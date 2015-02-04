#include "Actions.h"
#include <nan.h>
using namespace v8;

CBAction::~CBAction(){
    delete callback;
}

void
CBAction::ErrorCB(NPT_Result err){
    Handle<Value> argv[] = {
      NanError(NPT_ResultText(err))
    };
    callback->Call(1, argv);
}

void
CBAction::EmitAction(ObjectWrap* context){
    if(!NPT_SUCCEEDED(res))
        ErrorCB(res);
    else
        SuccessCB();
}

void
CBAction::SuccessCB(){
    Handle<Value> argv[] = {
      NanNull()
    };
    callback->Call(1, argv);
}

void
CBAction::SetResult(NPT_Result err){
    res = err;
}

DeviceAction::DeviceAction(NPT_String eventName, PLT_DeviceDataReference& device) :
    eventName(eventName)
{
    deviceName = device->GetFriendlyName();
    uuid = device->GetUUID();
    baseUrl = device->GetIconUrl();
    iconUrl = device->GetURLBase().ToString();
}

void 
DeviceAction::EmitAction(ObjectWrap* context){
    Local<Object> event = Object::New();
    event->Set(NanNew("uuid"),NanNew<String>(uuid));
    event->Set(NanNew("baseUrl"),NanNew<String>(baseUrl));
    event->Set(NanNew("iconUrl"),NanNew<String>(iconUrl));
    event->Set(NanNew("name"),NanNew<String>(deviceName));

    Local<Value> args[] = { NanNew<String>(eventName),event };
    EMIT_EVENT(NanObjectWrapHandle(context), 2, args);
}

void 
GetTrackPositionAction::GotResult(PLT_PositionInfo positionInfo){
    res = NPT_SUCCESS;
    info = positionInfo;
}

void
GetTrackPositionAction::SuccessCB(){
    Local<Object> position = Object::New();
    position->Set(NanNew("position"), NanNew<Number>(info.rel_time.ToMillis()));
    position->Set(NanNew("duration"), NanNew<Number>(info.track_duration.ToMillis()));

    Handle<Value> argv[] = {
        NanNull(),
        position
    };
    callback->Call(2, argv);
}
