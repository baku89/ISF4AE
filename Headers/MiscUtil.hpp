#pragma once

#include <vector>
#include <string>
#include <sstream>

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

template <class T>
int findIndex(std::vector<T> vals, T target) {
    // https://www-cns-s-u--tokyo-ac-jp.translate.goog/~masuoka/post/search_vector_index/?_x_tr_sl=ja&_x_tr_tl=en&_x_tr_hl=en&_x_tr_pto=sc
    auto itr = std::find(vals.begin(), vals.end(), target);

    if (itr == vals.end()) {
        return -1;
    } else {
        return std::distance(vals.begin(), itr);
    }
}

void setBitFlag(A_long flag, A_Boolean value, A_long *target) {
    if (value) {
        *target |= flag;
    } else {
        *target &= ~flag;
    }
}
