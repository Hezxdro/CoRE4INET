#ifndef __TTE4INET_RCTRAFFICCONDITIONER_H
#define __TTE4INET_RCTRAFFICCONDITIONER_H

#include <ModuleAccess.h>
#include <RCBuffer.h>
#include <PCFrame_m.h>
#include <TTEScheduler.h>
#include <HelperFunctions.h>

namespace TTEthernetModel {

/**
 * @brief A TrafficConditioner for RCMessages.
 *
 * The RCTrafficConditioner only allows lower priorities to transmit when there are no
 * RC messages queued.
 *
 */
template <class TC>
class RCTrafficConditioner : public TC
{
    public:
        /**
         * @brief Constructor
         */
        RCTrafficConditioner();
        /**
         * @brief Destructor
         */
        ~RCTrafficConditioner();

    private:
        /**
         * @brief Dedicated queue for each priority of rate-constrained messages
         */
        cQueue rcQueue[NUM_RC_PRIORITIES];
    protected:
        /**
         * @brief Initialization of the module
         */
        virtual void initialize();


        /**
         * @brief Forwards the messages from the different buffers and LLC
         * according to the specification for RCMessages.
         *
         * Rate-constrained messages are send immediately, lower priority frames are queued
         * as long as there are rate-constrained messages waiting.
         * If the mac layer is idle, messages are picked from the queues according
         * to the priorities, using the template class.
         *
         * @param msg the incoming message
         */
        virtual void handleMessage(cMessage *msg);

        /**
         * @brief Queues messages in the correct queue
         *
         * Rate-constrained messages are queued in this module, other messages are forwarded to the
         * template classes enqueueMessage method
         *
         * @param msg the incoming message
         */
        virtual void enqueueMessage(cMessage *msg);

        /**
        * @brief this method is invoked when the underlying mac is idle.
        *
        * When this method is invoked the module sends a new message when there is
        * one. Else it saves the state and sends the message immediately when it is
        * received.
        *
        * @param msg the message to be queued
        */
        virtual void requestPacket();

        /**
         * @brief Returns true when there are no pending messages.
         *
         * @return true if all queues are empty.
         */
        virtual bool isEmpty();

        /**
         * @brief Clears all queued packets and stored requests.
         */
        virtual void clear();

        /**
         * @brief Returns a frame directly from the queues, bypassing the primary,
         * send-on-request mechanism. Returns NULL if the queue is empty.
         *
         * @return the message with the highest priority from any queue. NULL if the
         * queues are empty or cannot send due to the traffic policies.
         */
        virtual cMessage *pop();

        /**
         * @brief Returns a pointer to a frame directly from the queues.
         *
         * front must return a pointer to the same message pop() would return.
         *
         * @return pointer to the message with the highest priority from any queue. NULL if the
         * queues are empty
         */
        virtual cMessage *front();
};

template <class TC>
RCTrafficConditioner<TC>::RCTrafficConditioner(){
    for (unsigned int i = 0; i < NUM_RC_PRIORITIES; i++)
    {
        char strBuf[64];
        snprintf(strBuf,64,"RC Priority %d Messages", i);
        rcQueue[i].setName(strBuf);
    }
}

template <class TC>
RCTrafficConditioner<TC>::~RCTrafficConditioner(){
}

template <class TC>
void RCTrafficConditioner<TC>::initialize()
{
    TC::initialize();
}

template <class TC>
void RCTrafficConditioner<TC>::handleMessage(cMessage *msg)
{
    //Frames arrived on in are rate-constreind frames
    if (msg->arrivedOn("RCin"))
    {
        if (TC::getNumPendingRequests())
        {
            //Reset Bag
            RCBuffer *rcBuffer = dynamic_cast<RCBuffer*> (msg->getSenderModule());
            if (rcBuffer)
                rcBuffer->resetBag();
            //Set Transparent clock when frame is PCF
            PCFrame *pcf = dynamic_cast<PCFrame*> (msg);
            if(pcf){
                setTransparentClock(pcf, cModule::getParentModule()->par("static_tx_delay").doubleValue(), (TTEScheduler*)cModule::getParentModule()->getParentModule()->getSubmodule("tteScheduler"));
            }
            TC::framesRequested--;
            cSimpleModule::send(msg, cModule::gateBaseId("out"));
        }
        else
        {
            enqueueMessage(msg);
        }
    }
    else{
        if(TC::getNumPendingRequests()){
            TC::handleMessage(msg);
        }
        else{
            TC::enqueueMessage(msg);
        }
    }
}

template <class TC>
void RCTrafficConditioner<TC>::enqueueMessage(cMessage *msg){
    if(msg->arrivedOn("RCin")){
       int priority = msg->getSenderModule()->par("priority").longValue();
       if (priority > 0 && priority < NUM_RC_PRIORITIES)
       {
           rcQueue[priority].insert(msg);
           TC::notifyListeners();
       }
       else
       {
           rcQueue[0].insert(msg);
           TC::notifyListeners();
           ev << "Priority missing!" << endl;
       }
    }
    else{
        TC::enqueueMessage(msg);
    }
}

template <class TC>
void RCTrafficConditioner<TC>::requestPacket()
{
    Enter_Method("requestPacket()");
    //Feed the MAC layer with the next frame
    TC::framesRequested++;

    if (cMessage *msg = pop())
    {
        TC::framesRequested--;
        cSimpleModule::send(msg, cModule::gateBaseId("out"));
    }
}

template <class TC>
cMessage* RCTrafficConditioner<TC>::pop()
{
    Enter_Method("pop()");
    //RCFrames
    for (unsigned int i = 0; i < NUM_RC_PRIORITIES; i++)
    {
        if (!rcQueue[i].isEmpty())
        {
            EtherFrame *message = (EtherFrame*) rcQueue[i].pop();
            //Reset Bag
            RCBuffer *rcBuffer = dynamic_cast<RCBuffer*> (message->getSenderModule());
            if (rcBuffer)
                rcBuffer->resetBag();

            PCFrame *pcf = dynamic_cast<PCFrame*> (message);
            if(pcf){
                setTransparentClock(pcf, cModule::getParentModule()->par("static_tx_delay").doubleValue(), (TTEScheduler*)cModule::getParentModule()->getParentModule()->getSubmodule("tteScheduler"));
            }
            return message;
        }
    }
    return TC::pop();
}

template <class TC>
cMessage* RCTrafficConditioner<TC>::front()
{
    Enter_Method("front()");
    //RCFrames
    for (unsigned int i = 0; i < NUM_RC_PRIORITIES; i++)
    {
        if (!rcQueue[i].isEmpty())
        {
            EtherFrame *message = (EtherFrame*) rcQueue[i].front();
            return message;
        }
    }
    return TC::front();
}

template <class TC>
bool RCTrafficConditioner<TC>::isEmpty()
{
    bool empty = true;
    for (unsigned int i = 0; i < NUM_RC_PRIORITIES; i++)
    {
        empty &= rcQueue[i].isEmpty();
    }
    empty &= TC::isEmpty();
    return empty;
}

template <class TC>
void RCTrafficConditioner<TC>::clear()
{
    TC::clear();
    for (unsigned int i = 0; i < NUM_RC_PRIORITIES; i++)
    {
        rcQueue[i].clear();
    }
}

}

#endif

