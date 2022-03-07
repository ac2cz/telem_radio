# telem_radio
Amateur radio telemetry modulator and radio control program

To build telem_radio clone this git repository to a local machine.  
telem_radio uses the jack audio server.  Install jack with apt or the equivalent on your machine:

sudo apt install jackd<br>
sudo apt install libjack-dev

cd to the Debug or Release directories and type:  
make all

This should build the executable.  You should then be able to run telem-radio -h and it will print usage.  But it will not process audio without jackd, the jack audio server.

cd to the env directory and start the jack server with the start script.  Then run telem_radio from either the Debug or Release
directories.  It should print something like this:
TELEM Radio Platform
Type (q)uit to exit, or (h)help..

Audio should now be copied from the microphone to the speaker of the default sound card.  Type h to see a list of commands or s to see the status of the filters and
telemetry.

If the audio is too quiet or too loud then run alsamixer to change the levels.
