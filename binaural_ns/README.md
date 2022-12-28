# binaural-ns~

"binaural-ns~" is a MAX/MSP extension object that can load and run a PyTorch neural network from the Facebook research https://github.com/facebookresearch/BinauralSpeechSynthesis.
The object will take in the sound in real time and binaurize it through the neural network. This is the only experimental project (known to me) that runs binaural neural synthesis in real time.
However, the performance is not perfect and there may be some glitches in the audio due to the latency and slowness of the neural network computations. The object will run python underneath.
The Python server will run PyTorch and will support the GPU if your computer allows it. So the output will be fast enough using the GPU if it is available (and all drivers are properly installed) on the machine. The object takes the network name as an argument.
For the object to function, you need to have sample rate = 48000 and buffer size = 2048 in your MAX patch. Also, you need to have Python installed on your computer with some libraries like Pytorch and NumPy.


## Requirements

* Max 8
	* spat5
* Python 3.7 or later 
	* argparse
	* numpy==1.23.4
	* scikit-learn==1.1.3
	* python-osc==1.8.0
	* torch==1.13.0+cu116
	* torchinfo==1.6.0

## Installation

For the instalation you treat this file as any other MAX package. Just place folder "binaural_ns" with all its subfolders into directory: C:\Users\username\Documents\Max 8\Packages\
After this binaural_ns~ object must be seen form any MAX patch. Please see binaural_ns~ help patch to see it in action.

#### Python and its libraries

Please make sure you have python installed on your computer. If not please downoad and install latest version of python from: https://www.python.org/downloads/
You will need to install above mentioned libriries in Python. 

#### GPU support

If your GPU is cuda capable and you have your pytorch installed with appropriate cuda verision, python server will utilise GPU power. Otherwise object won't be able to reproduce sensable sound. 

## Getting Started

Please download pretrained network binaural_network_1block.net from the link https://github.com/facebookresearch/BinauralSpeechSynthesis/releases/tag/v1.1 and place it in the externals folder.

The binaural_ns~  help file demonstrates the use case for the object. With Spat5 control panel you user is able to control directiosn of the object. Put desiered audio files in the player or use microphone to binauralise your voice. 
Obvously you need to use headphones to hear the effect fully.

If everything is installed correctly in response to “server 1” message binaural_ns~ must:

* Start command prompt window running python server dysplaying network summary.
* It should display the CPU and GPU availability of the hosting machine. 
* The binaural_ns~ must load network “binaural_network_1block.net” that is given as arguments.


Additionally, you can try out “gpu_change,” “verbose,” and “clean” messages. A detailed explanation of every parameter and functionally is provided in the reference window.

Please go ahead copy and adapt binaural_ns~ help patch to your own machine learning inference project!

Good luck!!

### Credits

Tornike karchkhadze 
UC San Diego
03.17.2022






