## Requirements ##
- ROOT6
- C++17 compiler
- CMake >= 2.8 (lx-cluster: cmake28 executable)
- Boost

## Build ##
    mkdir build; cd build
    cmake ..
    make

On lx-cluster:

- use `CMSSW_10_5_X`
- use `cmake28` instead of `cmake`

## Run ##
Execute `run.x` (created in the build directory).
Meaningful modules can be chosen as arguments:

    $ ./run.x --help
    Running on year 2018 (to change set ANALYSIS_YEAR_CONFIG variable)
    Usage: ./run.x module1[ module2[...]] [options]
    Allowed options:
      -h [ --help ]                         produce help message
      -f [ --fraction ] arg (=1)            Fraction of events to process
      --mc_dataset arg                      Single MC dataset to be processed, if 
                                            empty datasets are taken from 
                                            config.ini
      --data_dataset arg                    Single data dataset to be processed, if
                                            empty datasets are taken from 
                                            config.ini
      --signal_dataset arg                  Single signal dataset to be processed, 
                                            if empty datasets are taken from 
                                            config.ini
      --fileNR arg (=0)                     FileNR to be processed, if zero all 
                                            files are processed (filelist defined 
                                            in .ini)
      -s [ --systematic ] arg (=Nominal)    Systematic uncertainty to be applied 
                                            (only for distributions)
      -d [ --dataBasePath ] arg (=/net/data_cms1b/user/dmeuser/top_analysis/2018/v06/nTuple/)
                                            Alternative dataBasePath, which can be 
                                            used for condor submission with 
                                            inputFileTransfer
      --release                             Release mode (don't draw version 
                                            labels)


## Output ##
Plots are stored as canvases in a root file.

## Adding new modules ##
To add a new module, create a new `<module>.cpp` file in `src/modules`.
The entry function has to be `extern "C" void run() {...}`.
Add `<module>` to the list in `src/modules/CMakeLists.txt`

## Running on condor ##
The distributions module can be executed in multiple condor jobs.
One job corresponds to one sample (defined in `config.ini`).
The scripts for submitting the jobs can be found in `multiprocess/`.
Attention: The output hists are saved in an additional folder `output/multiHists/`
and have to be combined before plotting.  
...
