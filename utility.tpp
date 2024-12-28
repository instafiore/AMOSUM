#pragma once
#include "utility.h"
#include "settings.h"

template <typename V>
V PerfectHash<V>::get(int lit) {
    // Determine the index for positive or negative literals
    int i = (lit > 0) ? lit : (abs(lit) + N);
    return values[i];
}

template <typename V>
void PerfectHash<V>::set(const int& key, const V& value) {
    // Determine the index for positive or negative literals
    int i = (key > 0) ? key : (abs(key) + N);
    if(i < 0 or i >= values.size()){
        debug("overflow: ",i, " key: ",key);
        return ; 
    }
    values[i] = value;
}

template <typename T>
std::string vector_to_string(const std::vector<T>& vec){
    std::ostringstream oss;
    int n = vec.size() ;
    oss<<"[";
    for (int i = 0; i < n-1; i++)
    {
        oss<<"'"<<vec[i]<<"'"<<"," ;
    }
    if (n > 0) oss<<"'"<<vec[n-1]<<"'";

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



template <typename K, typename V>
void update_map_value_vector(std::unordered_map<K, std::vector<V>>& umap, K key, V value) {
    // Ensure the key exists and has an empty vector if not present
    if (umap.find(key) == umap.end()) {
        umap[key] = std::vector<V>();
    }

    // Append the values
    umap[key].push_back(value);

}

template <typename K, typename V>
std::vector<V> get_map_value_vector(std::unordered_map<K, std::vector<V>>& umap, K key) {
   
    if (umap.find(key) == umap.end()) {
        return std::vector<V>();
    }

    return umap[key];

}

template <typename K, typename V>
V get_map(std::unordered_map<K, V>& umap, K key, V default_value) {   
    if (umap.find(key) == umap.end()) return default_value;
    return umap[key];
}

template< typename T>
void extend_vector(std::vector<T>& to_extend, const std::vector<T>& input, size_t i = 0, int j = -1){
    if (j==-1) j=input.size();

    for (; i < j; i++)    to_extend.push_back(input[i]);
}


template <typename T>
class SymmetricFunction {
private:
    std::vector<T> data_structure;

protected:
    T NONE  ;   
    virtual T function_negative_lit(T value) const  = 0;  

public:

    // Constructor
    explicit SymmetricFunction(size_t N, T NONE = SETTINGS::NONE) : 
        data_structure(N,NONE), NONE(NONE) {}

    
    // Getter (accessor)
    virtual T get(int lit) const {
        int i = std::abs(lit);
        auto value = data_structure[i];
        if (lit < 0) value = function_negative_lit(value);
        return value;
    }

    // Setter (mutator)
    void set(int lit, T value) {
        int i = std::abs(lit);
        if (lit < 0) value = function_negative_lit(value);
        data_structure[i] = value;
    }
};

class WeightFunction: public SymmetricFunction<int>{

    int function_negative_lit(int value) const override { 
        return value ;
    }

public:
    int get(int lit) const override{
        if (lit == NONE) return 0 ;
        return this->SymmetricFunction::get(lit) ;
    }
    WeightFunction(int N) : SymmetricFunction(N) {}
};

class InterpretationFunction: public SymmetricFunction<int>{


    int function_negative_lit(int value) const override { 
        if (value == NONE)  return value ;
        return 1 - value ;
    }
public:
    InterpretationFunction(int N) : SymmetricFunction(N) {}
};

class AggregateFunction: public PerfectHash<bool>{
public:
    AggregateFunction(size_t N): PerfectHash(N, false){}
};

class GroupFunction: public PerfectHash<Group*>{
public:
    GroupFunction(size_t N): PerfectHash(N, nullptr){}
};