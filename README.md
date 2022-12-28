# binaural-ns~

binaural-ns~ is a MAX/MSP extension object that can load and run a PyTorch neural network from the well known Facebook research (almost) in real time.
The object will take in the sound and binaurize it through the neural network. This is the only experimental project (known to me) that tries to runs binaural neural synthesis in real time. The performance is not perfect and there may be some glitches in the audio due to the latency and slowness of the neural network computations. 

The binaural-ns~ object will run python underneath. The Python server will run PyTorch and will support the GPU if your computer allows it and all drivers are properly installed. The audio output will be only fast enough when using the GPU on the machine. 

For the object to function, you need to have sample rate = 48000 and buffer size = 2048 in your MAX patch. 

Please refer to the respective file for more details, depending on whether you want to test the MAX extension in MAX?MSP or build it (change it) in the C++ MAX SDK environment.

Binaural neural synthesis study: https://research.facebook.com/publications/neural-synthesis-of-binaural-speech-from-mono-audio/

Github: https://github.com/facebookresearch/BinauralSpeechSynthesis 

See paper here: https://openreview.net/forum?id=uAX8q61EVRu

Video: https://www.youtube.com/watch?v=aWxmQKm_s8Q&ab_channel=TheTWIMLAIPodcastwithSamCharrington


