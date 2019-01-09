## Preinstall

On ubuntu these are the packages that need to be installed to compile

	sudo apt-get install cmake libx11-dev x11proto-core-dev x11proto-gl-dev x11proto-kb-dev libpng12-dev zlib1g-dev x11proto-xf86vidmode-dev libxxf86vm-dev libxau-dev libxext-dev libxi-dev x11proto-input-dev x11proto-xext-dev libjpeg62-dev libssl1.0-dev

## Installation Instructions

PseuWoW installation uses CMake. Please create a new directory for the build and create Build Files appropriate for your system using cmake or cmake-gui. If you prefer to use the command line, this is an example of the process, starting in the PseuWoW root directory:

    mkdir build
    cd build
    cmake ..
    make
    make install

To build stuffextract, as well as the model viewer, define BUILD_TOOLS in cmake by appending -DBUILD_TOOLS=1 to the command line.
To build a Debug version of PseuWoW, define DEBUG in cmake by appending -DDEBUG=1 to the command line

After installation edit the config files in bin/conf and rename them to PseuWoW.conf, gui.conf, ScriptConfig.conf and users.conf.
If UseMPQ in PseuWoW.conf is set, create a link/copy to your "TheGame" **Data** folder in bin. Otherwise use stuffextract with the appropriate parameters to extract data from "TheGame" archives and copy it to bin/data

To run PseuWoW please execute

    ./pseuwow

The model viewer is strictly a debugging tool. It is not user friendly. The viewer expects to find MPQ files in ./Data and uses models and textures from MPQ files. The viewer cannot browse MPQ files.