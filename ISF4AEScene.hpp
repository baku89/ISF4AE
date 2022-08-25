#pragma once

#include <VVISF.hpp>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>

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

  void useCode(const std::string code) {
    auto doc = VVISF::CreateISFDocRefWith(code);
    useDoc(doc);

    // Check if there's a redifinition of inputs with same name.
    // this should precede the shader compilation since GLSL also raises redifinition error.
    std::unordered_set<std::string> inputNames;
    for (auto& input : inputs()) {
      std::string name = input->name();
      if (inputNames.find(name) != inputNames.end()) {
        std::map<std::string, std::string> errDict;

        std::stringstream ss;
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
      std::map<std::string, std::string> errDict;

      errDict["ia4ErrLog"] = R"(The type of uniform i4a_Downsample has to be "point2D")";

      auto err = ISFErr(ISFErrType_ErrorCompilingGLSL, "Invalid uniform", "", errDict);
      throw err;
    }
  }

  std::map<std::string, std::string> errDict() { return _errDict; }

  bool isTimeDependant() {
    std::string& fs = *doc()->fragShaderSource();

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
    this->setRenderPrepCallback([](const VVGL::GLScene& n, const bool inReshaped, const bool inPgmChanged) {
      // Prevent a result to be multiplied by alpha.
      glDisable(GL_BLEND);
    });
  }
};

using ISF4AESceneRef = std::shared_ptr<ISF4AEScene>;

inline ISF4AESceneRef CreateISF4AESceneRef() {
  return std::make_shared<ISF4AEScene>();
}

}  // namespace VVISF
