#include "stats.h"
#include "logger.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <numeric>
#include <sstream>


namespace fiore {

double StatSummary::median() const noexcept{
    if(sample.empty()) return 0.0;
    std::vector<double> s = sample;
    const size_t mid = s.size() / 2;
    std::nth_element(s.begin(), s.begin() + mid, s.end());
    const double upper = s[mid];
    if(s.size() % 2 == 1) return upper;
    const double lower = *std::max_element(s.begin(), s.begin() + mid);
    return (lower + upper) / 2.0;
}

namespace {

    void writeJsonString(std::ostream& out, const std::string& s){
        out<<'"';
        for(const char c: s){
            switch(c){
                case '"':  out<<"\\\""; break;
                case '\\': out<<"\\\\"; break;
                case '\n': out<<"\\n";  break;
                case '\r': out<<"\\r";  break;
                case '\t': out<<"\\t";  break;
                default:
                    if(static_cast<unsigned char>(c) < 0x20){
                        char buffer[7];
                        std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned char>(c));
                        out<<buffer;
                    }else out<<c;
            }
        }
        out<<'"';
    }

    // JSON has no NaN or infinity: they are written as null
    void writeJsonValue(std::ostream& out, const double v){
        if(std::isfinite(v)) out<<std::setprecision(17)<<v;
        else out<<"null";
    }

    void writeJsonValue(std::ostream& out, const StatValue& v){
        std::visit([&out](const auto& x){
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::string>) writeJsonString(out, x);
            else if constexpr (std::is_same_v<T, int64_t>) out<<x;
            else writeJsonValue(out, x);
        }, v);
    }

    std::vector<std::string> sortedKeys(const auto& m){
        std::vector<std::string> keys;
        keys.reserve(m.size());
        for(const auto& [k, _]: m) keys.push_back(k);
        std::sort(keys.begin(), keys.end());
        return keys;
    }

    // indent >= 0: one entry per line, indented by `indent` levels (2 spaces each); indent < 0: one line
    // keys are sorted: unordered_map iteration order changes between runs, sorted output can be diffed
    template<typename V>
    void writeJsonObject(std::ostream& out, const std::unordered_map<std::string, V>& m, const int indent = -1){
        const std::vector<std::string> keys = sortedKeys(m);
        if(keys.empty()){ out<<"{}"; return; }

        const bool pretty = indent >= 0;
        const std::string inner = pretty ? std::string(2 * (indent + 1), ' ') : "";
        const std::string outer = pretty ? std::string(2 * indent, ' ') : "";
        out<<'{';
        for(size_t i = 0; i < keys.size(); ++i){
            if(i > 0) out<<',';
            out<<(pretty ? "\n" : (i > 0 ? " " : "")) <<inner;
            writeJsonString(out, keys[i]);
            out<<": ";
            writeJsonValue(out, m.at(keys[i]));
        }
        out<<(pretty ? "\n" + outer : "")<<'}';
    }

    // one summary as a JSON object; `value` is the stat itself (the sum for increased stats)
    void writeJsonSummary(std::ostream& out, const StatValue& value, const StatSummary& s, const int indent){
        const bool pretty = indent >= 0;
        const std::string inner = pretty ? std::string(2 * (indent + 1), ' ') : "";
        const std::string outer = pretty ? std::string(2 * indent, ' ') : "";
        const std::string sep = pretty ? ",\n" + inner : ", ";
        out<<'{'<<(pretty ? "\n" + inner : "");
        out<<"\"value\": ";  writeJsonValue(out, value);    out<<sep;
        out<<"\"count\": "<<s.count;                         out<<sep;
        out<<"\"sum\": ";    writeJsonValue(out, s.sum);     out<<sep;
        out<<"\"min\": ";    writeJsonValue(out, s.min);     out<<sep;
        out<<"\"max\": ";    writeJsonValue(out, s.max);     out<<sep;
        out<<"\"mean\": ";   writeJsonValue(out, s.mean());  out<<sep;
        out<<"\"median\": "; writeJsonValue(out, s.median()); out<<sep;
        out<<"\"medianExact\": "<<(s.isMedianExact() ? "true" : "false");
        out<<(pretty ? "\n" + outer : "")<<'}';
    }

    // generic stats: increased stats as a summary object, set stats as a plain value
    void writeJsonGeneric(std::ostream& out, const std::unordered_map<std::string, StatValue>& values,
                          const std::unordered_map<std::string, StatSummary>& summaries, const int indent){
        const std::vector<std::string> keys = sortedKeys(values);
        if(keys.empty()){ out<<"{}"; return; }

        const bool pretty = indent >= 0;
        const std::string inner = pretty ? std::string(2 * (indent + 1), ' ') : "";
        const std::string outer = pretty ? std::string(2 * indent, ' ') : "";
        out<<'{';
        for(size_t i = 0; i < keys.size(); ++i){
            if(i > 0) out<<',';
            out<<(pretty ? "\n" : (i > 0 ? " " : "")) <<inner;
            writeJsonString(out, keys[i]);
            out<<": ";
            const StatValue& value = values.at(keys[i]);
            auto it = summaries.find(keys[i]);
            if(it != summaries.end()) writeJsonSummary(out, value, it->second, pretty ? indent + 1 : -1);
            else writeJsonValue(out, value);
        }
        out<<(pretty ? "\n" + outer : "")<<'}';
    }
}


Statistics* Statistics::getInstance() noexcept{ assert(instance != nullptr); return instance; }
void Statistics::initInstance(std::string logFileName) noexcept{
    if(instance == nullptr){
        instance = new Statistics(logFileName);
    }
}

void Statistics::printOnFile(){
    logFile<<this->toJson()<<"\n";
    logFile.flush(); 
}
void Statistics::printOnSTDOUT(){
    std::cout<<this->toJson()<<std::endl;
}





std::string Statistics::toJson() const noexcept{
    std::ostringstream out;
    out<<"{\n";
    out<<"  \"lastSolve\": "; writeJsonObject(out, statsLastSolve, 1); out<<",\n";
    out<<"  \"solver\": ";    writeJsonObject(out, statsSolver, 1);    out<<",\n";
    out<<"  \"generic\": ";   writeJsonGeneric(out, genericStats, genericSummaries, 1);
    out<<",\n  \"series\": {";
    const std::vector<std::string> seriesKeys = sortedKeys(series);
    for(size_t i = 0; i < seriesKeys.size(); ++i){
        if(i > 0) out<<',';
        out<<"\n    ";
        writeJsonString(out, seriesKeys[i]);
        out<<": [";
        const std::vector<std::pair<double, double>>& points = series.at(seriesKeys[i]);
        for(size_t p = 0; p < points.size(); ++p){
            if(p > 0) out<<", ";
            out<<'[';
            writeJsonValue(out, points[p].first);
            out<<", ";
            writeJsonValue(out, points[p].second);
            out<<']';
        }
        out<<']';
    }
    out<<(seriesKeys.empty() ? "}" : "\n  }");
    #ifdef __debug__
        std::vector<std::string> names = sortedKeys(customStatsSolver);
        out<<",\n  \"propagators\": {";
        for(size_t i = 0; i < names.size(); ++i){
            if(i > 0) out<<',';
            out<<"\n    ";
            writeJsonString(out, names[i]);
            out<<": ";
            writeJsonObject(out, customStatsSolver.at(names[i]), 2);
        }
        out<<(names.empty() ? "}" : "\n  }");
    #endif
    out<<"\n}";
    return out.str();
}

std::string Statistics::toString() const noexcept{ 
    std::ostringstream oss;
    oss<<"Statistics("<<this->toStringStatsSolver()<<","<<this->toStringGenericStats()<<")";
    return oss.str();
}

std::string Statistics::toStringStatsSolver() const noexcept{
    std::ostringstream oss;
    if(this->statsLastSolve.size() > 0){ oss<<"Last Call: "; writeJsonObject(oss, this->statsLastSolve); }
    if(this->statsSolver.size() > 0){ oss<<" Acc: "; writeJsonObject(oss, this->statsSolver); }
    #ifdef __debug__
        if(this->customStatsSolver.size() > 0) oss<<" customStatsSolver: "<<customStatsSolver;
    #endif
    return oss.str();
}

std::string Statistics::toStringGenericStats() const noexcept{
    std::ostringstream oss;
    if(genericStats.size() > 0){ oss<<" Generic: "; writeJsonGeneric(oss, this->genericStats, this->genericSummaries, -1); }
    return oss.str();
}

inline Statistics* Statistics::instance = nullptr;

std::ostream& operator<<(std::ostream& out, const Statistics& stats){ return out<<stats.toString(); }

} // namespace fiore
