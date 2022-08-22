#pragma once

#include <VVISF.hpp>
#include <string>
#include <map>
#include <regex>

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
    
    std::map<std::string,std::string> errDict() {
        return _errDict;
    }
    
    bool isTimeDependant() {
        std::string &fs = *doc()->fragShaderSource();
        
        // TODO: Regex below is quite makeshift and should be refactored.
        std::regex re("TIME|TIMEDELTA|FRAMEINDEX|DATE");
        
        return std::regex_search(fs, re);
    }
    
    std::string getFragCode() {
        auto doc = this->doc();
        
        return *doc->jsonSourceString() + *doc->fragShaderSource();
    }
    
protected:
    
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
