#include <uv.h>
#include "MediaController.h"

MediaController::MediaController(PLT_CtrlPointReference& ctrlPoint, uv_async_t* async) :
    PLT_SyncMediaBrowser(ctrlPoint),
    PLT_MediaController(ctrlPoint),
    async(async)
{
    PLT_MediaController::SetDelegate(this);
}

void
MediaController::OnMRStateVariablesChanged(PLT_Service*  service, NPT_List<PLT_StateVariable*>*  vars){

    NPT_List<PLT_StateVariable*>::Iterator item = vars->GetFirstItem();
    while (item) {
        queue.push(new StateVariableAction(service,*item,RENDERER));
    }
    uv_async_send(async);
}

void
MediaController::OnMSStateVariablesChanged(PLT_Service*  service, NPT_List<PLT_StateVariable*>*  vars){
    NPT_List<PLT_StateVariable*>::Iterator item = vars->GetFirstItem();
    while (item) {
        queue.push(new StateVariableAction(service,*item,SERVER));
    }
    uv_async_send(async);
}

bool 
MediaController::OnMSAdded(PLT_DeviceDataReference& device) 
{   
    NPT_String uuid = device->GetUUID();

    // Issue special action upon discovering MediaConnect server
    PLT_Service* service;
    if (NPT_SUCCEEDED(device->FindServiceByType("urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:*", service))) {
        PLT_ActionReference action;
        PLT_SyncMediaBrowser::m_CtrlPoint->CreateAction(
            device, 
            "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
            "IsAuthorized", 
            action);
        if (!action.IsNull()) {
            action->SetArgumentValue("DeviceID", "");
            PLT_SyncMediaBrowser::m_CtrlPoint->InvokeAction(action, 0);
        }

        PLT_SyncMediaBrowser::m_CtrlPoint->CreateAction(
            device, 
            "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
            "IsValidated", 
            action);
        if (!action.IsNull()) {
            action->SetArgumentValue("DeviceID", "");
            PLT_SyncMediaBrowser::m_CtrlPoint->InvokeAction(action, 0);
        }
    }

    DeviceAction* event = new DeviceAction("serverAdded",device);
    queue.push(event);
    uv_async_send(async);

    return true; 
}

bool 
MediaController::OnMRAdded(PLT_DeviceDataReference& device) 
{   
    NPT_String uuid = device->GetUUID();
    PLT_Service* service;
    if (NPT_SUCCEEDED(device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:*", service))) {
        NPT_AutoLock lock(m_MediaRenderers);
        m_MediaRenderers.Put(uuid, device);

        DeviceAction* event = new DeviceAction("rendererAdded",device);
        queue.push(event);
        uv_async_send(async);
    }

    return true; 
}

void
MediaController::OnGetPositionInfoResult(NPT_Result res, PLT_DeviceDataReference& /* device */,PLT_PositionInfo* info,void* action)
{
    if (!action)
        return;

    GetTrackPositionAction* posAction = (GetTrackPositionAction*) action;
    if(NPT_SUCCEEDED(res)){
        posAction->GotResult(*info);
    }else{
        posAction->SetResult(res);
    }

    queue.push(posAction);
    uv_async_send(async);
}

void
MediaController::OnCBActionResult(NPT_Result res,void* action){
    if(!action)
        return;

    CBAction* cb = (CBAction*) action;
    cb->SetResult(res);

    queue.push(cb);
    uv_async_send(async);
}

class BrowseAction : public NanAsyncWorker {
public:
    BrowseAction(NanCallback *callback, NPT_String uuid,NPT_String objectId, MediaController* mc)
    : NanAsyncWorker(callback), uuid(uuid), objectId(objectId), mc(mc) {}
    ~BrowseAction() {}

    void Execute () {
        res = NPT_FAILURE;
        PLT_DeviceDataReference* device = NULL;
        mc->GetMediaServersMap().Get(uuid, device);
        if (device) {
            // send off the browse packet and block
            res = mc->BrowseSync(
                *device,
                objectId,
                resultList,
                false);
        }
    }

    void HandleOKCallback(){
        NanScope();
        if(NPT_SUCCEEDED(res)){
            Local<Array> dirArray = Array::New();
            if(!resultList.IsNull()){
                NPT_List<PLT_MediaObject*>::Iterator item = resultList->GetFirstItem();
                int i =0;
                while (item) {
                    Local<Object> temp = Object::New();
                    temp->Set(NanNew("_id"),NanNew<String>((*item)->m_ObjectID));
                    temp->Set(NanNew("title"),NanNew<String>((*item)->m_Title));
                    temp->Set(NanNew("isContainer"),NanNew<Boolean>((*item)->IsContainer()));
                    dirArray->Set(i++,temp);
                    ++item;
                }
            }
            Local<Value> argv[] = {
                NanNull()
              , dirArray
            };
            callback->Call(2, argv);
        }else{
            Local<Value> argv[] = {
                NanError("Failed to retrieve directory"),
            };
            callback->Call(1, argv);
        }
    }

protected:
    NPT_Result res;
    NPT_String uuid;
    NPT_String objectId;
    PLT_MediaObjectListReference resultList;
    MediaController* mc;
};

class GetTracksAction : public BrowseAction{
public:
    GetTracksAction(NanCallback *callback, NPT_String uuid,NPT_String objectId, MediaController* mc)
    : BrowseAction(callback,uuid,objectId,mc) {}
    ~GetTracksAction() {}


    Handle<Array> wrapResources(NPT_Array<PLT_MediaItemResource>* m_Resources){
        NanEscapableScope();
        Local<Array> resArray = Array::New((int)m_Resources->GetItemCount());
        for (NPT_Cardinal u=0; u<m_Resources->GetItemCount(); u++) {
            Local<Object> resObj = Object::New();
            NPT_String protocol = (*m_Resources)[u].m_ProtocolInfo.ToString();
            resObj->Set(NanNew("ProtocolInfo"),NanNew<String>(protocol));
            resObj->Set(NanNew("Uri"),NanNew<String>((*m_Resources)[u].m_Uri));
            resArray->Set(NanNew<Integer>(u),resObj);
        }
        return NanEscapeScope(resArray);
    }
    void HandleOKCallback(){
        NanScope();
        if(NPT_SUCCEEDED(res)){
            Local<Array> trackArray = Array::New((int)resultList->GetItemCount());
            int i =0;
            NPT_List<PLT_MediaObject*>::Iterator item = resultList->GetFirstItem();
            while (item) {
                Local<Object> temp = Object::New();
                if(!(*item)->IsContainer()){
                    temp->Set(NanNew("Resources"),wrapResources(&(*item)->m_Resources));
                    temp->Set(NanNew("Didl"),NanNew<String>((*item)->m_Didl));
                    temp->Set(NanNew("Title"),NanNew<String>((*item)->m_Title));
                    temp->Set(NanNew("Album"),NanNew<String>((*item)->m_Affiliation.album));
                    temp->Set(NanNew("TrackNumber"),NanNew<Integer>((*item)->m_MiscInfo.original_track_number));

                    NPT_List<PLT_PersonRole>::Iterator person = (*item)->m_People.artists.GetFirstItem();
                    if(person){
                        temp->Set(NanNew("Artist"),NanNew<String>((*item)->m_People.artists.GetFirstItem()->name));
                    }else{
                        temp->Set(NanNew("Artist"),NanNew("unknown"));
                    }
                }

                temp->Set(NanNew("isContainer"),NanNew<Boolean>((*item)->IsContainer()));  
                temp->Set(NanNew("oID"),NanNew<String>((*item)->m_ObjectID));
                trackArray->Set(i,temp);

                ++item;
                i++;
            }
            Local<Value> argv[] = {
                NanNull()
              , trackArray
            };
            callback->Call(2, argv);
        }else{
            Local<Value> argv[] = {
                NanError("Failed to retrieve directory"),
            };
            callback->Call(1, argv);
        }
    }

};

void 
MediaController::BrowseDirectory(NanCallback *callback, NPT_String uuid,NPT_String objectId){
    NanAsyncQueueWorker(new BrowseAction(callback, uuid, objectId,this));
}

void 
MediaController::GetTracks(NanCallback *callback, NPT_String uuid,NPT_String objectId){
    NanAsyncQueueWorker(new GetTracksAction(callback, uuid, objectId,this));
}

NPT_Result 
MediaController::SetRenderer(NPT_String uuid)
{
    PLT_DeviceDataReference* result;
    NPT_AutoLock lock(m_CurMediaRendererLock);
    NPT_AutoLock MRLock(m_MediaRenderers);
    m_MediaRenderers.Get(uuid, result);
    if(!result)
        return NPT_FAILURE;
    else{
        m_CurMediaRenderer = *result;
        return NPT_SUCCESS;
    }
}

NPT_Result
MediaController::OpenTrack(NPT_Array<PLT_MediaItemResource> &Resources,NPT_String& Didl,Action* action){
    NPT_Result res = NPT_FAILURE;
    PLT_DeviceDataReference device;

    GetCurMR(device);
    if(!device.IsNull()){
        if(Resources.GetItemCount() > 0) {
            // look for best resource to use by matching each resource to a sink advertised by renderer
            NPT_Cardinal resource_index;
            PLT_MediaItem temp;
            temp.m_Resources = Resources;//FindBestResource wants a PLT_MediaItem, so make a temp one and give it our resource array
            if (NPT_FAILED(FindBestResource(device, temp, resource_index))) {
                return NPT_ERROR_NOT_SUPPORTED;
            }
            // invoke the setUri
            return SetAVTransportURI(device, 0, Resources[resource_index].m_Uri, Didl,action);
        } else {
            NPT_LOG_SEVERE("couldn't find resource");
            return NPT_ERROR_NO_SUCH_ITEM;
        }
    }
    return res;
}

NPT_Result
MediaController::OpenNextTrack(NPT_Array<PLT_MediaItemResource> &Resources,NPT_String& Didl,Action* action){
    NPT_Result res = NPT_FAILURE;
    PLT_DeviceDataReference device;

    GetCurMR(device);
    if(!device.IsNull()){
        if(Resources.GetItemCount() > 0) {
            // look for best resource to use by matching each resource to a sink advertised by renderer
            NPT_Cardinal resource_index;
            PLT_MediaItem temp;
            temp.m_Resources = Resources;//FindBestResource wants a PLT_MediaItem, so make a temp one and give it our resource array
            if (NPT_FAILED(FindBestResource(device, temp, resource_index))) {
                return NPT_ERROR_NOT_SUPPORTED;
            }
            // invoke the setUri
            return SetAVTransportURI(device, 0, Resources[resource_index].m_Uri, Didl,action);
        } else {
            NPT_LOG_SEVERE("couldn't find resource");
            return NPT_ERROR_NO_SUCH_ITEM;
        }
    }
    return res;
}

NPT_Result
MediaController::GetTrackPosition(Action* action){
    NPT_AutoLock lock(m_CurMediaRendererLock);
    if (!m_CurMediaRenderer.IsNull())
        return GetPositionInfo(m_CurMediaRenderer, 0, action);
    else
        return NPT_ERROR_NO_SUCH_ITEM;
}

void
MediaController::GetCurMR(PLT_DeviceDataReference& renderer){
    NPT_AutoLock lock(m_CurMediaRendererLock);
    renderer = m_CurMediaRenderer;
}


PLT_DeviceDataReference
MediaController::getDeviceReference(NPT_String UUID){

    NPT_AutoLock             lock(m_MediaServers);
    PLT_DeviceDataReference* result = NULL;
    m_MediaServers.Get(UUID, result);
    return result?*result:PLT_DeviceDataReference();
}

void
MediaController::FlushQueue(ObjectWrap *context){
    while (!queue.empty()) {
        Action* event = queue.front();
        event->EmitAction(context);
        queue.pop();
        delete event;
    }
}