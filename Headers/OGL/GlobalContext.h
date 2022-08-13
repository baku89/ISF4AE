#pragma once

#include <VVGL/VVGL.hpp>

namespace OGL {

class GlobalContext {
   public:
    bool initialized = false;
    GlobalContext();
    ~GlobalContext();
    void bind();
    
    VVGL::GLCPUToTexCopierRef uploader;
    VVGL::GLTexToCPUCopierRef downloader;

   private:
    VVGL::GLContextRef sharedContext;
};

}  // namespace OGL
