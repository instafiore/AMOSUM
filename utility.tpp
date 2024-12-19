#include "utility.h"


template <typename V>
PerfectHash<V>::PerfectHash(int N) : N(N) {
    // it is a (N * 2) vector where:
    // values[:N-1]    are the values for the positive literals
    // values[N:]      are the values for the negative literals
    values.resize(N * 2, NULL);
}

template <typename V>
V& PerfectHash<V>::operator[](int lit) {
    // Determine the index for positive or negative literals
    int i = (lit > 0) ? lit : (abs(lit) + N);
    return values[i];
}

template <typename V>
void PerfectHash<V>::set(const int& key, const V& value) {
    // Determine the index for positive or negative literals
    int i = (key > 0) ? key : (abs(key) + N);
    values[i] = value;
}

template <typename T>
std::string vector_to_string(const std::vector<T>& vec){
    std::ostringstream oss;
    oss<<"[";
    for (size_t i = 0; i < vec.size()-1; i++)
    {
        oss<<"'"<<vec[i]<<"'"<<"," ;
    }
    if (vec.size() > 0) oss<<"'"<<vec[vec.size()-1]<<"'";

    oss<<"]";
    return oss.str();
}

template <typename Key, typename Value>
std::string unordered_map_to_string(std::unordered_map<Key, Value> map){

    std::ostringstream oss;
    oss<< "{" ;
    for (const auto& [key, value] : map) {
        oss <<"'"<< key << "':'" << value<<"', " ;
    }
    oss << "}" ;
    return oss.str() ;
}