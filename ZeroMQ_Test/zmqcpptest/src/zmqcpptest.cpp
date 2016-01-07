#include <zmq.hpp>

int main (int argc, char *argv[])
{
  zmq::context_t context(1);
  
  zmq::socket_t  sender(context, ZMQ_PUSH);
  sender.connect("tcp://lbne35t-gateway01.fnal.gov:5000");
  
  return 0;
}
