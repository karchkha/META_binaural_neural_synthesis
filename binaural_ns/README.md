# binaural-ns~

"binaural-ns~" is a MAX/MSP extension object that can (almost) run binaural neural synthesis in real time.

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

!!! It's for Windows only !!!
For the instalation you treat this file as any other MAX package. Just place folder "binaural_ns" with all its subfolders into directory: C:\Users\username\Documents\Max 8\Packages\
After this binaural_ns~ object must be seen form any MAX patch. Please see binaural_ns~ help patch to see it in action.

#### Python and its libraries

Please make sure you have python installed on your computer. If not please downoad and install latest version of python from: https://www.python.org/downloads/
You will need to install above mentioned libriries in Python. 

#### GPU support

If your GPU is cuda capable and you have your pytorch installed with appropriate cuda verision, python server will utilise GPU power. Otherwise object won't be able to reproduce sensable sound!

## Getting Started

Please download the repositoy and place binaural_ns file in your MAX extentions directory. Then download pretrained network binaural_network_1block.net from the link https://github.com/facebookresearch/BinauralSpeechSynthesis/releases/tag/v1.1 and place it in the binaural_ns/externals folder.

The binaural_ns~  help file demonstrates the use case for the object. With Spat5 control panel user is able to control direction of the object. Put desiered audio files in the player or use microphone to binauralise your voice.

Obvously you need to use headphones to hear the effect fully.

If everything is installed correctly the response to “server 1” message binaural_ns~ must:

* Start command prompt window running python server dysplaying network summary.
* should display the CPU and GPU availability of the hosting machine. 
* The binaural_ns~ must load network “binaural_network_1block.net” that is given as arguments.
* MAX patch must give binauralised stereo sound.

Additionally, you can try out “gpu_change,” “verbose,” and “clean” messages. A detailed explanation of every parameter and functionally is provided in the reference window.

Please feel free to copy, change and adapt binaural_ns~ and help patch to your need!

Good luck!!

### Credits

Tornike karchkhadze 
UC San Diego
03.17.2022






