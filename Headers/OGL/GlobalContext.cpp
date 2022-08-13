#include "GlobalContext.h"
#include "Debug.h"

using namespace VVGL;

namespace OGL {

GlobalContext::GlobalContext() {
    
    this->sharedContext = CreateNewGLContextRef(NULL, CreateCompatibilityGLPixelFormat());
    
    CreateGlobalBufferPool(this->sharedContext);
    
    this->downloader = CreateGLTexToCPUCopierRefUsing(this->sharedContext->newContextSharingMe());
    this->uploader = CreateGLCPUToTexCopierRefUsing(this->sharedContext->newContextSharingMe());
    
    this->initialized = true;
}

void GlobalContext::bind() {
    this->sharedContext->makeCurrent();
}

GlobalContext::~GlobalContext() {
}

}  // namespace OGL
