#pragma once
#include "utilityCommon.h"
#include <ostream>
#include <sstream>
#include <memory>
#include <optional>
#include <functional>


namespace fiore {

template <typename V>
std::ostream& operator<<(std::ostream& out, V* value){

    if(value != nullptr) out<<*value;
    else out<<std::string("None");
    return out;
}

template <typename T>
size_t choose(const std::vector<T>& v, std::mt19937& rng) noexcept{
    std::uniform_int_distribution<size_t> dist(0, v.size()-1);
    return dist(rng);
}

template <typename T>
size_t uniform(const size_t  start, const size_t end, std::mt19937& rng) noexcept{
    std::uniform_int_distribution<size_t> dist(start, end);
    return dist(rng);
}

template <typename V>
std::ostream& operator<<(std::ostream& out, const std::vector<V>& vec){
    int n = vec.size() ;

    out<<'[';
    for (int i = 0; i < n-1; i++)  out<<'\''<<vec[i]<<'\''<<std::string(", ") ;
    if (n > 0) out<<'\''<<vec[n-1]<<'\'';

    out<<']';

    return out;
}

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& out, const std::unordered_map<T1,T2>& umap){

    int n = umap.size() ;
    out<<'[';
    int i = 0 ;
    for(const auto& [literalId, value]: umap){
        out<<'\''<<literalId<<'\''<<": \'"<<value<<'\''<<( i < n - 1 ? std::string(", ") : std::string("")) ;
        ++i ;
    }
    out<<']';
    
    return out;
}

template <typename V>
std::ostream& operator<<(std::ostream& out, const std::unordered_set<V>& uset){

    int n = uset.size() ;
    out<<'{';
    int i = 0 ;
    for(const auto& value: uset){
        out<<'\''<<value<<'\''<<( i < n - 1 ? std::string(", ") : std::string("")) ;
        ++i ;
    }

    out<<'}';
    return out;
}

template <typename K, typename V, typename LessEqual = std::less_equal<K>>
K maxKey(const std::unordered_map<K, V>& m, LessEqual lessOrEqual = LessEqual{}) {
    if (m.empty()) throw Exception("Max Key failed, empty map");

    K maxK = m.begin()->first;
    for (const auto& [k, _] : m)
        if (lessOrEqual(maxK, k))
            maxK = k;

    return maxK;
}

template<typename K, typename V, typename D, typename H = Hasher, typename VMapper = V, typename DMapper = D>
std::ostream& operator<<(std::ostream& out, const Hash<K,V,D,H,VMapper,DMapper>& hash){ out<<hash.toString();return out;}



template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& out, const std::pair<T1, T2>& pair) noexcept { return out<<"("<<pair.first<<", "<<pair.second<<")"; }

template <typename V>
inline std::ostream& operator<<(std::ostream& out, const std::unique_ptr<V>& value){ return out<<value.get();}

template<typename V, typename Checker, typename Indexer>
std::ostream& operator<<(std::ostream& out, const IterableSet<V, Checker, Indexer>& vec){ return out<<vec.data; }

template<typename V>
std::ostream& operator<<(std::ostream& out, const Vector<V>& vec){ return out<<vec.data; }



template<typename V>
std::string join(std::string unifier, const std::vector<V>& vec)noexcept{
    std::ostringstream oss;
    for(size_t i = 0; i < vec.size(); ++i){
        if(i > 0) oss<<unifier;
        oss<<vec[i];
    }
    return oss.str();
}

template<typename V>
std::string join(std::string unifier, const std::unordered_set<V>& set)noexcept{
    std::ostringstream oss;

    size_t i = 0;
    size_t n = set.size();
    for(const V& elem: set) oss<<elem<< ((i++ < n-1) ? unifier : "");

    return oss.str();
}

template<typename K, typename V, typename D, typename H, typename VMapper, typename DMapper>
std::string Hash<K, V, D, H,VMapper,DMapper>::toString(MapInterface<K,VMapper>* mapperToPrintable, bool printDefault) const noexcept{
    std::unordered_map<std::string, V> dictStringLiteralToValue;
    std::ostringstream out;
    for(size_t key = 0; key<this->size(); ++key){
        const V& element = this->getUsingHash(key);
        if(!printDefault && element == this->defaultValue) continue;
        std::string stringKey;
        std::ostringstream outKey;
        if(mapperToPrintable != nullptr) outKey<<mapperToPrintable->get(key);            
        else outKey<<key;            
        stringKey = outKey.str();
        dictStringLiteralToValue[stringKey] = element;
    }
    out<<dictStringLiteralToValue;
    return out.str();
}

template<typename K, typename V, typename D, typename VMapper, typename DMapper>
std::string NHash<K,V,D,VMapper, DMapper>::toString(MapInterface<K,VMapper>* mapperToPrintable, bool printDefault) const noexcept{
    std::unordered_map<std::string, V> dictStringLiteralToValue;
    std::ostringstream out;
    for(size_t key = 0; key<this->size(); ++key){
        if(key == originalSize) continue;
        const V& element = this->getUsingHash(key);
        if(!printDefault && element == this->defaultValue) continue;
        std::string stringKey;
        int32_t literalId = key > originalSize ? originalSize - key : key;
        std::ostringstream outKey;
        if(mapperToPrintable != nullptr) outKey<<mapperToPrintable->get(literalId);            
        else outKey<<createLiteralString(literalId);            
        stringKey = outKey.str();
        outKey.flush();
        dictStringLiteralToValue[stringKey] = element;
    }
    out<<dictStringLiteralToValue;
    return out.str();
}

template<typename V, typename D, typename VMapper>
std::string  NSet<V,D, VMapper>::toStringSet(MapLiteralId2Literal* mapLiteralId2Literal)const noexcept{
    std::unordered_set<std::string> uset;
    std::ostringstream out;
    for(size_t key = 0; key<this->size(); ++key){
        if(key == this->originalSize) continue;
        int32_t originalId = key > this->originalSize ? this->originalSize - key : key;
        std::ostringstream outKey;
        if(!this->contains(originalId)) continue;            
        if(mapLiteralId2Literal != nullptr) outKey<<mapLiteralId2Literal->literal(originalId);            
        else outKey<<createLiteralString(originalId);
        std::string stringKey;            
        stringKey = outKey.str();
        outKey.flush();
        uset.emplace(stringKey);
    }
    out<<uset;
    return out.str();
}

inline bool isAtomInsideVectorLiterals(const Vector<const Literal*>& vec, const size_t& atomId){
    for(const Literal* const& l: vec) if((size_t)std::abs(l->id) == atomId) return true;
    return false;
}


template<typename D>
std::string Interpretation<D>::toInterpretationString(MapLiteralId2Literal* mapLiteralId2Literal, bool printUndefined,  bool onlyTrueAtoms, const Vector<const Literal*>* toShow) const noexcept {

    std::unordered_map<std::string, TruthValue> result1;
    std::unordered_set<std::string> result2;
    std::ostringstream oss;
    bool enabledToShow = toShow != nullptr ;
    for(size_t i = 1; i < this->size(); ++i){
        size_t atomId = i;
        if((!printUndefined && this->isUndef(atomId)) || (enabledToShow &&  !isAtomInsideVectorLiterals(*toShow, atomId))) continue;
        std::string stringLiteral;
        if(mapLiteralId2Literal != nullptr){
            const Literal* literal =  mapLiteralId2Literal->literal(atomId);
            std::ostringstream outKey;
            outKey<<literal->name;
            stringLiteral = outKey.str();
        }else{
            stringLiteral = createLiteralString(atomId);
        }
        
        if(onlyTrueAtoms && this->isFalse(atomId)) continue;

        if(!onlyTrueAtoms) result1[stringLiteral] = this->at(atomId);
        else result2.emplace(stringLiteral);
    }

    if(!onlyTrueAtoms) oss<<result1;
    else oss<<result2;

    return oss.str();
}


template<typename D>
std::string Interpretation<D>::toConstraint(MapLiteralId2Literal* mapLiteralId2Literal)noexcept{
    std::ostringstream oss;
    for(size_t atomId = 1; atomId < this->size(); ++atomId){
        const TruthValue& truthValue = this->at(atomId);
        if(truthValue.isUndef()) continue;
        std::ostringstream name;
        name<<mapLiteralId2Literal->get(atomId);
        oss<<":- "<<(truthValue.isTrue() ? "not " : "")<<name.str()<<".\n";
    }
    return oss.str();
}

template<typename V>
size_t Vector<V>::index(const V& elem) const noexcept{
    size_t i = 0;
    const size_t& n = this->data.size();
    const auto& d = this->data;
    for(; i < n; ++i) if(d[i] == elem) break;
    return i;
}
template<typename V>
std::unordered_map<V, size_t>  Vector<V>::indexMap() const noexcept{
    std::unordered_map<V, size_t> res;
    const size_t& n = this->data.size();
    const auto& d = this->data;
    for(size_t i = 0; i < n; ++i) 
        res[d[i]] = i ;
    return res;
}

template<typename V>
void Vector<V>::reverse() noexcept { std::reverse(data.begin(), data.end()); }

template<typename V>
std::string Vector<V>::toString(){
    std::ostringstream oss;
    oss<<this;
    return oss.str();
}


template<typename V>
void Vector<V>::swapLeftRigth(const size_t i){
    if(i + 1 >= data.size()) return;
    std::rotate(data.begin(), data.begin() + i + 1, data.end());
}

// moves the left part [0, i] to the back; the order inside each part is not preserved
template<typename V>
void Vector<V>::moveFrontToBack(const size_t i){
    const size_t n = data.size();
    if(i + 1 >= n) return;
    const size_t L = i + 1;              // size of the left part
    const size_t m = std::min(L, n - L);
    std::swap_ranges(data.begin(), data.begin() + m, data.end() - m);
}


template<typename V, typename DC, typename DI>
IterableSet<V,DC, DI>::IterableSet(const std::vector<V>& elements, bool (*key)(V a, V b))noexcept:
    Vector<V>(0),
    N(elements.empty() ? 0 : this->hasher(key != nullptr ? *std::max_element(elements.begin(), elements.end(), key): *std::max_element(elements.begin(), elements.end())) + 1),
    checker(N), indexer(N){
    for(const V& elem: elements) this->add(elem);
}

template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::add(const V& elem)noexcept{
    if(!contains(elem)){
        checker.add(elem);
        size_t i = this->data.size();
        indexer.set(elem, i);
        this->data.push_back(elem);
    }
}

template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::sort(bool (*key)(V, V))noexcept{
    Vector<V>::sort(key);
    for(size_t i = 0; i < this->data.size(); ++i){
        const V& elem = this->data[i];
        indexer.set(elem, i);
    }
}

template<typename V, typename DC, typename DI>
template<typename Compare>
void IterableSet<V,DC, DI>::sort(Compare key)noexcept{
    Vector<V>::sort(key);
    for(size_t i = 0; i < this->data.size(); ++i){
        const V& elem = this->data[i];
        indexer.set(elem, i);
    }
}

template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::set(const size_t& i, const V& elem)noexcept{
    const V old = this->data[i];
    if(old == elem) return;
    assert(!contains(elem));   // a set cannot hold elem twice
    this->data[i] = elem;
    indexer.set(elem, i);
    checker.remove(old);
    checker.add(elem);
}

template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::reverse()noexcept{
    std::reverse(this->data.begin(), this->data.end());
    for(size_t i = 0; i < this->data.size(); ++i) indexer.set(this->data[i], i);
}

template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::remove(const V& elem) noexcept{
    // ! It does not maintain order
    if(contains(elem)){
        const size_t &i = indexer.get(elem);
        V x = this->data[i];
        size_t n = this->data.size();
        V &last =  this->data[n-1];
        indexer.set(last, i);
        this->data[i] = last;
        this->data[n-1] = x;
        this->pop_back();
    }
}

template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::resize(size_t k) noexcept{
    while(this->data.size() > k) this->pop_back();
}


template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::pop_back() noexcept{
    checker.remove(this->data[this->data.size()-1]);
    Vector<V>::pop_back();
}


template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::moveForward(size_t i, size_t j) noexcept { 
    Vector<V>::moveForward(i, j); 
    resetIndexRange(i, j);
}

template<typename V, typename DC, typename DI>
void IterableSet<V,DC, DI>::moveBackward(size_t i, size_t j) noexcept { 
    Vector<V>::moveBackward(i, j); 
    resetIndexRange(i, j);
}

} // namespace fiore
