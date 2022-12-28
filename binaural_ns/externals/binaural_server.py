"""
If executing this script returns an 'Address already in use' error
make sure there are no processes running on the ports already.
To do that run 'sudo lsof -i:9997' 'sudo lsof -i:9998'
(9997 and 9998 are the default ports used here, so adjust accordingly
if using different ports) This commands brings up list of processes using these ports,
and gives their PID. For each process type, 'kill XXXX' where XXXX is PID.
"""


import argparse
from asyncio.windows_events import NULL
import sys
import pickle
import numpy as np
from sklearn.decomposition import sparse_encode
from pythonosc import dispatcher
from pythonosc import osc_server
from pythonosc import udp_client

from pythonosc import osc_bundle_builder
from pythonosc import osc_message_builder

#import multiprocessing

import os

import torch as th
from src.models import BinauralNetwork
from torchinfo import summary
import time


#np.set_printoptions(threshold=sys.maxsize)
#import random

x=[]

mono=th.from_numpy(np.zeros((1, 1, 3072)).astype(np.float32))
view=th.from_numpy(np.zeros((1, 7, 6)).astype(np.float32))
mono, view = mono.cuda(), view.cuda()


net = None
client_port=[]



def data_append(*args):
    #print (len(args[1:]))
    #print (args[1:])
    x.append(args[1:])
    client.send_message("/appended", True)
    #print(".")




def predict(*args):
    global net
    global x

    try:  #### if import fails we just send last existinbg data

        #start_time = time.time()

        new_input = th.FloatTensor(x)

        #print(new_input[:,2048:])

        mono[:,:,:1024] = mono [:,:,2048:]
        mono[:,:, 1024:] = new_input[:,:2048]

        view[:, :, :2] = view [:,:,4:]
        new_pos = new_input[:,2048:].reshape(4,7) #.transpose()
        new_pos = th.transpose(new_pos, 0, 1)
        view [:,:,2:] = new_pos

        with th.no_grad():
            binaural = net(mono, view)["output"].squeeze(0)
            binaural = binaural[:,1024:]

            flatten_prediction = binaural.flatten().cpu().numpy().astype(np.float64)

            #print(flatten_prediction.shape)

    except:
        print("\nError while importing data\n") 

    #############################################################
    ############  Sendig audio back with bundles  ###############
    #############################################################

    for i in range(0,4096,512):

        bundle = osc_bundle_builder.OscBundleBuilder(
        osc_bundle_builder.IMMEDIATELY)
        msg = osc_message_builder.OscMessageBuilder(address="/prediction")
        
        for k in range(512):
            prediction_for_sending=flatten_prediction[i+k]
            #print (prediction_for_sending)
                 
            msg.add_arg(prediction_for_sending)

        bundle.add_content(msg.build())
     
        bundle = bundle.build()

        client.send(bundle)
            
    x.clear()
    

def load_network(*args):
    global net

    net = BinauralNetwork(view_dim=7,
                    warpnet_layers=4,
                    warpnet_channels=64,
                    wavenet_blocks=1,
                    layers_per_block=10,
                    wavenet_channels=64
                    )
    net.load_from_file(args[1:][0])
    net.eval().cuda()
    net(mono, view)["output"]
    print(summary(net, input_data=[mono, view], verbose=0, depth=2))



def clear_x(*args):
    global x
    x=[]






######################### SERVER #############################

if __name__ == "__main__":
    import warnings
    #multiprocessing.freeze_support()   
    warnings.filterwarnings("ignore")
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip",
        default="127.0.0.1", help="The ip to listen on")
    parser.add_argument("--serverport",
        type=int, default=7000, help="The port for server listen on")
    parser.add_argument("--clientport",
        type=int, default=8000, help="The client port")
 
    args = parser.parse_args()
    
    

    dispatcher = dispatcher.Dispatcher()


    dispatcher.map("/data", data_append)
    dispatcher.map("/predict", predict)
    dispatcher.map("/filename", load_network)
    dispatcher.map("/clear", clear_x)

    

    ### server
    server = osc_server.ThreadingOSCUDPServer(
        (args.ip, args.serverport), dispatcher)
    server.max_packet_size = 32768 #24576

    ### client
    client_port=str(args.clientport)
    client = udp_client.SimpleUDPClient(args.ip, args.clientport)
    
    client.send_message("/ready", True)

    print("Serving on {}".format(server.server_address))
    print("\n",th.cuda.get_device_properties(0),"\n")

    #print("Ctrl+C to quit")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        while True:
            pass
        #except KeyboardInterrupt:
        #    pass
        #sys.exit()








