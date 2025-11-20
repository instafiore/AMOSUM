#pragma once
#include "utility.h"
#include <type_traits>
#include "settings.h"

template <typename V>
V PerfectHash<V>::get(int lit) const{
    // Determine the index for positive or negative literals
    int i = (lit > 0) ? lit : (abs(lit) + N);
    return values[i];
}


template <typename V>
V* get_perfect_hash_with_pointer(PerfectHash<V*>* ph, int lit){
    // Determine the index for positive or negative literals
    auto res = ph->get(lit);

    if(res == nullptr) {
        V* pointer = new V();
        ph->set(lit, pointer);
        return pointer ;
    }
    else return res ;
}


template <typename V>
void PerfectHash<V>::set(const int& key, const V& value){
    // Determine the index for positive or negative literals
    int i = (key > 0) ? key : (abs(key) + N);
    if(i < 0 or i >= values.size()){
        debugf("overflow: ",i, " key: ",key);
        exit(0) ; 
    }
    values[i] = value;
}

#include <typeinfo>


template <typename T>
std::string vector_to_string(const std::vector<T>& vec, std::string name = "", std::string parentesis = "[]"){
    std::ostringstream oss;
    int n = vec.size() ;
    
    oss<<name<<parentesis[0];
    for (int i = 0; i < n-1; i++)  oss<<"'"<<vec[i]<<"'"<<"," ;
    if (n > 0) oss<<"'"<<vec[n-1]<<"'";

    oss<<parentesis[1];
    return oss.str();
}



template <typename Key, typename Value>
std::string unordered_map_to_string(const std::unordered_map<Key, Value>& map){

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
std::vector<V> get_map_value_vector(std::map<K, std::vector<V>>& umap, K key) {
   
    if (umap.find(key) == umap.end()) {
        return std::vector<V>();
    }

    return umap[key];
}

template <typename K, typename V>
V get_map(std::unordered_map<K, V>& umap, K key, V default_value, bool insert = false) {   
    if (umap.find(key) == umap.end()){
        if(insert) umap[key] = default_value ;
        return default_value;
    }
    return umap[key];
}


template< typename T>
void extend_vector(std::vector<T>& to_extend, const std::vector<T>& input, int i = 0, int j = -1){

    if (j == -1 || j > input.size()) j = input.size();
    if (i < 0) i = 0;

    for (; i < j; i++) to_extend.push_back(input[i]);
}

template <typename T>
void extend_unordered_set(std::unordered_set<T>& to_extend, const std::vector<T>& input, int i = 0, int j = -1) {
    if (j == -1 || j > static_cast<int>(input.size())) j = input.size();
    if (i < 0) i = 0;

    for (; i < j; i++) {
        to_extend.emplace(input[i]);
    }
}


template <typename T>
class SymmetricFunction {
private:
    std::vector<T> data_structure;

protected:
    T NONE  ;   
    virtual T function_negative_lit(T value) const  {
        return value ;
    } 

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
    AggregateFunction(int N): PerfectHash(N, false){}
};

class PerfectSet: public PerfectHash<int>{
private:
    int cont = 0 ;
public:
    PerfectSet(int N): PerfectHash(N, -1){}

    int get(int key) const override{
        int value = PerfectHash::get(key);
        bool res = value == cont;
        // debugf("value: ",value, " cont: ", cont);
        // assert(!res);
        return res;
    }

    void set(const int& key, const int& value) override{
        // debugf("inserting with value ", value);
        // assert(value != true);
        if(value == true) PerfectHash::set(key, cont);
        else if(value == false) PerfectHash::set(key, cont-1);
        else assert(false) ;
        // debugf("Inserted value: ",PerfectHash::get(key));
    }

    inline constexpr void clear(){ ++cont ; }
};

class GroupFunction: public PerfectHash<Group*>{
public:
    GroupFunction(int N): PerfectHash(N, nullptr){}
};