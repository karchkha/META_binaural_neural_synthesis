# binaural-ns~

"binaural-ns~" is a MAX/MSP extension object that can load and run a PyTorch neural network from the Facebook research https://github.com/facebookresearch/BinauralSpeechSynthesis.
The object will take in the sound in real time and binaurize it through the neural network. This is the only experimental project (known to me) that runs binaural neural synthesis in real time.
However, the performance is not perfect and there may be some glitches in the audio due to the latency and slowness of the neural network computations. The object will run python underneath.
The Python server will run PyTorch and will support the GPU if your computer allows it. So the output will be fast enough using the GPU if it is available (and all drivers are properly installed) on the machine. The object takes the network name as an argument.
For the object to function, you need to have sample rate = 48000 and buffer size = 2048 in your MAX patch. Also, you need to have Python installed on your computer with some libraries like Pytorch and NumPy.

Please refer to the respective file for more details, depending on whether you want to test the MAX extension or build it (change it) in the C++ MAX SDK environment.

The project is extantion to: https://research.facebook.com/publications/neural-synthesis-of-binaural-speech-from-mono-audio/

Github: https://github.com/facebookresearch/BinauralSpeechSynthesis study.

See paper here: https://openreview.net/forum?id=uAX8q61EVRu

Video: https://www.youtube.com/watch?v=aWxmQKm_s8Q&ab_channel=TheTWIMLAIPodcastwithSamCharrington


