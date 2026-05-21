// #include "stats.h"
// #include "logger.h"

// #ifndef NSTATS 

// std::string Stat::toString(int ind){
//     std::ostringstream oss;
//     oss << indent(ind) << "Stat: " << name << "\n";
//     ++ind;
//     oss << indent(ind) << "Count: " << count << "\n";
//     oss << indent(ind) << "Sum: " << (decimal ? sum : int(sum)) << "\n";
//     oss << indent(ind) << "Min: " << (decimal ? min : int(min)) << "\n";
//     oss << indent(ind) << "Max: " << (decimal ? max : int(max)) << "\n";
//     oss << indent(ind) << "Mean: " << sum/count << "\n";
//     return oss.str();
// }

// std::string Stat::toStringOneline(){
//     std::ostringstream oss;
//     oss << "\tStat(" << name << "){";
//     oss << "Count:\t" << count << "\t";
//     oss << "Sum:\t" << (decimal ? sum : int(sum)) << "\t";
//     oss << "Min:\t" << (decimal ? min : int(min)) << "\t";
//     oss << "Max:\t" << (decimal ? max : int(max)) << "\t";
//     double mean = count > 0 ? sum/count : 0;
//     oss << "Mean:\t" << mean  << "}";
//     return oss.str();
// }

// void  Stat::add_value(float value){
//     count++;
//     sum += value;
//     if(min == 0) min = value;
//     if(max == 0) max = value;
//     if(value < min) min = value;
//     if(value > max) max = value;
// }

// std::string HandleStats::toString(){
//     std::ostringstream oss;
//     oss << "Stats report: \n";
//     for(auto& [name, stat]: stats){
//         oss <<  stat->toString(1);
//     }
//     return oss.str();
// }

// std::string HandleStats::toStringOneline(){
//     std::ostringstream oss;
//     for(auto& [name, stat]: stats){
//         oss <<  stat->toStringOneline();
//     }
//     return oss.str();
// }

// void HandleStats::printStringOneline(){
//     std::ostringstream oss;
//     for(auto& [name, stat]: stats){
//         oss <<  stat->toStringOneline();
//     }
//     std::string out =  oss.str();
//     logFile<< out << "\n";
//     logFile.flush(); 
// }

// HandleStats* HandleStats::instance = nullptr;

// void HandleStats::add_stat(Stat* stat){
//     stats[stat->name] = stat;
// }

// void HandleStats::remove_stat(Stat* stat){
//     auto it = stats.find(stat->name);
//     if(it != stats.end()){
//         delete it->second;
//         stats.erase(it);
//     }
// }

// void HandleStats::add_value(std::string name, float value){
    
//     auto it = stats.find(name);
//     if(it == stats.end()){
//         stats[name] = new Stat(name, true);
//     }
//     stats[name]->add_value(value);
// }

// #endif
