#pragma once

#include <VVGL/VVGL.hpp>

namespace OGL {

class GlobalContext {
   public:
    bool initialized = false;
    GlobalContext();
    ~GlobalContext();
    void bind();

   private:
    VVGL::GLContextRef sharedContext;
};

}  // namespace OGL
