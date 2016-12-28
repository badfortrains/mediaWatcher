#include "actions.h"
#include <nan.h>
using namespace v8;

CBAction::~CBAction(){
    delete callback;
}

void
CBAction::ErrorCB(NPT_Result err){
    Nan::HandleScope scope;
    Local<Value> argv[] = {
      Nan::Error(NPT_ResultText(err))
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
    Nan::HandleScope scope;
    Local<Value> argv[] = {
      Nan::Null()
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
    Nan::HandleScope scope;
    Local<Object> event = Nan::New<Object>();
    event->Set(Nan::New<String>("uuid").ToLocalChecked(),Nan::New<String>(uuid).ToLocalChecked());
    event->Set(Nan::New<String>("baseUrl").ToLocalChecked(),Nan::New<String>(baseUrl).ToLocalChecked());
    event->Set(Nan::New<String>("iconUrl").ToLocalChecked(),Nan::New<String>(iconUrl).ToLocalChecked());
    event->Set(Nan::New<String>("name").ToLocalChecked(),Nan::New<String>(deviceName).ToLocalChecked());

    Local<Value> args[] = { Nan::New<String>(eventName).ToLocalChecked(),event };
    EMIT_EVENT(context->handle(), 2, args);
}

void 
GetTrackPositionAction::GotResult(PLT_PositionInfo positionInfo){
    res = NPT_SUCCESS;
    info = positionInfo;
}

void
GetTrackPositionAction::SuccessCB(){
    Local<Object> position = Nan::New<Object>();
    position->Set(Nan::New<String>("position").ToLocalChecked(), Nan::New<Number>(info.rel_time.ToMillis()));
    position->Set(Nan::New<String>("duration").ToLocalChecked(), Nan::New<Number>(info.track_duration.ToMillis()));

    Handle<Value> argv[] = {
        Nan::Null(),
        position
    };
    callback->Call(2, argv);
}

StateVariableAction::StateVariableAction(PLT_Service* service, PLT_StateVariable* var, EventSource deviceType){
    name = var->GetName();
    value = var->GetValue();
    uuid = service->GetDevice()->GetUUID();
    sourceType = deviceType;
}

void
StateVariableAction::EmitAction(ObjectWrap* context){
    Nan::HandleScope scope;
    Local<Object> event = Nan::New<Object>();
    event->Set(Nan::New<String>("name").ToLocalChecked(),Nan::New<String>(name).ToLocalChecked());
    event->Set(Nan::New<String>("value").ToLocalChecked(),Nan::New<String>(value).ToLocalChecked());
    event->Set(Nan::New<String>("uuid").ToLocalChecked(),Nan::New<String>(uuid).ToLocalChecked());
    event->Set(Nan::New<String>("sourceType").ToLocalChecked(),sourceType == RENDERER ? Nan::New<String>("renderer").ToLocalChecked() : Nan::New<String>("server").ToLocalChecked());

    Local<Value> args[] = { Nan::New<String>("stateChange").ToLocalChecked(),event };
    EMIT_EVENT(context->handle(), 2, args);
}