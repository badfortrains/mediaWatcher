#include <uv.h>
#include "mediaController.h"

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
        item++;
    }
    uv_async_send(async);
}

void
MediaController::OnMSStateVariablesChanged(PLT_Service*  service, NPT_List<PLT_StateVariable*>*  vars){
    NPT_List<PLT_StateVariable*>::Iterator item = vars->GetFirstItem();
    while (item) {
        queue.push(new StateVariableAction(service,*item,SERVER));
        item++;
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

    NPT_AutoLock lock(m_MediaServers);
    m_MediaServers.Put(uuid, device);

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
MediaController::OnMSRemoved(PLT_DeviceDataReference& device)
{
    NPT_String uuid = device->GetUUID();
    {
        NPT_AutoLock lock(m_MediaServers);
        m_MediaServers.Erase(uuid);
    }

    queue.push(new DeviceAction("serverRemoved",device));
    uv_async_send(async);
}

void
MediaController::OnMRRemoved(PLT_DeviceDataReference& device)
{
    NPT_String uuid = device->GetUUID();

    {
        NPT_AutoLock lock(m_MediaRenderers);
        m_MediaRenderers.Erase(uuid);
    }

    {
        NPT_AutoLock lock(m_CurMediaRendererLock);
        // if it's the currently selected one, we have to get rid of it
        if (!m_CurMediaRenderer.IsNull() && m_CurMediaRenderer == device) {
            m_CurMediaRenderer = NULL;
        }
    }

    queue.push(new DeviceAction("rendererRemoved",device));
    uv_async_send(async);
}

PLT_DeviceMap
MediaController::GetMRs(){
    PLT_DeviceMap res;
    NPT_AutoLock MRLock(m_MediaRenderers);
    res  = m_MediaRenderers;
    return res;
}

PLT_DeviceMap
MediaController::GetMSs(){
    PLT_DeviceMap res;
    NPT_AutoLock MRLock(m_MediaServers);
    res  = m_MediaServers;
    return res;
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

class BrowseAction : public Nan::AsyncWorker {
public:
    BrowseAction(Nan::Callback *callback, NPT_String uuid,NPT_String objectId, MediaController* mc)
    : Nan::AsyncWorker(callback), uuid(uuid), objectId(objectId), mc(mc) {}
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
        if(NPT_SUCCEEDED(res)){
            Local<Array> dirArray = Nan::New<Array>();
            if(!resultList.IsNull()){
                NPT_List<PLT_MediaObject*>::Iterator item = resultList->GetFirstItem();
                int i =0;
                while (item) {
                    Local<Object> temp = Nan::New<Object>();
                    temp->Set(Nan::New<String>("_id").ToLocalChecked(),Nan::New<String>((*item)->m_ObjectID).ToLocalChecked());
                    temp->Set(Nan::New<String>("Title").ToLocalChecked(),Nan::New<String>((*item)->m_Title).ToLocalChecked());
                    temp->Set(Nan::New<String>("isContainer").ToLocalChecked(),Nan::New<Boolean>((*item)->IsContainer()));
                    dirArray->Set(i++,temp);
                    ++item;
                }
            }
            Local<Value> argv[] = {
                Nan::Null()
              , dirArray
            };
            callback->Call(2, argv);
        }else{
            Local<Value> argv[] = {
                Nan::Error(NPT_ResultText(res))
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
    GetTracksAction(Nan::Callback *callback, NPT_String uuid,NPT_String objectId, MediaController* mc)
    : BrowseAction(callback,uuid,objectId,mc) {}
    ~GetTracksAction() {}


    Handle<Array> wrapResources(NPT_Array<PLT_MediaItemResource>* m_Resources){
        Nan::EscapableHandleScope scope;
        Local<Array> resArray = Nan::New<Array>((int)m_Resources->GetItemCount());
        for (NPT_Cardinal u=0; u<m_Resources->GetItemCount(); u++) {
            Local<Object> resObj = Nan::New<Object>();
            NPT_String protocol = (*m_Resources)[u].m_ProtocolInfo.ToString();
            resObj->Set(Nan::New<String>("ProtocolInfo").ToLocalChecked(),Nan::New<String>(protocol).ToLocalChecked());
            resObj->Set(Nan::New<String>("Uri").ToLocalChecked(),Nan::New<String>((*m_Resources)[u].m_Uri).ToLocalChecked());
            resArray->Set(Nan::New<Integer>(u),resObj);
        }
        return scope.Escape(resArray);
    }
    void HandleOKCallback(){
        if(NPT_SUCCEEDED(res)){
            Local<Array> trackArray = Nan::New<Array>((int)resultList->GetItemCount());
            int i =0;
            NPT_List<PLT_MediaObject*>::Iterator item = resultList->GetFirstItem();
            while (item) {
                Local<Object> temp = Nan::New<Object>();
                if(!(*item)->IsContainer()){
                    temp->Set(Nan::New<String>("Resources").ToLocalChecked(),wrapResources(&(*item)->m_Resources));
                    temp->Set(Nan::New<String>("Didl").ToLocalChecked(),Nan::New<String>((*item)->m_Didl).ToLocalChecked());
                    temp->Set(Nan::New<String>("Title").ToLocalChecked(),Nan::New<String>((*item)->m_Title).ToLocalChecked());
                    temp->Set(Nan::New<String>("Album").ToLocalChecked(),Nan::New<String>((*item)->m_Affiliation.album).ToLocalChecked());
                    temp->Set(Nan::New<String>("TrackNumber").ToLocalChecked(),Nan::New<Integer>((*item)->m_MiscInfo.original_track_number));

                    NPT_List<PLT_PersonRole>::Iterator person = (*item)->m_People.artists.GetFirstItem();
                    if(person){
                        temp->Set(Nan::New<String>("Artist").ToLocalChecked(),Nan::New<String>((*item)->m_People.artists.GetFirstItem()->name).ToLocalChecked());
                    }else{
                        temp->Set(Nan::New<String>("Artist").ToLocalChecked(),Nan::New<String>("unknown").ToLocalChecked());
                    }
                }

                temp->Set(Nan::New<String>("isContainer").ToLocalChecked(),Nan::New<Boolean>((*item)->IsContainer()));
                temp->Set(Nan::New<String>("oID").ToLocalChecked(),Nan::New<String>((*item)->m_ObjectID).ToLocalChecked());
                trackArray->Set(i,temp);

                ++item;
                i++;
            }
            Local<Value> argv[] = {
                Nan::Null()
              , trackArray
            };
            callback->Call(2, argv);
        }else{
            Local<Value> argv[] = {
                Nan::Error(NPT_ResultText(res))
            };
            callback->Call(1, argv);
        }
    }

};

void
MediaController::BrowseDirectory(Nan::Callback *callback, NPT_String uuid,NPT_String objectId){
    Nan::AsyncQueueWorker(new BrowseAction(callback, uuid, objectId,this));
}

void
MediaController::GetTracks(Nan::Callback *callback, NPT_String uuid,NPT_String objectId){
    Nan::AsyncQueueWorker(new GetTracksAction(callback, uuid, objectId,this));
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
            SetNextAVTransportURI(device, 0, Resources[resource_index].m_Uri, Didl,action);
            //HACK, we don't have a delegate method for this result so we can't tell what happned
            //just return timeout.
            return NPT_ERROR_TIMEOUT;
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
