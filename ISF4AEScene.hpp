#pragma once

#include <VVISF.hpp>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace std;
using namespace VVGL;
using namespace VVISF;

namespace VVISF {

/**
 * A sub-class of ISFScene with some render setting hooks for AE and error logging methods.
 */
class ISF4AEScene : public ISFScene {
 public:
  ISF4AEScene() : ISFScene() { _setUpRenderPrepCallback(); }

  ISF4AEScene(const GLContextRef& inCtx) : ISFScene(inCtx) { _setUpRenderPrepCallback(); }

  void useCode(const string& fsCode, const string& vsCode) {
    ISFDocRef doc = nullptr;
    if (vsCode.empty()) {
      doc = CreateISFDocRefWith(fsCode);
    } else {
      doc = CreateISFDocRefWith(fsCode, "/", vsCode);
    }
    useDoc(doc);

    // Check if there's a redifinition of inputs with same name.
    // this should precede the shader compilation since GLSL also raises redifinition error.
    unordered_set<string> inputNames;
    for (auto& input : inputs()) {
      string name = input->name();
      if (inputNames.find(name) != inputNames.end()) {
        map<string, string> errDict;

        stringstream ss;
        ss << "Input redifinition: \"" << name << "\".";

        errDict["ia4ErrLog"] = ss.str();

        auto err = ISFErr(ISFErrType_ErrorLoading, "Invalid uniform", "", errDict);
        throw err;
      }

      inputNames.insert(name);
    }

    // Then complie
    compileProgramIfNecessary();

    // Throw GLSL errors
    if (_errDict.size() > 0) {
      auto err = ISFErr(ISFErrType_ErrorCompilingGLSL, "Shader Problem", "", _errDict);
      throw err;
    }

    // Check if types of i4a_* uniforms are correct
    ISFAttrRef attr;

    attr = inputNamed("i4a_Downsample");
    if (attr && attr->type() != ISFValType_Point2D) {
      map<string, string> errDict;

      errDict["ia4ErrLog"] = R"(The type of uniform i4a_Downsample has to be "point2D")";

      auto err = ISFErr(ISFErrType_ErrorCompilingGLSL, "Invalid uniform", "", errDict);
      throw err;
    }
  }

  map<string, string> errDict() { return _errDict; }

  bool isTimeDependant() {
    string& fs = *doc()->fragShaderSource();

    // TODO: Regex below is quite makeshift and should be refactored.
    regex re("TIME|TIMEDELTA|FRAMEINDEX|DATE");

    return regex_search(fs, re);
  }

  string getFragCode() {
    auto doc = this->doc();

    return *doc->jsonSourceString() + *doc->fragShaderSource();
  }

 protected:
  void _setUpRenderPrepCallback() {
    this->setRenderPrepCallback([](const VVGL::GLScene& n, const bool inReshaped, const bool inPgmChanged) {
      // Prevent a result to be multiplied by alpha.
      glDisable(GL_BLEND);
    });
  }
};

using ISF4AESceneRef = shared_ptr<ISF4AEScene>;

inline ISF4AESceneRef CreateISF4AESceneRefUsing(const VVGL::GLContextRef& inCtx) {
  return make_shared<ISF4AEScene>(inCtx);
}

}  // namespace VVISF
