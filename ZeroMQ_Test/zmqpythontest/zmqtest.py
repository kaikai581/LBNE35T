import zmq

context = zmq.Context()

socket = context.socket(zmq.PUSH)

socket.connect("tcp://lbne35t-gateway01.fnal.gov:5000")

socket.send_json({"type": "moni", "service": "test_service", "varname": "example_variable", "value": 100100}) 
