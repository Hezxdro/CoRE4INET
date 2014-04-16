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

#include "Buffer.h"
#include "ApplicationBase.h"
#include "CoRE4INETDefs.h"

#include "HelperFunctions.h"

#include <ModuleAccess.h>

using namespace CoRE4INET;

Define_Module(Buffer);

simsignal_t Buffer::txPkSignal = SIMSIGNAL_NULL;
simsignal_t Buffer::rxPkSignal = SIMSIGNAL_NULL;

Buffer::~Buffer()
{
    destinationGates.clear();
}

int Buffer::numInitStages() const
{
    return 1;
}

void Buffer::initialize(int stage)
{
    if (stage == 0)
    {
        ev << "Initialize Buffer" << endl;
        initializeStatistics();
    }
}

void Buffer::initializeStatistics()
{
    txPkSignal = registerSignal("txPk");
    rxPkSignal = registerSignal("rxPk");
}

void Buffer::recordPacketSent(EtherFrame *frame)
{
    emit(txPkSignal, frame);
}

void Buffer::recordPacketReceived(EtherFrame *frame)
{
    emit(rxPkSignal, frame);
}

EtherFrame* Buffer::getFrame()
{
    return dequeue();
}

void Buffer::putFrame(EtherFrame* frame)
{
    enqueue(frame);
}

void Buffer::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("in"))
    {
        EtherFrame *frame = check_and_cast<EtherFrame *>(msg);
        recordPacketReceived(frame);

        if (frame->getByteLength() < MIN_ETHERNET_FRAME_BYTES)
        {
            frame->setByteLength(MIN_ETHERNET_FRAME_BYTES);  // "padding"
        }
        putFrame((EtherFrame*) frame);
    }
}

void Buffer::handleParameterChange(__attribute((unused)) const char* parname)
{
    destinationGates.clear();
    std::vector<std::string> destinationGatePaths =
            cStringTokenizer(par("destination_gates").stringValue(), DELIMITERS).asVector();
    for (std::vector<std::string>::iterator destinationGatePath = destinationGatePaths.begin();
            destinationGatePath != destinationGatePaths.end(); destinationGatePath++)
    {
        cGate* gate = gateByFullPath((*destinationGatePath));
        if (!gate)
        {
            gate = gateByShortPath((*destinationGatePath), this);
        }
        if (gate)
        {
            if (findContainingNode(gate->getOwnerModule()) != findContainingNode(this))
            {
                opp_error(
                        "Configuration problem of destination_gates: Gate: %s is not in node %s! Maybe a copy-paste problem?",
                        (*destinationGatePath).c_str(), findContainingNode(this)->getFullName());
            }
            destinationGates.push_back(gate);
        }
        else
        {
            opp_error("Configuration problem of destination_gates: Gate: %s could not be resolved!",
                    (*destinationGatePath).c_str());
        }
    }
}

void Buffer::enqueue(__attribute((unused))  EtherFrame *newFrame)
{
    ev << "Buffer::enqueue not implemented" << endl;
    throw;
}

EtherFrame * Buffer::dequeue()
{
    ev << "Buffer::dequeue not implemented" << endl;
    throw;
}
