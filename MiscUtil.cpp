#include "MiscUtil.h"

std::string joinWith(std::vector<std::string> texts, std::string delimiter) {
   
   std::stringstream ss;
   
   for (int i = 0; i < texts.size(); i++) {
           
       ss << texts[i];
       
       if (i != texts.size() - 1) {
           ss << delimiter;
       }
   }
   
   return ss.str();
}

void setBitFlag(int flag, bool value, int *target) {
    if (value) {
        *target |= flag;
    } else {
        *target &= ~flag;
    }
}
