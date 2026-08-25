#include "Population.hpp"
#include "Planet.hpp"
namespace Civitron{
    Population::Population(){

    }
    nlohmann::json Population::ToJson(){
        nlohmann::json j;
        std::vector<int> indexes = {};
        for (int i = 0; i < humans.size(); i ++){
            for (int j = 0; j < planet->entities.size(); j ++){
                if (planet->entities[j].get() == humans[i]){
                    indexes.push_back(j);
                    break;
                }
            }
        }
        j["humans"] = indexes;
        return j;
    }
    void Population::FromJson(nlohmann::json& j){
        for (int i = 0; i < j["humans"].size(); i ++){
            humans.push_back((Human*)(planet->entities[j["humans"][i]].get()));
        }
    }
}