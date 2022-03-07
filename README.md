# telem_radio
Amateur radio telemetry modulator and radio control program

To build telem_radio clone this git repository to a local machine.  
telem_radio uses the jack audio server.  Install jack with apt or the equivalent on you machine:
sudo apt install jackd
sudo apt install libjack-dev

cd to the Debug or Release directories.  
Type make all to build

You should then be able to run telem-radio -h and it will print usage.

cd to the env directory and start the jack server with the start scriopt.  Then run telem_radio from either the Debug or Release
directories.  It should print the name and version, then indicate that the audio loop is running.  Audio should be copied from the
microphone to the speaker of the default sound card.  Type h to see a list of commands or s to see the status of the filters and
telemetry.

