# sslmflowpath (sslmfp)

This projects is a suit of tools for extracting and analysis of source sink location model.



Contact: qingyufeng@outlook.com





## Build with Visual Studio on windows

Download the folder and compile directly. The instructions on how to setup dependencies are described in the Dependencies section




## Build on Linux

Using Cmake
cd src && mkdir build && cd build
cmake ..
make && make install
The executables are written to /usr/local/sslmfp directory.  This can be changed at the second last line (following DESTINATION) if desired.



Dependencies
------------
### Dependencies include GDAL, MPI and C++ 2011.

#### On Windows

1. Include GDAL GDAL headers and lib should be put somewhere.
    1. They can be downloaded from http://www.gisinternals.com/Include 
2. mpi headers and bin
    1. Downloadable from: https://www.microsoft.com/en-us/download/details.aspx?id=52981Include 
3. the header folder in Project -> Properties -> C/C++ -> AdditionalIncludeDirectories.Example: C:\GDAL\include;C:\Program Files (x86)\Microsoft SDKs\MPI\Include;%(AdditionalIncludeDirectories)
4. Include the lib folder in Project -> Properties -> LINKER -> Additional Library Directories:Example: C:\GDAL\lib;C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64;%(AdditionalLibraryDirectories)
5. Include the library in name under Project -> Properties -> Linker-> Additional Dependencies:Example: gdal_i.lib;msmpi.lib;%(AdditionalDependencies)

#### On Linux  

I've added a DependentSetupGDALMPICH2.sh to help, which has been tested on Ubuntu 1804 and 2004.  Ubuntu 1804 can install mpi with : sudo apt install openmpi 

Testing
-------
See the repository https://github.com/QingyuFeng/sslmflowpath/tree/master/examplews for test data and scripts that exercise every function.  These can also serve as examples for using some of the functions.



## Acknowledgement

This program used the outputs and code structure of TauDEM, which is available from https://github.com/dtarb/TauDEM), and generate extra outputs.

In order to write the output into json, the Rapidjson library was used, which is available from https://rapidjson.org/. 

Thanks for the contributors！

