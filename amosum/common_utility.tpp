#pragma once
#include <ostream>
#include <sstream>
#include <memory>
#include <optional>
#include <functional>


template <typename V>
std::ostream& operator<<(std::ostream& out, const std::vector<V>& vec);


template <typename V>
std::ostream& operator<<(std::ostream& out, V* value){

    if(value != nullptr) out<<*value;
    else out<<std::string("None");
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

template<typename K, typename V, typename D, typename H = cmn::Hasher, typename VMapper = V, typename DMapper = D>
std::ostream& operator<<(std::ostream& out, const cmn::Hash<K,V,D,H,VMapper,DMapper>& hash){ out<<hash.toString();return out;}

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
std::ostream& operator<<(std::ostream& out, const std::pair<T1, T2>& pair) noexcept { return out<<"("<<pair.first<<", "<<pair.second<<")"; }

template <typename V>
inline std::ostream& operator<<(std::ostream& out, const std::unique_ptr<V>& value){ return out<<value.get();}

template<typename V, typename Checker, typename Indexer>
std::ostream& operator<<(std::ostream& out, const cmn::ContiguousSet<V, Checker, Indexer>& vec){ return out<<vec.data; }


template<typename V>
std::ostream& operator<<(std::ostream& out, const cmn::Vector<V>& vec){ return out<<vec.data; }

namespace cmn{

template <typename K, typename V, typename LessEqual = std::less_equal<K>>
K maxKey(const std::unordered_map<K, V>& m, LessEqual lessOrEqual = LessEqual{}) {
    
    K maxK = m.begin()->first;
    for (const auto& [k, _] : m)
        if (lessOrEqual(maxK, k))
            maxK = k;

    return maxK;
}

template<typename V>
std::string join(std::string unifier, std::vector<V> vec)noexcept{
    std::ostringstream oss;

    for(size_t i = 0; i < vec.size() -1 ; ++i) oss<<vec[i]<<unifier;
    if(vec.size() > 1) oss<<vec[vec.size()-1];

    return oss.str();
}

template<typename V>
std::string join(std::string unifier, std::unordered_set<V> set)noexcept{
    std::ostringstream oss;

    size_t i = 0;
    size_t n = set.size();
    for(const V& elem: set) oss<<elem<< ((i++ < n-1) ? unifier : "");

    return oss.str();
}

} // namespace cmn

template<typename K, typename V, typename D, typename H, typename VMapper, typename DMapper>
std::string cmn::Hash<K, V, D, H,VMapper,DMapper>::toString(cmn::MapInterface<K,VMapper>* mapperToPrintable, bool printDefault) const noexcept{
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
        outKey.flush();
        dictStringLiteralToValue[stringKey] = element;
    }
    ::operator<<(out,dictStringLiteralToValue);
    return out.str();
}

template<typename K, typename V, typename D, typename VMapper, typename DMapper>
std::string cmn::NHash<K,V,D,VMapper, DMapper>::toString(MapInterface<K,VMapper>* mapperToPrintable, bool printDefault) const noexcept{
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
    ::operator<<(out,dictStringLiteralToValue);
    return out.str();
}

inline bool isAtomInsideVectorLiterals(const cmn::Vector<const cmn::Literal*>& vec, const size_t& atomId){
    for(const cmn::Literal* const& l: vec) if((size_t)std::abs(l->id) == atomId) return true;
    return false;
}


template<typename D>
std::string cmn::Interpretation<D>::toInterpretationString(cmn::MapLiteralId2Literal* mapLiteralId2Literal, bool printUndefined,  bool onlyTrueAtoms, const cmn::Vector<const cmn::Literal*>* toShow) const noexcept {

    std::unordered_map<std::string, cmn::TruthValue> result1;
    std::unordered_set<std::string> result2;
    std::ostringstream oss;
    bool enabledToShow = toShow != nullptr ;
    for(size_t i = 1; i < this->size(); ++i){
        size_t atomId = i;
        if((!printUndefined && this->isUndef(atomId)) || (enabledToShow &&  !isAtomInsideVectorLiterals(*toShow, atomId))) continue;
        std::string stringLiteral;
        if(mapLiteralId2Literal != nullptr){
            const cmn::Literal* literal =  mapLiteralId2Literal->literal(atomId);
            std::ostringstream outKey;
            outKey<<literal->name;
            stringLiteral = outKey.str();
        }else{
            stringLiteral = createLiteralString(atomId);
        }
        
        if(onlyTrueAtoms && this->isFalse(atomId)) continue;

        if(!onlyTrueAtoms) result1[stringLiteral] = this->get(atomId);
        else result2.emplace(stringLiteral);
    }

    if(!onlyTrueAtoms) ::operator<<(oss,result2);
    else  ::operator<<(oss,result2);

    return oss.str();
}


template<typename D>
std::string cmn::Interpretation<D>::toConstraint(cmn::MapLiteralId2Literal* mapLiteralId2Literal)noexcept{

    std::ostringstream oss;

    for(size_t atomId = 1; atomId < this->size(); ++atomId){     
        const cmn::TruthValue& truthValue = this->get(atomId); 
        if(truthValue.isUndef()) continue;
        oss<<mapLiteralId2Literal->get(atomId);
        std::string stringLiteral = oss.str();
        std::string notStr = truthValue.isTrue() ? "not " : "";
        oss<<std::string(":- ")<<notStr<<stringLiteral<<std::string(".\n");
    }

    return oss.str();
}

// template<typename V, typename DC, typename DI>
// IndexedVector<V,DC, DI>::IndexedVector(const std::vector<V>& elements, bool (*key)(V a, V b))noexcept: 
//     N(this->hasher((*std::max_element(elements.begin(), elements.end(), key)))+1), 
//     checker(N), indexer(N){
//     for(const V& elem: elements) this->add(elem);
// }

