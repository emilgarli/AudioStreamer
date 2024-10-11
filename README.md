# AudioStreamer
This project is a demo on a simple realtime stream of an audio file. The client AudioStreamer reads a sound file, plays it using PortAudio, and captures the realtime audio data to send it over a socket to a connected server.
The server AudioNode will then receive the audio data, and play it as it is being received.

The server is supposed to be able to receive several audio files of instruments of the same song at the same time, and sync the streams so that a finished song is playing. This has not yet been implemented.

# Dependencies

PortAudio
