#pragma once
#include <vector>
// #include "args-parser/args-parser/all.hpp"
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>
#include <ostream>
#include <algorithm>
#include <sstream>
#include "constants.h"
#include <compare>
#include <any>
#include <cmath>
#include <functional>
#include <random>



namespace fiore {

bool randomBool(const size_t& seed, float p = 0.5);
double random01(const size_t& seed);
template <typename T>
size_t choose(const std::vector<T>& v, std::mt19937& rng) noexcept;
size_t uniform(const size_t  start, const size_t end, std::mt19937& rng) noexcept;
//! Base exception of the library.
class Exception : public std::logic_error
{
public:
    explicit Exception(std::string what)
        : std::logic_error("Please use desc() method of the exception.")
        , m_what(std::move(what))
    {
    }

    virtual ~Exception() noexcept
    {
    }

    //! \return What as std::wstring.
    const std::string &desc() const noexcept
    {
        return m_what;
    }

private:
    //! What happened?
    std::string m_what;
}; // class Exception






inline std::string indent(int depth) noexcept {
    std::string ind = "";
    for (int i = 0; i < depth; ++i) {
      ind += "\t" ;
    }
    return ind ; 
}


inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) if (!token.empty()) tokens.push_back(token);
    return tokens;
}

struct PrintableRegex: public std::regex{
    std::string input;
    PrintableRegex(std::string s): std::regex(s), input(s){}
    PrintableRegex() = default;

    bool operator==(const PrintableRegex& other) const {
        return this->input == other.input;
    }

    bool operator!=(const PrintableRegex& other) const {
        return !((*this) == other);
    }

};

struct Integer: public PrintableRegex{
    Integer(): PrintableRegex("(-?[1-9]\\d*|0)"){}
};

struct UnsignedInteger: public PrintableRegex{
    UnsignedInteger(): PrintableRegex("([1-9]\\d*|0)"){}
};

struct UnsignedIntegerOrInf: public PrintableRegex{
    UnsignedIntegerOrInf(): PrintableRegex("([1-9]\\d*|inf)"){}
};

struct UnsignedIntegerOrUmax: public PrintableRegex{
    UnsignedIntegerOrUmax(): PrintableRegex("([1-9]\\d*|umax)"){}
};

struct Decimal: public PrintableRegex{
    Decimal(): PrintableRegex("-?\\d+(\\.\\d+)?"){}
};

struct PositiveInteger: public PrintableRegex{
    PositiveInteger(): PrintableRegex("[1-9]\\d*"){}
};

struct PositiveDecimal: public PrintableRegex{
    PositiveDecimal(): PrintableRegex("(0\\.\\d*[1-9]\\d*|[1-9]\\d*(\\.\\d+)?)"){}
};

struct OpenDecimal01: public PrintableRegex{
    OpenDecimal01(): PrintableRegex("0\\.\\d*[1-9]\\d*"){}
};

struct Decimal01: public PrintableRegex{
    Decimal01(): PrintableRegex("(0(\\.\\d+)?|1(\\.0+)?)"){}
};


struct Decimal01OrMaxIncrease: public PrintableRegex{
    Decimal01OrMaxIncrease(): PrintableRegex("((0(\\.\\d+)?|1(\\.0+)?)|max-increase)"){}
};

struct Word: public PrintableRegex{
    Word(): PrintableRegex("\\w+"){}
};

inline int32_t not_(int32_t value)noexcept{return -value;}
void setExitCode(int exitCode, bool exitOnError = false, std::string message = "")noexcept;
template<typename V>
std::string join(std::string unifier, const std::vector<V>& vec)noexcept;
template<typename V>
std::string join(std::string unifier, const std::unordered_set<V>& vec)noexcept;
char const *const * fromVecStr2VecCstring(std::vector<std::string>& vecString);
template<typename V>
std::vector<V> sliceVec(const std::vector<V>& data,  size_t i = 0, size_t j = 0){
    if (j == 0 || j > data.size()) j = data.size();
    if (i > data.size()) i = 0;

    std::vector<V> res;
    for (; i < j; ++i)
        res.push_back(data[i]);

    return res;
}

template<typename V>
inline std::vector<V> fromArrayToVector(const V* array, const size_t& size)noexcept{
    std::vector<V> vec;
    for(size_t i = 0; i < size; ++i) vec.push_back(array[i]);
    return vec;
}

inline std::string negateLiteralName(std::string literalName) noexcept {
    std::regex not_prefix("^not\\s+");
    return std::regex_match(literalName, not_prefix) ? std::regex_replace(literalName, not_prefix, "") :  "not " + literalName ;;
}



template<typename V>
void sliceVecInPlace(std::vector<V>& data,  size_t i = 0, size_t j = 0){ data = sliceVec(data, i, j);}

// void sliceVecInPlace(std::vector<std::pair<size_t, int>>& data,  size_t i = 0, size_t j = 0);


inline std::string createLiteralString(int32_t literalId)noexcept{ return std::string(literalId < 0 ? "not " : "") + "a" + std::to_string(abs(literalId)); }
using Clock = std::chrono::steady_clock;
inline Clock::time_point startTimer()noexcept{ return Clock::now(); }
inline double elapsedTime(const Clock::time_point& start)noexcept{
    auto end = Clock::now(); 
    std::chrono::duration<double> elapsed = (end - start);
    return elapsed.count();
}


// Enum
enum Event{AddedPBConstraint};



struct Subscriber{
    virtual void update(const std::unordered_map<std::string, std::any>& kwargs) noexcept = 0;
    virtual ~Subscriber() = default;
};

struct Publisher{
    std::vector<Subscriber*> subscribers;
    inline void subscribe(Subscriber* subscribe)noexcept{ subscribers.push_back(subscribe);}
    virtual ~Publisher() = default;
};

struct Mapping2Integer;

} // namespace fiore

namespace std {
    template<>
    struct hash<fiore::Mapping2Integer> { size_t operator()(const fiore::Mapping2Integer& obj) const noexcept; };
}

namespace fiore {
struct Mapping2Integer{ virtual int32_t integer() const noexcept = 0; virtual ~Mapping2Integer(){};};

struct Hasher { 
    size_t operator()(const Mapping2Integer& map2Int) const noexcept { 
        const size_t &k = std::hash<Mapping2Integer>{}(map2Int); 
        return k; 
    } 
    size_t operator()(const Mapping2Integer* map2Int) const noexcept { return this->operator()(*map2Int); } 
    size_t operator()(const int64_t map2Int) const noexcept { return std::hash<int64_t>{}(map2Int); } 
    size_t operator()(const size_t map2Int) const noexcept { return map2Int; } 
};

struct Literal: public Mapping2Integer{
    const int32_t id;
    const std::string name;

    Literal(int32_t id, std::string name)noexcept: id(id), name(name){}
    constexpr bool positive() const noexcept{ return id > 0;};
    inline bool operator==(const Literal& other) const noexcept{ return this->id == other.id; }
    inline int32_t integer() const noexcept override{ return id; }
    inline std::string toString() const{ std::ostringstream out; out<<'('<<this->name<<std::string(", ")<<this->id<<')'; return out.str(); }
    inline const Literal* operator!() const noexcept{ return new Literal(not_(id), negateLiteralName(name));}
};


struct SolvingLiteral: public Literal{

    const std::unordered_set<const Literal*> programLiterals;
    const bool isCallIdLiteral;

    SolvingLiteral(int32_t id, std::unordered_set<const Literal*> programLiterals =  std::unordered_set<const Literal*>(), bool isCallIdLiteral = false)noexcept:
    Literal(id, std::string("[" + join("&", programLiterals) + "]" + (isCallIdLiteral ?  "-activator" : ""))),
    programLiterals(programLiterals), 
    isCallIdLiteral(isCallIdLiteral)
    {}
};

struct TOP: public SolvingLiteral{
    TOP()noexcept:
    SolvingLiteral(CONSTANTS::TOP){}
};

struct BOTTOM: public SolvingLiteral{
    BOTTOM()noexcept:
    SolvingLiteral(CONSTANTS::BOTTOM){}
};




template< typename V>
inline void extendVector(std::vector<V>& toExtend, const std::vector<V>& input, size_t i = 0, size_t j = CONSTANTS::INFPlus32)noexcept{
    const size_t& n = input.size();
    if (j >= n ) j = n;
    for (; i < j; i++) toExtend.push_back(input[i]);
}

// template< typename V>
// inline void insertIntoVector(std::vector<V>& toModify, const std::vector<V>& toInsert, size_t i = 0, size_t j = CONSTANTS::INFPlus)noexcept{
//     const size_t& n = toInsert.size();
//     if (j >= n ) j = n;
//     for (; i < j; i++) toModify.push_back(toInsert[i]);
// }

template <typename V>
inline std::string vector2String(const std::vector<V>& vec, std::string name)noexcept{
    std::ostringstream oss;
    int n = vec.size() ;
    
    oss<<name<<'[';
    for (int i = 0; i < n-1; i++)  oss<<'\''<<vec[i]<<'\''<<std::string(", ") ;
    if (n > 0) oss<<'\''<<vec[n-1]<<'\'';

    oss<<']';
    return oss.str();
}


template <typename T1, typename T2>
inline std::string unorderedMap2String(const std::unordered_map<T1,T2>& umap, std::string name )noexcept{
    std::ostringstream oss;
    int n = umap.size() ;
    oss<<name<<'[';
    int i = 0 ;
    for(const auto& [literalId, value]: umap){
        oss<<'\''<<literalId<<':'<<'\''<<value<<"'"<<( i < n - 1 ? std::string(", ") : std::string(""));
        ++i;
    }
    oss<<']';
    return oss.str();
}

template <typename T1, typename T2>
inline T2 getDefault(const std::unordered_map<T1,T2>& umap, T1 key, T2 defaultValue, bool update = false)noexcept{
    auto it = umap.find(key);
    if(umap.find(key) == umap.end()){
        if(update) umap[key] = defaultValue;
        return defaultValue;
    }
    return it->second;
}

template <typename T1, typename T2>
inline void setDefault(std::unordered_map<T1,T2>& umap, T1 key, T2 defaultValue)noexcept{ if(umap.find(key) == umap.end())  umap[key] = defaultValue; }


template <typename V>
inline std::string unorderedSet2String(const std::unordered_set<V>& uset, std::string name)noexcept{
    std::ostringstream oss;
    int n = uset.size() ;
    
    oss<<name<<'{';

    int i = 0 ;
    for(const auto& value: uset){
        oss<<'\''<<value<<'\''<<( i < n - 1 ? std::string(", ") : std::string("")) ;
        ++i ;
    }

    oss<<'}';
    return oss.str();
}

template<typename V>
struct Invertable{ virtual V operator!() const noexcept = 0 ; virtual ~Invertable() = default;  };
struct InvertableInplace{ virtual void flip() noexcept = 0 ; virtual ~InvertableInplace() = default;};
struct IsUndefCheck{ virtual bool isUndef() const noexcept = 0 ; virtual ~IsUndefCheck() = default;};

// template<char Default = 2>
struct TruthValue: public Invertable<TruthValue>, public InvertableInplace, public IsUndefCheck{

    static constexpr char F = 0 ;
    static constexpr char T = 1;
    static constexpr char U = 2;
    static constexpr char Default = F;


    TruthValue(): data(Default){}
    TruthValue(const char& value): data(value){}

    inline TruthValue operator!() const noexcept override { return data == U ? TruthValue(U) : TruthValue(1 - data);}
    inline void flip() noexcept override{ if(data != U) data = (1 - data);}
    inline bool isTrue() const noexcept{return data == T;}
    inline bool isFalse()const noexcept{return data == F;}
    inline bool isUndef()const noexcept override{return data == U;}

    inline bool operator==(const TruthValue& other)const noexcept{return this->data == other.data;}
    inline bool operator<(const TruthValue& other)const noexcept{return this->data < other.data;}
    inline bool operator<=(const TruthValue& other)const noexcept{return (*this) < other || (*this) == other;}
    inline bool operator>(const TruthValue& other)const noexcept{return !((*this)<=other);}
    inline bool operator>=(const TruthValue& other)const noexcept{return !((*this)<other);}

    inline void True() noexcept { data = T;}
    inline void False() noexcept{ data = F;}
    inline void Undefined()noexcept{ data = U;}

    std::string toString()const noexcept{
        std::ostringstream out;
        if(this->isTrue()) out<<std::string("True");
        else if(this->isFalse()) out<<std::string("False");
        else out<<std::string("Undef");
        return out.str();
    }

    virtual ~TruthValue() = default;
    inline bool operator==(const TruthValue& other)noexcept{
        if(&other == this) return true;
        return this->data == other.data;
    }
    inline bool operator==(const char other)noexcept{
        return this->data == other;
    }
    inline bool operator!=(const TruthValue& other) noexcept{ return !(*this == other); }
    inline bool operator!=(const char other) noexcept{ return !(*this == other); }
private:
    char data;
};


template<typename K, typename V>
struct MapInterface{
    virtual void set(int32_t literalId, const Literal* literal)noexcept = 0;
    virtual const Literal* get(int32_t literalId)noexcept = 0;
};

struct MapLiteralId2LiteralInterface: public MapInterface<int32_t, const Literal*>{
    virtual const Literal* const& literal(const int32_t& literalId) const = 0;
    virtual void addLiteral(const Literal* literal)noexcept = 0;
    virtual void addFreshLiteral(const Literal* literal)noexcept = 0;
    
    inline void set(int32_t literalId, const Literal* literal)noexcept override {addLiteral(literal);}
    inline const Literal* get(int32_t literalId)noexcept override{return literal(literalId);}
};


template<typename T>
struct is_std_vector : std::false_type {};

template<typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type {};

template<typename T>
inline constexpr bool is_std_vector_v = is_std_vector<T>::value;


template<typename K, typename V, typename D, typename H = Hasher, typename VMapper = V, typename DMapper = D>
class Hash{

protected:
    const size_t N;
    D* _data;
    const V defaultValue;
    V defaultValueMutable;
    H hasher;

public:

    inline size_t hashKey(const K& key) const{return this->hasher(key);}
    inline const D& data() const noexcept { return *this->_data;}
    Hash(const size_t& N, V defaultValue = V()): N(N), defaultValue(defaultValue){}

    inline size_t size() const noexcept {return this->N;}

    virtual std::string toString(MapInterface<K,VMapper>* mapperToPrintable = nullptr, bool printDefault = false) const noexcept;
    inline const V& at(const K& key) const { return this->getUsingHash(this->hasher(key)); }
    inline V& get(const K& keyInput) { 
        const size_t& key = this->hasher(keyInput);
        assert(key < this->N);
        defaultValueMutable = defaultValue;
        if constexpr (std::is_same_v<D, std::unordered_map<size_t, V>>) {
            auto& umap = *this->_data;
            auto it = umap.find(key);
            return (it != umap.end()) ? it->second : defaultValueMutable;
        } else {
            return (*this->_data)[key];
        }
    }
    inline void set(const K& key, const V& value) { 
        this->setUsingHash(this->hasher(key), value);
    }

    virtual inline const V& getUsingHash(const size_t& key) const{ 
        assert(key < this->N);
        if constexpr (std::is_same_v<D, std::unordered_map<size_t, V>>) {
            auto& umap = *this->_data;
            auto it = umap.find(key);
            return (it != umap.end()) ? it->second : defaultValue;
        } else {
            return (*this->_data)[key];
        }
    }
    virtual inline void setUsingHash(const size_t& key, const V& value) { (*this->_data)[key] = value; }

    virtual ~Hash(){
        if(_data != nullptr) delete _data;
        if constexpr (std::is_pointer_v<V>){ 
            if(defaultValue != nullptr){ 
                delete defaultValue;
            }
        }
    }

    template<typename T = V>
    requires is_std_vector_v<T>
    void add(const K& k, const typename T::value_type& value) {
        auto& vec = (*this->_data)[this->hasher(k)];
        vec.push_back(value);
    }


    // std::string toString() override{ return "not implemented"; }

};

struct HasherHandlingNegatives { 
    size_t originalSize;
    size_t operator()(const Mapping2Integer* map2Int) const noexcept { 
        const int32_t& id = map2Int->integer();
        return (id > 0) ? static_cast<size_t>(id) : static_cast<size_t>(-id) + originalSize; 
    } 
    size_t operator()(const int32_t& id) const noexcept { 
        return (id > 0) ? static_cast<size_t>(id) : static_cast<size_t>(-id) + originalSize; 
    } 
};


template<typename K, typename V, typename D, typename VMapper = V, typename DMapper = D>
struct NHash: public Hash<K, V, D, HasherHandlingNegatives, VMapper, DMapper>{

    NHash(const size_t& N, V defaultValue = V()) noexcept: Hash<K, V, D, HasherHandlingNegatives, VMapper, DMapper>(N*2, std::move(defaultValue)), originalSize(N){
        // it is a (N * 2) vector where:
        //      values[:N-1]    are the values for the positive literals
        //      values[N:]      are the values for the negative literals
        // data = std::vector<T>(N*2, defaultValue);
        this->hasher.originalSize = originalSize;
    }    
    
    virtual std::string toString(MapInterface<K,VMapper>* mapperToPrintable = nullptr, bool printDefault = false) const noexcept override;
protected:
    const size_t originalSize;
};

template<typename K, typename V, typename H = Hasher, typename VMapper = V, typename DMapper = std::vector<V>>
class PerfectHash: public Hash<K, V, std::vector<V>, H, VMapper, DMapper>{
protected:
public:
    PerfectHash(size_t N, V defaultValue = V())noexcept: Hash<K, V, std::vector<V>, H, VMapper, DMapper>(N, std::move(defaultValue)){
        this->_data = new std::vector<V>(N,defaultValue);
    }
};

template<typename K, typename V, typename H = Hasher, typename VMapper = V, typename DMapper = std::unordered_map<int32_t,V>>
class UMHash: public Hash<K, V, std::unordered_map<size_t,V>, H, VMapper, DMapper>{
    
public:
    UMHash(size_t N, V defaultValue = V())noexcept:  Hash<K, V, std::unordered_map<size_t,V>, H, VMapper, DMapper>(N, std::move(defaultValue)){
        this->_data = new std::unordered_map<size_t,V>();
        for(size_t i = 1; i < this->size(); ++i) (*this->_data)[i] = defaultValue;
    }
};


template<typename K, typename V, typename VMapper = V, typename DMapper = std::unordered_map<size_t,V>>
struct UMNHash: public NHash<K, V, std::unordered_map<size_t,V>, VMapper, DMapper>{

    UMNHash(const size_t& N, V defaultValue = V()) noexcept: NHash<K, V, std::unordered_map<size_t,V>, VMapper, DMapper>(N*2, std::move(defaultValue)), originalSize(N){
        // it is a (N * 2) vector where:
        //      values[:N-1]    are the values for the positive element
        //      values[N:]      are the values for the negative negative
        this->_data = new std::unordered_map<size_t, V>();
        for(size_t i = 1; i < this->size(); ++i) (*this->_data)[i] = defaultValue;
    }    

protected:
    const size_t originalSize;
};


template<typename K, typename V, typename VMapper = V, typename DMapper = std::vector<V>>
struct PerfectNHash: public NHash<K, V, std::vector<V>, VMapper, DMapper>{

    PerfectNHash(const size_t& N, V defaultValue = V()) noexcept: NHash<K, V, std::vector<V>, VMapper, DMapper>(N, std::move(defaultValue)){
        // it is a (N * 2) vector where:
        //      values[:N-1]    are the values for the positive literals
        //      values[N:]      are the values for the negative literals
        // data = std::vector<T>(N*2, defaultValue);
        this->_data = new std::vector<V>(this->N, defaultValue);
    }    

    // inline void enlargeData(const clingo_literal_t& upTo)noexcept{
    //     const clingo_literal_t& r = upTo - this->N;
    //     std::vector<V> extend = std::vector<V>(r+1, this->defaultValue);
    //     extendVector(this->data, extend);
    //     this->N = this->data.size();
    // }   
};



// struct MapLiteralId2Literal: public MapLiteralId2LiteralInterface, public Hash<int32_t, const Literal*, std::unordered_map<int32_t, const Literal*>, std::hash<int32_t>, const Literal*, std::unordered_map<int32_t, const Literal*>>{

//     MapLiteralId2Literal(const size_t& N = 0) noexcept: Hash<int32_t, const Literal*, std::unordered_map<int32_t, const Literal*>, std::hash<int32_t>, const Literal*, std::unordered_map<int32_t, const Literal*>>(N, nullptr){}

//     const Literal* const& literal(const int32_t& literalId)override{
//         return this->getUsingHash(key);
//     }

//     inline const Literal* const& getUsingHash(const size_t& key) const override{ 
//         assert(key < this->N); 
//         auto it = this->data().find(key);
//         assert(it != this->data().end());
//         return it->second;
//     }

//     inline virtual void addLiteral(const Literal* literal)noexcept override{this->set(literal->id, literal);}
//     inline virtual void addFreshLiteral(const Literal* literal)noexcept override{literalsNotInMap.emplace(literal->id, literal);}

//     ~MapLiteralId2Literal(){
//         for(auto&[key, value] : this->data())if(value != nullptr) delete value;
//         for(auto&[key, value] : literalsNotInMap) if(value != nullptr) delete value;
//     }

// private:
//     mutable std::unordered_map<int32_t, const Literal*> literalsNotInMap;
// };


struct MapLiteralId2Literal: public MapLiteralId2LiteralInterface{
    std::unordered_map<int32_t, const Literal*> map;
    mutable std::unordered_map<int32_t, const Literal*> literalsNotInMap;

    MapLiteralId2Literal(const size_t& N = 0) noexcept{}

    inline const Literal* const& literal(const int32_t& literalId) const override{
        auto it =  map.find(literalId);
        if(it == map.end()){
            it =  map.find(not_(literalId));
            if(it != map.end()){
                const Literal* const & l = it->second;
                const Literal* nl = !(*l);
                literalsNotInMap[not_(literalId)] = nl;
                return literalsNotInMap[not_(literalId)];
            }
            if(literalsNotInMap.find(literalId) == literalsNotInMap.end()) {
                std::string name;
                if(literalId == CONSTANTS::BOTTOM || literalId == CONSTANTS::TOP)
                    name = literalId == CONSTANTS::BOTTOM ? "F" : "T";
                else
                    name = createLiteralString(literalId);
                literalsNotInMap[literalId] = new Literal(literalId, name);
            }
            return literalsNotInMap[literalId];
        }
        return it->second;
    }

    inline void addLiteral(const Literal* literal)noexcept override{
        map[literal->id] = literal;
    }

    inline void addFreshLiteral(const Literal* literal)noexcept override{
        addLiteral(literal);
    }

    inline std::string toString() const;

    virtual ~MapLiteralId2Literal(){
        for(auto&[key, value] : map) if(value != nullptr) delete value;
        for(auto&[key, value] : literalsNotInMap) if(value != nullptr) delete value;
    }
};

std::ostream& operator<<(std::ostream& out, const MapLiteralId2Literal map);

// struct MapLiteralId2Literal: public PerfectNHash<int32_t, const Literal*>{

//     MapLiteralId2Literal(const size_t& N) noexcept: PerfectNHash<int32_t, const Literal*>(N, nullptr){}

//     const Literal* const& literal(const int32_t& literalId){
//         const size_t key = this->hasher(literalId);
//         return this->getUsingHash(key);
//     }

//     inline const Literal* const& getUsingHash(const size_t& key) const override{ 
//         assert(key < this->N); 
//         const int32_t literalId = key < originalSize ? key : originalSize - key;
//         const Literal* &literal = (*this->_data)[key];
//         if(literal == nullptr){
//             std::string name = createLiteralString(literalId);
//             if(literalId == CONSTANTS::BOTTOM || literalId == CONSTANTS::TOP)
//                 name = literalId == CONSTANTS::BOTTOM ? "F" : "T";
//             if(literalsNotInMap.find(literalId) == literalsNotInMap.end()) 
//                 literalsNotInMap[literalId] = new Literal(literalId, name);
//             return literalsNotInMap[literalId];
//         }
//         return literal;
//     }

//     inline virtual void addLiteral(const Literal* literal)noexcept{this->set(literal->id, literal);}
//     inline virtual void addFreshLiteral(const Literal* literal)noexcept{literalsNotInMap.emplace(literal->id, literal);}

//     ~MapLiteralId2Literal(){
//         for(auto& value : this->data()) if(value != nullptr) delete value;
//         for(auto&[key, value] : literalsNotInMap) if(value != nullptr) delete value;
//     }

// private:
//     mutable std::unordered_map<int32_t, const Literal*> literalsNotInMap;
// };

template<typename K, typename V, typename D, typename VMapper = V, typename DMapper = D>
struct SymmetricFunction: public Hash<K, V, D, HasherHandlingNegatives, VMapper, DMapper>{

protected:

    virtual inline void setUsingHash(const size_t& key, const V& value) noexcept override { 
        assert(key < this->N * 2);
        if(key < this->N) (*this->_data)[key] = value; 
        else (*this->_data)[key - this->N] = !value; 
    }

public:
    SymmetricFunction(size_t N, V defaultValue = V())noexcept:  
        Hash<K, V, D, HasherHandlingNegatives, VMapper, DMapper>(N, std::move(defaultValue)) {
        this->hasher.originalSize = N;
    }
    
    virtual inline const V getSymmetricValue(const K& key) const noexcept { 
        size_t keyH = this->hasher(key);
        assert(keyH < this->N * 2);
        return keyH < this->N ? (*this->_data)[keyH] : !(*this->_data)[keyH - this->N]; 
    }
};

template<typename V>
struct Vector;

template<typename D>
struct Interpretation: public SymmetricFunction<int32_t, TruthValue, D, const Literal*, std::vector<const Literal*>>{

    Interpretation(size_t N, TruthValue defaultValue = TruthValue())noexcept:  
       SymmetricFunction<int32_t, TruthValue, D, const Literal*, std::vector<const Literal*>>(N, std::move(defaultValue)) {
    }
    
    inline bool isTrue(const int32_t& literalId) const noexcept{
        int32_t id = std::abs(literalId);
        TruthValue &value = (*this->_data)[id];
        if(value.isUndef()) return false;
        if(literalId < 0) return value.isFalse();
        return value.isTrue();
    }

    inline bool isFalse(const int32_t& literalId) const noexcept{ return isTrue(not_(literalId)); }

    inline bool isUndef(const int32_t& literalId) const noexcept{
        int32_t id = std::abs(literalId);
        TruthValue &value = (*this->_data)[id];
        if(value.isUndef()) return true;
        return false;
    }

    void set(const int32_t& literalId,  const char value){
        // ! Attention: char has to be a value in intervall [0,2]
        int32_t id = std::abs(literalId);
        D* d = this->_data;
        if(value == TruthValue::U){
            (*d)[id].Undefined(); 
            return;
        }
        if(literalId < 0){
            if(value == TruthValue::T) (*d)[id].False();
            else (*d)[id].True();
        }else{
            if(value == TruthValue::T) (*d)[id].True();
            else (*d)[id].False();
        }
    }

    std::string toInterpretationString(MapLiteralId2Literal* mapLiteralId2Literal = nullptr, bool printUndefined = false,  bool onlyTrueAtoms = true, const Vector<const Literal*>* toShow = nullptr) const noexcept;
    inline std::string toAnswerSetString(MapLiteralId2Literal* mapLiteralId2Literal, const Vector<const Literal*>* toShow = nullptr) const noexcept{
        std::ostringstream out;
        out<<std::string("Answer Set ")<<this->toInterpretationString(mapLiteralId2Literal, false, true, toShow);
        return out.str();
    }
    std::string toConstraint(MapLiteralId2Literal* mapLiteralId2Literal)noexcept;
};

struct PerfectInterpretation: public Interpretation<std::vector<TruthValue>>{

    PerfectInterpretation(size_t N, TruthValue defaultValue = TruthValue())noexcept:  
        Interpretation<std::vector<TruthValue>>(N, std::move(defaultValue)) {
        this->_data = new std::vector<TruthValue>(N,defaultValue);
    }

};

struct UMInterpretation: public Interpretation<std::unordered_map<size_t,TruthValue>>{

    UMInterpretation(size_t N, TruthValue defaultValue = TruthValue())noexcept:  
        Interpretation<std::unordered_map<size_t,TruthValue>>(N, std::move(defaultValue)) {
        this->_data = new std::unordered_map<size_t,TruthValue>();
        if(defaultValue != TruthValue::Default)
            for(size_t atomId = 1; atomId < this->size(); ++atomId) (*this->_data)[atomId] = defaultValue;
    }

};


template<typename V, typename D>
struct Set: public Hash<V, int, D>{

    Set(const size_t& N): Hash<V, int, D>(N,0), count(1){}

    inline bool contains (const V& elem) const {  return this->hasher(elem) >= this->N ? false : this->at(elem) == count; }
    inline void add(const V& elem){ this->set(elem, count); }
    inline void remove(const V& elem){  this->set(elem, count-1); }
    inline void clear()noexcept{ ++count; }

private:
    int count;
};


template<typename V>
struct PerfectSet: public Set<V, std::vector<int>>{
    PerfectSet(const size_t& N): Set<V, std::vector<int>>(N){
        this->_data = new std::vector<int>(this->N, this->defaultValue);
    }
};

template<typename V>
struct UMSet: public Set<V, std::vector<int>>{
    UMSet(const size_t& N): Set<V, std::vector<int>>(N){
        this->_data = new std::unordered_map<size_t, int>();
        for(size_t i = 1; i < this->size(); ++i) (*this->_data)[i] = this->defaultValue;
    }
};

template<typename V, typename D, typename VMapper = V>
struct NSet: public NHash<V, int, D>{

    NSet(const size_t& N): NHash<V, int, D>(N,0), count(1){}

    inline bool contains (const V& elem) const {   return elem >= ((int)this->originalSize) || elem <= -((int)this->originalSize) ? false : this->at(elem) == count; }
    inline void add(const V& elem){ this->set(elem, count); }
    inline void remove(const V& elem){  this->set(elem, count-1); }
    inline void clear()noexcept{ ++count; }

    std::string toStringSet(MapLiteralId2Literal* mapLiteralId2Literal = nullptr) const noexcept;
private:
    int count;
};


template<typename V>
struct PerfectNSet: public NSet<V, std::vector<int>>{
    PerfectNSet(const size_t& N): NSet<V, std::vector<int>>(N){
        this->_data = new std::vector<int>(this->N, this->defaultValue);
    }
};

template<typename V>
struct UMNSet: public NSet<V, std::vector<int>>{
    UMNSet(const size_t& N): NSet<V, std::vector<int>>(N){
        this->_data = new std::unordered_map<size_t, int>();
        for(size_t i = 1; i < this->size(); ++i) (*this->_data)[i] = this->defaultValue;
    }
};

template<typename V, typename Checker, typename Indexer>
struct IterableSet;


template<typename V>
std::ostream& operator<<(std::ostream& out, const Vector<V>& vec);
template<typename V, typename Checker, typename Indexer>
std::ostream& operator<<(std::ostream& out, const IterableSet<V, Checker, Indexer>& vec);

template<typename V>
struct Vector{

protected:
    std::vector<V> data;
  
public:
    Vector(const std::vector<V>& elements)noexcept : data(elements){}
    Vector(const size_t& N = 0)noexcept : data(N){}
    virtual ~Vector() = default;

    virtual inline bool contains(const V& elem) const noexcept{ return std::find(this->data.begin(), this->data.end(), elem) != this->data.end(); }
    virtual inline void add(const V& elem) noexcept{ data.push_back(elem);}
    virtual inline void clear()noexcept{  data.clear(); }
    template<typename Compare>
    void sort(Compare key)noexcept{ std::sort(data.begin(), data.end(), key); }
    const V& get(size_t index) const{ return data[index]; }
    virtual inline void remove(const V& elem) noexcept{ auto it = std::find(this->data.begin(), this->data.end(), elem); if(it != this->data.end()) this->data.erase(it);}
    virtual inline void sort(bool (*key)(V, V) = nullptr)noexcept{ if(key == nullptr) std::sort(data.begin(), data.end()); else std::sort(data.begin(), data.end(), key); }
    inline const std::vector<V> slice(size_t i = 0, size_t j = 0) const noexcept{ return sliceVec(data,i,j);}
    inline void sliceInPlace(size_t i = 0, size_t j = 0) noexcept{ return sliceVecInPlace<V>(data,i,j);}
    virtual inline void set(const size_t& i, const V& elem) noexcept{ this->data[i] = elem; }
    inline void push_back(const V& elem){ add(elem); }
    inline const V& back()const{ return data[data.size()-1]; }
    virtual inline void pop_back() noexcept { return data.pop_back(); }
    inline size_t size()const { return data.size(); }
    inline bool empty()const{ return data.empty();}
    inline auto begin() { return data.begin(); }
    inline auto end()   { return data.end(); }
    inline auto begin() const { return data.begin(); }
    inline auto end()   const { return data.end(); }
    virtual inline void resize(size_t k) noexcept { data.resize(k); } 
    virtual void reverse() noexcept;
    size_t index(const V& elem) const noexcept;
    std::string toString();
    void swapLeftRigth(const size_t i);
    void moveFrontToBack(const size_t i);
    virtual inline void moveForward(size_t i, size_t j) noexcept { std::rotate(data.begin()+i, data.begin()+i+1, data.begin()+j+1); }
    virtual inline void moveBackward(size_t i, size_t j) noexcept { std::rotate(data.begin()+i, data.begin()+j, data.begin()+j+1); }

    friend std::ostream& operator<< <V>(std::ostream& out, const Vector<V>& vec);
    inline const std::vector<V>& getData()noexcept{ return data;}

    std::unordered_map<V, size_t>  indexMap() const noexcept;
};


template<typename V, typename Checker, typename Indexer>
struct IterableSet: public Vector<V> {

private:
    inline void resetIndexRange(const size_t i, const size_t j) noexcept { for(size_t p = i; p <= j; ++p) indexer.set(this->data[p], p); }

protected:
    const size_t N;
    Checker checker;
    Indexer indexer;

public:
    IterableSet(const size_t N): Vector<V>(0), N(N), checker(N), indexer(N)  {}
    IterableSet(const std::vector<V>& elements, bool (*key)(V a, V b) = nullptr)noexcept;
    // PerfectVector(const PerfectVector<std::unique_ptr<T>& elements, bool (*key)(T a, T b) = nullptr)noexcept;
    // PerfectVector& operator=() noexcept;

    inline bool contains(const V& elem) const noexcept override { return checker.contains(elem); }
    void add(const V& elem)noexcept override;
    void remove(const V& elem) noexcept override;   
    void sort(bool (*key)(V, V) = nullptr)noexcept override;
    template<typename Compare>
    void sort(Compare key)noexcept;
    inline void clear()noexcept override{ this->data.clear(); checker.clear(); }
    void set(const size_t& i, const V& elem)noexcept override;
    void reverse()noexcept override;
    void resize(size_t k) noexcept override;
    inline void pop_back() noexcept override;
    void moveForward(size_t i, size_t j) noexcept override;
    void moveBackward(size_t i, size_t j) noexcept override;

};

template<typename V>
struct PerfectIterableSet: public IterableSet<V, PerfectSet<V> , PerfectHash<V,size_t>>{
    PerfectIterableSet(const size_t N): IterableSet<V, PerfectSet<V> , PerfectHash<V,size_t>>(N){}
    PerfectIterableSet(const std::vector<V>& elements, bool (*key)(V a, V b) = nullptr)noexcept: IterableSet<V, PerfectSet<V> , PerfectHash<V,size_t>>(elements, key){}
};

template<typename V>
struct PerfectIterableNSet: public IterableSet<V, PerfectNSet<V> , PerfectNHash<V,size_t>>{
    PerfectIterableNSet(const size_t N): IterableSet<V, PerfectNSet<V> , PerfectNHash<V,size_t>>(N){}
    PerfectIterableNSet(const std::vector<V>& elements, bool (*key)(V a, V b) = nullptr)noexcept: IterableSet<V, PerfectNSet<V> , PerfectNHash<V,size_t>>(elements, key){}
};

template<typename V>
struct UMIterableSet: public IterableSet<V, UMSet<V> , UMHash<V,size_t>>{
    UMIterableSet(const size_t N): IterableSet<V, UMSet<V> , UMHash<V,size_t>>(N){}
    UMIterableSet(const std::vector<V>& elements, bool (*key)(V a, V b) = nullptr)noexcept: IterableSet<V, UMSet<V> , UMHash<V,size_t>>(elements, key){}
};

template<typename V>
struct UMNIterableSet: public IterableSet<V, UMNSet<V> , UMNHash<V,size_t>>{
    UMNIterableSet(const size_t N): IterableSet<V, UMNSet<V> , UMNHash<V,size_t>>(N){}
    UMNIterableSet(const std::vector<V>& elements, bool (*key)(V a, V b) = nullptr)noexcept: IterableSet<V, UMNSet<V> , UMNHash<V,size_t>>(elements, key){}
};

// TODO OrderedIterableSet version (currently I do not need it!)
// void removeOrdered(const V& elem){
//     // ! It does not maintain order
//     if(contains(elem)){
//         const size_t &index = indexer.get(elem);
//         size_t n = data.size();
//         for(size_t i = index; i < n-1; ++i){
//             const V& elem =  data[i+1];
//             data[i] = elem;
//             indexer.set(elem, i);
//         }
//         checker.remove(elem);
//     }
// }


template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& out, const std::pair<T1, T2>& pair) noexcept;
std::ostream& operator<<(std::ostream& out, const char* value);
std::ostream& operator<<(std::ostream& out, const Literal& literal);
std::ostream& operator<<(std::ostream& out, const TruthValue& truthValue);
std::ostream& operator<<(std::ostream& out, const PrintableRegex& p);

} // namespace fiore

#include "utilityCommon.tpp"
