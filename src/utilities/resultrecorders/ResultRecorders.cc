//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
#include "ResultRecorders.h"

#include <cmessage.h>
#include <limits>

using namespace TTEthernetModel;

FloatingIntervalVectorRecorder::FloatingIntervalVectorRecorder(){
    interval = SimTime(-1);
    handle = NULL;
    lastTime = 0;
}

void FloatingIntervalVectorRecorder::subscribedTo(cResultFilter *prev)
{
    cNumericResultRecorder::subscribedTo(prev);

    // we can register the vector here, because base class ensures we are subscribed only at once place
    opp_string_map attributes = getStatisticAttributes();

    handle = ev.registerOutputVector(getComponent()->getFullPath().c_str(), getResultName().c_str());
    ASSERT(handle != NULL);
    for (opp_string_map::iterator it = attributes.begin(); it != attributes.end(); ++it){
        ev.setVectorAttribute(handle, it->first.c_str(), it->second.c_str());
        if(opp_strcmp(it->first.c_str(), "interval")==0){
            interval = SimTime::parse(it->second.c_str());
        }
    }
    if(interval<SimTime(0)){
        cComponent *comp = getComponent();
        do{
            if(comp->hasPar("interval")){
                interval = SimTime(comp->par("interval").doubleValue());
            }
        }while((comp=(cComponent*)comp->getParentModule()));
    }
    if(interval<SimTime(0)){
        interval = SimTime(1);
    }
}

void FloatingIntervalVectorRecorder::collect(simtime_t_cref t, double value){
    if (t < lastTime)
    {
        throw cRuntimeError("%s: Cannot record data with an earlier timestamp (t=%s) "
                            "than the previously recorded value (t=%s)",
                            getClassName(), SIMTIME_STR(t), SIMTIME_STR(lastTime));
    }

    //first add new value to interval, give hint for faster execution
    if(inInterval.size()>0){
        inInterval.insert(--inInterval.end(), std::pair<simtime_t, double>(t, value));
    }
    else{
        inInterval[t]=value;
    }
    //erase old values
    inInterval.erase(inInterval.begin(), inInterval.lower_bound((t-interval)));

    lastTime = t;
}

Register_ResultRecorder("floatingIntervalCountVector", FloatingIntervalCountVectorRecorder);

void FloatingIntervalCountVectorRecorder::collect(simtime_t_cref t, double value){
    FloatingIntervalVectorRecorder::collect(t, value);
    ev.recordInOutputVector(handle, t, inInterval.size());
}

Register_ResultRecorder("floatingIntervalSumVector", FloatingIntervalSumVectorRecorder);

void FloatingIntervalSumVectorRecorder::collect(simtime_t_cref t, double value){
    FloatingIntervalVectorRecorder::collect(t, value);
    double sumValue = 0;
    for(std::map<simtime_t, double>::iterator it= inInterval.begin(); it!=inInterval.end();++it){
        sumValue += (*it).second;
    }
    ev.recordInOutputVector(handle, t, sumValue);
}

Register_ResultRecorder("floatingIntervalAvgVector", FloatingIntervalAvgVectorRecorder);

void FloatingIntervalAvgVectorRecorder::collect(simtime_t_cref t, double value){
    FloatingIntervalVectorRecorder::collect(t, value);
    double sumValue = 0;
    for(std::map<simtime_t, double>::iterator it= inInterval.begin(); it!=inInterval.end();++it){
        sumValue += (*it).second;
    }
    ev.recordInOutputVector(handle, t, (sumValue/inInterval.size()));
}

Register_ResultRecorder("floatingIntervalMinVector", FloatingIntervalMinVectorRecorder);

void FloatingIntervalMinVectorRecorder::collect(simtime_t_cref t, double value){
    FloatingIntervalVectorRecorder::collect(t, value);
    double minValue = std::numeric_limits<double>::max();
    for(std::map<simtime_t, double>::iterator it= inInterval.begin(); it!=inInterval.end();++it){
        if((*it).second< minValue){
            minValue = (*it).second;
        }
    }
    ev.recordInOutputVector(handle, t, minValue);
}

Register_ResultRecorder("floatingIntervalMaxVector", FloatingIntervalMaxVectorRecorder);

void FloatingIntervalMaxVectorRecorder::collect(simtime_t_cref t, double value){
    FloatingIntervalVectorRecorder::collect(t, value);
    double maxValue = std::numeric_limits<double>::min();
    for(std::map<simtime_t, double>::iterator it= inInterval.begin(); it!=inInterval.end();++it){
        if((*it).second > maxValue){
            maxValue = (*it).second;
        }
    }
    ev.recordInOutputVector(handle, t, maxValue);
}

Register_ResultRecorder("floatingIntervalVarianceVector", FloatingIntervalVarianceVectorRecorder);

void FloatingIntervalVarianceVectorRecorder::collect(simtime_t_cref t, double value){
    FloatingIntervalVectorRecorder::collect(t, value);
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::min();
    for(std::map<simtime_t, double>::iterator it= inInterval.begin(); it!=inInterval.end();++it){
        if((*it).second< minValue){
            minValue = (*it).second;
        }
        if((*it).second > maxValue){
            maxValue = (*it).second;
        }
    }
    ev.recordInOutputVector(handle, t, (maxValue-minValue));
}
