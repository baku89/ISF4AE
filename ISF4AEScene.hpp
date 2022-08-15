#pragma once

#include <VVISF/VVISF.hpp>

using namespace VVGL;

namespace VVISF {

/**
 * A sub-class of ISFScene with some render setting hooks for AE and error logging methods.
 */
class ISF4AEScene : public ISFScene {
public:
    
    ISF4AEScene(): ISFScene() {
        _setUpRenderPrepCallback();
    }
    
    ISF4AEScene(const GLContextRef & inCtx): ISFScene(inCtx) {
        _setUpRenderPrepCallback();
    }
    
private:
    
    void _setUpRenderPrepCallback() {
        this->setRenderPrepCallback([](const VVGL::GLScene & n, const bool inReshaped, const bool inPgmChanged) {
            // Prevent a result to be multiplied by alpha.
            glDisable(GL_BLEND);
        });
    }
    
};

using ISF4AESceneRef = std::shared_ptr<ISF4AEScene>;

inline ISF4AESceneRef CreateISF4AESceneRef() { return std::make_shared<ISF4AEScene>(); }




}
