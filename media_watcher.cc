/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND. See the accompanying 
 * LICENSE file.
 */

#include <v8.h>
#include <node.h>
#include "MediaFinder.h"
#include "Neptune.h"
#include <unistd.h>

using namespace node;
using namespace v8;


#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

#define V8_SET(OBJ, NAME, STR)																					\
	OBJ->Set(String::New(NAME),String::New(STR,STR.GetLength()));					\


class Stop;
class Play;
class Open;
class OpenNext;
class GetTracks;
class GetMediaInfo;
class WatchEvents;
class Pause;
class BrowseDirectory;
class GetMediaPosition;
class SetPosition;

template <class Wrap>
static Handle<Value> Action(const Arguments& args){
    Wrap* wrap = new Wrap();
    wrap->Start(args);
    return Undefined();
}


class MediaWatcher: ObjectWrap
{
private:
  Media_Finder*	controller;
  
public:
	controllerInfo* watchInfo;
  static Persistent<ObjectTemplate> track_tmpl;
  static Persistent<FunctionTemplate> s_ct;
  static void Init(Handle<Object> target)
  {
    HandleScope scope;
    //Handle<Value>
    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    Local<ObjectTemplate> o = ObjectTemplate::New();

    track_tmpl = Persistent<ObjectTemplate>::New(o);
    track_tmpl->SetInternalFieldCount(2);
    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("MediaWatcher"));

    NODE_SET_PROTOTYPE_METHOD(s_ct, "startUpnp", StartUpnp);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "pollEvent", PollEvent);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "getServer", GetServer);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "getRenderers", GetRenderers);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "setRenderer", SetRenderer);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "watchEvents", Action<WatchEvents>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "getTracks", Action<GetTracks>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "doBrowse", Action<BrowseDirectory>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "getMediaInfo", Action<GetMediaInfo>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "seek", Action<SetPosition>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "getPosition", Action<GetMediaPosition>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "stop", Action<Stop>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "pause", Action<Pause>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "play", Action<Play>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "open", Action<Open>);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "openNext", Action<OpenNext>);

    target->Set(String::NewSymbol("MediaWatcher"),
                s_ct->GetFunction());
  }

  MediaWatcher()
  {


	watchInfo = new controllerInfo;
  }

  ~MediaWatcher()
  {
  }

  static Handle<Value> New(const Arguments& args)
  {
    HandleScope scope;
    MediaWatcher* mw = new MediaWatcher();
    mw->Wrap(args.This());
    return args.This();
  }

  struct watch_baton_t {
    uv_work_t request;
    MediaWatcher *mw;
    NPT_String UUID;
    Persistent<Function> cb;
    PLT_MediaObjectListReference tracks;
    NPT_Result res;
  };

  static Handle<Value> StartUpnp(const Arguments& args)
  {
    HandleScope scope;
    REQ_FUN_ARG(0, cb);
    MediaWatcher* mw = ObjectWrap::Unwrap<MediaWatcher>(args.This());
    watch_baton_t *baton = new watch_baton_t();
    baton->mw = mw;
    baton->cb = Persistent<Function>::New(cb);
    mw->Ref();
    baton->request.data = baton;
    //eio_custom(EIO_Watch, EIO_PRI_DEFAULT, EIO_AfterWatch, baton);
    //ev_ref(EV_DEFAULT_UC);
     uv_queue_work(uv_default_loop(), &baton->request,EIO_Watch, (uv_after_work_cb) EIO_AfterWatch);

    return Undefined();
  }

  static void EIO_Watch(uv_work_t* req)
  {
   watch_baton_t *baton = static_cast<watch_baton_t *>(req->data);
   //NPT_LogManager::GetDefault().Configure("plist:.level=INFO;.handlers=ConsoleHandler;.ConsoleHandler.colors=off;.ConsoleHandler.filter=63");
   //create upnp engine
    PLT_UPnP upnp;
    //create control point
    PLT_CtrlPointReference ctrlPoint(new PLT_CtrlPoint());
    //create the controller, and give it our watch structure
    baton->mw->controller = new Media_Finder(ctrlPoint, baton->mw->watchInfo);
    upnp.AddCtrlPoint(ctrlPoint);
    upnp.Start();

    //loop forever
    while(1){
      sleep(10);
    };

    //return 0;

  }


  static Handle<Value> GetRenderers(const Arguments& args){
    HandleScope scope;
    int i = 0;
    MediaWatcher* mw = ObjectWrap::Unwrap<MediaWatcher>(args.This());
    PLT_DeviceMap devices = mw->controller->GetMRs();
    
    const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
    Local<Array> res = Array::New(entries.GetItemCount());
    NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
    while (entry) {
	Local<Object> tempDevice = Object::New();
        PLT_DeviceDataReference device = (*entry)->GetValue();
        NPT_String              name   = device->GetFriendlyName();
        tempDevice->Set(String::New("uuid"),String::New((char*) (*entry)->GetKey()));
	tempDevice->Set(String::New("name"),String::New((char*)name));
	res->Set(Integer::New(i),tempDevice);
 	i++;
        ++entry;
    }
    return scope.Close(res);
  }

  static Handle<Value> SetRenderer(const Arguments& args){
    HandleScope scope;
    MediaWatcher* mw = ObjectWrap::Unwrap<MediaWatcher>(args.This());
    NPT_String UUID = *String::AsciiValue(args[0]);
    if(NPT_SUCCEEDED(mw->controller->SetMR(UUID)))
	return True();
    else
	return False();
}

  static Handle<Value> PollEvent(const Arguments& args){
    HandleScope scope;
	  NPT_LOG_INFO("in poll");
    MediaWatcher* mw = ObjectWrap::Unwrap<MediaWatcher>(args.This());
    mw->watchInfo->m_EventStack.Lock();
    EventInfo ev;

    if(NPT_SUCCEEDED(mw->watchInfo->m_EventStack.Pop(ev))){
			mw->watchInfo->m_EventStack.Unlock();
			Local<Object> result = Object::New();
			NPT_LOG_INFO("HERE");
			NPT_LOG_INFO(ev.Name);
      if(ev.Name.Compare("serverAdded") == 0){
        ServerInfo* server = (ServerInfo*) ev.userData;
        V8_SET(result,"iconUrl",server->iconUrl);
        V8_SET(result,"baseUrl",server->baseUrl);
        V8_SET(result,"nameTest",ev.Name);
        delete server;
      }
		  V8_SET(result,"name",ev.Name);
			V8_SET(result,"value",ev.Value);
			V8_SET(result,"uuid",ev.UUID);
      result->Set(String::New("sourceType"),(ev.SourceType == RENDERER) ? String::New("renderer") : String::New("server"));
			return scope.Close(result);
    }
    else{
			mw->watchInfo->m_EventStack.Unlock();
			return False();//no more events
    }
   return False();
  }


  static Handle<Value> GetServer(const Arguments& args){
    HandleScope scope;
    MediaWatcher* mw = ObjectWrap::Unwrap<MediaWatcher>(args.This());
    mw->watchInfo->m_DeviceStack.Lock();
    NPT_String device;
    if(NPT_SUCCEEDED(mw->watchInfo->m_DeviceStack.Pop(device))){
    	mw->watchInfo->m_DeviceStack.Unlock();
    	Handle<String> UUID = String::New((char*)device,device.GetLength());
    	return scope.Close(UUID);
    }
    else{
	    mw->watchInfo->m_DeviceStack.Unlock();
	    return False();//no more devices
    }
  }



  static void EIO_AfterWatch(uv_work_t* req)
  {
    HandleScope scope;
    watch_baton_t *baton = static_cast<watch_baton_t *>(req->data);
    //ev_unref(EV_DEFAULT_UC);
    baton->mw->Unref();

    Local<Value> argv[1];

    argv[0] = String::New("Hello World");

    TryCatch try_catch;

    baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    baton->cb.Dispose();

    delete baton;

  }
  friend class QueryWrap;


};
Persistent<ObjectTemplate> MediaWatcher::track_tmpl;
Persistent<FunctionTemplate> MediaWatcher::s_ct;

class QueryWrap{
public:
	uv_work_t request;
	MediaWatcher *mw;
	Media_Finder*	controller;
	Persistent<Function> cb;
  NPT_Result res;
	void Start(const Arguments& args){
		HandleScope scope;
		if (args.Length() <= 0 || !args[0]->IsFunction())	
			NPT_LOG_SEVERE("Argument 0 must be a function");
		else{
			Local<Function> callback = Local<Function>::Cast(args[0]);
			mw = ObjectWrap::Unwrap<MediaWatcher>(args.This());
			cb = Persistent<Function>::New(callback);
			controller = mw->controller;
			mw->Ref();
			QueryWrap* self = this;
			request.data = self;
			SetupBaton(args);
			//eio_custom(QueryWrap::DoTask, EIO_PRI_DEFAULT,QueryWrap::OnFinish, self);
			//ev_ref(EV_DEFAULT_UC);
			uv_queue_work(uv_default_loop(), &(request),QueryWrap::DoTask, (uv_after_work_cb) QueryWrap::OnFinish);

		}
	}

private:
	virtual void SetupBaton(const Arguments& args){};
	virtual void ThreadTask(){};
	virtual void After(){};
	
	static void DoTask(uv_work_t* req) {
		QueryWrap *self = static_cast<QueryWrap *>(req->data);
		self->ThreadTask();
	}
	static void OnFinish(uv_work_t* req){
		HandleScope scope;
		QueryWrap *self = static_cast<QueryWrap *>(req->data);
		//ev_unref(EV_DEFAULT_UC);
		self->mw->Unref();

		self->After();

		self->cb.Dispose();
		delete self;
	}

protected:
  void CallOnComplete(Local<Value> data) {
    HandleScope scope;
    Local<Value> argv[1] = {data};
    TryCatch try_catch;
		cb->Call(Context::GetCurrent()->Global(), 1, argv);
		if (try_catch.HasCaught()) {
			FatalException(try_catch);
		}
  }
  void CallOnComplete(Local<Value> arg1,Local<Value> arg2) {
    HandleScope scope;
    Local<Value> argv[2] = {arg1,arg2};

    TryCatch try_catch;
    cb->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }
  }

};

class WatchEvents : public QueryWrap{
private:
	void ThreadTask(){
   mw->watchInfo->hasChanged.WaitUntilEquals(1);
   mw->watchInfo->hasChanged.SetValue(0);
	}
	void After(){
		NPT_LOG_INFO("Event triggered");
		CallOnComplete(String::New("EVENT"));
	}
};


class Stop : public QueryWrap{
private:
	void ThreadTask(){
    NPT_LOG_INFO("START STOP");
		PLT_BrowseData playInfo;
		playInfo.shared_var.SetValue(0);
		if(NPT_SUCCEEDED(controller->StopTrack(&playInfo))){
      playInfo.shared_var.WaitUntilEquals(1);
      res = playInfo.res;
    }else{
      res = NPT_FAILURE;
    }
	}
	void After(){ 
		NPT_LOG_INFO("After stop");
		if(NPT_SUCCEEDED(res))
			CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
		else
		  CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
	}
};

class SetVolume : public QueryWrap{
private:
	int value;
	void SetupBaton(const Arguments& args){
		if (args.Length() <= 1 || !args[1]->IsInt32())	
			NPT_LOG_SEVERE("Argument 1 must be an Integer ");
		else{
			value = args[1]->IntegerValue();
		}
	}
	void ThreadTask(){
		PLT_BrowseData playInfo;
		playInfo.shared_var.SetValue(0);
		if(NPT_SUCCEEDED(controller->Volume(value, &playInfo))){
  		playInfo.shared_var.WaitUntilEquals(1);
  		res = playInfo.res;
    }else{
      res = NPT_FAILURE;
    }

	}
	void After(){ 
		if(NPT_SUCCEEDED(res))
			CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
		else
		  CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
	}
};

class SetMute : public QueryWrap{
private:
	bool value;
	void SetupBaton(const Arguments& args){
		if (args.Length() <= 1 || !args[1]->IsBoolean())	
			NPT_LOG_SEVERE("Argument 1 must be a bool");
		else{
			value = args[1]->BooleanValue();
		}
	}
	void ThreadTask(){
		PLT_BrowseData playInfo;
		playInfo.shared_var.SetValue(0);
		if(NPT_SUCCEEDED(controller->Mute(value, &playInfo))){
  		playInfo.shared_var.WaitUntilEquals(1);
  		res = playInfo.res;
    }else{
      res = NPT_FAILURE;
    }
	}
	void After(){ 
		if(NPT_SUCCEEDED(res))
			CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
		else
		  CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
	}
};

class Pause : public QueryWrap{
private:
	void ThreadTask(){
		PLT_BrowseData playInfo;
		playInfo.shared_var.SetValue(0);
		if(NPT_SUCCEEDED(controller->PauseTrack(&playInfo))){
      playInfo.shared_var.WaitUntilEquals(1);
      res = playInfo.res;
    }else{
      res = NPT_FAILURE;
    }
	}
	void After(){ 
		if(NPT_SUCCEEDED(res))
			CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
		else
		  CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
	}
};

class Play : public QueryWrap{
private:

	void ThreadTask(){
		PLT_BrowseData playInfo;
		playInfo.shared_var.SetValue(0);
		if(NPT_SUCCEEDED(controller->PlayTrack(&playInfo))){
      playInfo.shared_var.WaitUntilEquals(1);
      res = playInfo.res;
    }else{
      res = NPT_FAILURE;
    }
	}
	void After(){ 
		NPT_LOG_INFO("After play");
		if(NPT_SUCCEEDED(res))
			CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
		else
		  CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
	}
};
/*
class Pause : public QueryWrap{
private:

	void ThreadTask(){
		PLT_BrowseData playInfo;
		playInfo.shared_var.SetValue(0);
		controller->PauseTrack(&playInfo);
		playInfo.shared_var.WaitUntilEquals(1);
		res = playInfo.res;
	}
	void After(){ 
		NPT_LOG_INFO("After Pause");
		if(NPT_SUCCEEDED(res))
			CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
		else
		  CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
	}
};
*/
class Open : public QueryWrap{
private:
	NPT_Array<PLT_MediaItemResource> *resource;
	NPT_String* Didl;
	void SetupBaton(const Arguments& args){
		NPT_LOG_INFO("start open");
		Local<Object> track = args[1]->ToObject();
		Local<Array> resArray = Local<Array>::Cast(track->Get(String::New("Resources")));
		resource = new NPT_Array<PLT_MediaItemResource>(resArray->Length());
		for(unsigned int i=0; i < resArray->Length(); i++){
			PLT_MediaItemResource* curRes = new PLT_MediaItemResource();
			curRes->m_Uri = *String::AsciiValue(resArray->Get(i)->ToObject()->Get(String::New("Uri")));
			curRes->m_ProtocolInfo = *String::AsciiValue(resArray->Get(i)->ToObject()->Get(String::New("ProtocolInfo")));
			resource->Add(*curRes);
		}
		Didl = new NPT_String(*String::AsciiValue(track->Get(String::New("Didl"))));
    NPT_String didl2 = *String::AsciiValue(track->Get(String::New("Didl")));
		NPT_LOG_INFO_1("Didl = %s",(char*)(*Didl));
		NPT_LOG_INFO("all set");
	}
	void ThreadTask(){ 
		PLT_BrowseData playInfo;
	  playInfo.shared_var.SetValue(0);
		if(NPT_SUCCEEDED(controller->OpenTrack(*resource, *Didl, &playInfo))){
			playInfo.shared_var.WaitUntilEquals(1);
      res = playInfo.res;
    }else{
			res = NPT_FAILURE;
	  }
	}
	void After(){
		NPT_LOG_INFO("After opem");
		if(NPT_SUCCEEDED(res))
			CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
		else
			CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
		
	}
};


class GetMediaInfo : public QueryWrap{
public:
  PLT_MediaInfo info;
private:
  void ThreadTask(){
    Info_data playInfo;
    playInfo.shared_var.SetValue(0);
    controller->GetTrackInfo(&playInfo);
    playInfo.shared_var.WaitUntilEquals(1,30000);
    res = playInfo.res;
    info = playInfo.info;
  }
  void After(){ 
    NPT_LOG_INFO("After GetMediaInfo");
    if(NPT_SUCCEEDED(res)){
      Local<Object> resObj = Object::New();
      V8_SET(resObj,"uri",info.cur_uri);
      CallOnComplete(resObj);
    }else{
      CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
    }
  }
};

class GetMediaPosition : public QueryWrap{
public:
  PLT_PositionInfo info;
private:
  void ThreadTask(){
    Position_data playInfo;
    playInfo.shared_var.SetValue(0);
    controller->GetPosition(&playInfo);
    playInfo.shared_var.WaitUntilEquals(1,30000);
    res = playInfo.res;
    info = playInfo.info;
  }
  void After(){ 
    NPT_LOG_INFO("After get position");
    if(NPT_SUCCEEDED(res)){
      Local<Object> resObj = Object::New();
      resObj->Set(String::New("position"), Number::New(info.rel_time.ToMillis()));
      resObj->Set(String::New("duration"), Number::New(info.track_duration.ToMillis()));
      CallOnComplete(resObj);
    }else{
      CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
    }
  }
};


class GetTracks : public QueryWrap{
private:
	NPT_String UUID;
  NPT_String dirID;
	PLT_MediaObjectListReference tracks;

  //return an array of objects that contain a uri, and a protocol string
	Handle<Array> wrapResources(NPT_Array<PLT_MediaItemResource>* m_Resources){
		HandleScope scope;
		Local<Array> resArray = Array::New((int)m_Resources->GetItemCount());
    for (NPT_Cardinal u=0; u<m_Resources->GetItemCount(); u++) {
			Local<Object> resObj = Object::New();
			NPT_String protocol = (*m_Resources)[u].m_ProtocolInfo.ToString();
			resObj->Set(String::New("ProtocolInfo"),String::New(protocol,protocol.GetLength()));
			resObj->Set(String::New("Uri"),String::New((*m_Resources)[u].m_Uri,(*m_Resources)[u].m_Uri.GetLength()));
		  resArray->Set(Integer::New(u),resObj);
		}
		return scope.Close(resArray);
	}


	void SetupBaton(const Arguments& args){
		NPT_LOG_INFO("start get tracks");
    UUID = *String::AsciiValue(args[1]);
    dirID = *String::AsciiValue(args[2]);
	}
	void ThreadTask(){ 
   NPT_LOG_INFO("fire get tracks ");
   res = controller->DoSearch(UUID,dirID,tracks);
	}
	void After(){
		HandleScope scopeTest;
		NPT_LOG_INFO("AFTER");
	  NPT_LOG_INFO_1("Res = %d ",res);
    if(NPT_SUCCEEDED(res)){
      Local<Array> trackArray = Array::New((int)tracks->GetItemCount());
      int i =0;

			NPT_List<PLT_MediaObject*>::Iterator item = tracks->GetFirstItem();
			NPT_List<PLT_MediaObject*>::Iterator lastItem;
			while (item) {
				Local<Object> temp = Object::New();
		 		temp->Set(String::New("Resources"),wrapResources(&(*item)->m_Resources));
			  V8_SET(temp,"Didl",(*item)->m_Didl);
				V8_SET(temp,"Title",(*item)->m_Title);
        NPT_List<PLT_PersonRole>::Iterator person = (*item)->m_People.artists.GetFirstItem();
        if(person){
				  V8_SET(temp,"Artist",(*item)->m_People.artists.GetFirstItem()->name);
        }else{
          temp->Set(String::New("Artist"),String::New("unknown"));
        }
				V8_SET(temp,"Album",(*item)->m_Affiliation.album);
				temp->Set(String::New("TrackNumber"),Integer::New((*item)->m_MiscInfo.original_track_number));
				
        //temp->Set(String::New("TrackNumber"),Str::New((*item)->m_ObjectID));
				V8_SET(temp,"oID",(*item)->m_ObjectID);

		 		//add a copy of the temp object into our array
		 		trackArray->Set(i,temp->Clone());
		 
		 		lastItem = item;
		    ++item;
		    //delete the previous item
		 		tracks->Erase(lastItem);
			  i++;
    	}
			CallOnComplete(trackArray);
    }else{
			NPT_LOG_INFO("fail");
			CallOnComplete(String::New("fail"));
			//argv[0] = String::New("fail");		
		}
	}
};


class SetPosition : public QueryWrap{
private:
  NPT_String target;

  void SetupBaton(const Arguments& args){
    target = *String::AsciiValue(args[1]);
  }
  void ThreadTask(){
    PLT_BrowseData playInfo;
    playInfo.shared_var.SetValue(0);
    if(NPT_SUCCEEDED(controller->SetPosition(&playInfo,target))){
      playInfo.shared_var.WaitUntilEquals(1,30000);
      res = playInfo.res;
    }else{
      res = NPT_FAILURE;
    }
  }
  void After(){ 
    NPT_LOG_INFO("After set position");
    if(NPT_SUCCEEDED(res))
      CallOnComplete(Local<Boolean>::New(Boolean::New(true)));
    else
      CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
  }
};


class BrowseDirectory : public QueryWrap{
private:
  PLT_MediaObjectListReference browseResults;
  NPT_String UUID;
  NPT_String dirID;


  void SetupBaton(const Arguments& args){
    UUID = *String::AsciiValue(args[1]);
    dirID = *String::AsciiValue(args[2]);
  }
  void ThreadTask(){ 
   res = controller->DoBrowse(UUID,dirID,browseResults);
  }
  void After(){
    HandleScope scopeTest;
    if(NPT_SUCCEEDED(res)){
      Local<Array> dirArray = Array::New();

      if(!browseResults.IsNull()){
        NPT_List<PLT_MediaObject*>::Iterator item = browseResults->GetFirstItem();
        int i =0;
        while (item) {
           Local<Object> temp = Object::New();
            V8_SET(temp,"_id",(*item)->m_ObjectID);
            V8_SET(temp,"Title",(*item)->m_Title);
            temp->Set(String::New("isContainer"),Boolean::New((*item)->IsContainer()));
            dirArray->Set(i++,temp);
            ++item;
        }
      }
      CallOnComplete(dirArray,String::New(UUID));
    }else{
      NPT_LOG_INFO("fail");
      CallOnComplete(Local<Boolean>::New(Boolean::New(false)));
    }
  }
};

class OpenNext : public QueryWrap{
private:
	NPT_Array<PLT_MediaItemResource> *resource;
	NPT_String* Didl;
	void SetupBaton(const Arguments& args){
		Local<Object> track = args[1]->ToObject();
		resource = (NPT_Array<PLT_MediaItemResource>*) track->GetPointerFromInternalField(0);
		Didl = (NPT_String*) track->GetPointerFromInternalField(1);
	}

	void ThreadTask(){ 
		//Open* newSelf = (Open*) this;
		PLT_BrowseData playInfo;
	  playInfo.shared_var.SetValue(0);
		if(NPT_SUCCEEDED(controller->OpenNextTrack(*resource, *Didl, &playInfo))){
			playInfo.shared_var.WaitUntilEquals(1);
      res = playInfo.res;
    }else{
			res = NPT_FAILURE;
	  }
	}
	void After(){
	 NPT_LOG_INFO("After opem mext");
		if(NPT_SUCCEEDED(res))
			CallOnComplete(String::New("Open Next Succeeded"));
		else
			CallOnComplete(String::New("Open Next Failed"));
	}
};

extern "C" {
  static void init (Handle<Object> target)
  {
    MediaWatcher::Init(target);
  }

  NODE_MODULE(media_watcher, init);
}
