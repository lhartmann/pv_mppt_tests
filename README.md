# pv_mppt_tests

A set of programs to test PV modules and MPPT techniques. This set of programs was developed as part of my PhD thesis, and was used mostly to evaluate the performance of model-based [MPP-Locus Accelerated Method (MLAM) MPPT technique](https://ieeexplore.ieee.org/document/6220896).

* `mppt`: Multi-purpose simulation/experiment controller:
  * Runs up to two PV generators for comparision:
    * Phisical generators wired to KEPCO BOP power supplies, cotrolled via SCPI serial.
    * Simulated generators based on stimuli and models.
  * Logs Isc and Voc, for environmental profiling and stimuli generation.
  * Can run multiple MPPT technique variatons on physical/simulated PV generators.
* `dat2mat`: Converts text-based data files to binary Matlab format, for size and speed improvements.
* `genstim`: Creates G and T profiles from measured Isc and Voc curves.
* `gentbl`: Creates error tables for validating MPPT techniques.
* `stim2sas`: Creates Voc,Isc,Vmp,Imp profiles for use with Keysight's Solar Array Simulator.

# Building

For the simulations you can use online IDEs, of which I recommend [gitpod](gitpod.io).

[![Open in Gitpod](https://gitpod.io/button/open-in-gitpod.svg)](https://gitpod.io#github.com/lhartmann/pv_mppt_tests)

* Click on the link above
* Open a terminal with CTRL+`
* Run:
```
mkdir build
cd build
cmake ..
make -j
```

You can obviously run locally on a linux box. Build should be easy as there are pretty much no external dependencies.

# Running

All programs are command line non-interactive, and docs are still missing. You can find the command line switches by reading the source (sorry), and looking for the args[] array. At least command line validation error messages should be useful.

# Potentially Useful Building Blocks

* PV Generator modelling con be found on `pvgen_*` files.
* An interface class is defined on `pvgen.h`, and can be used to refer to any models.
  * Single-cell model is on `pvgen_sc*`, and models uniform G and T.
  * Multi-cell (string) model is on `pvgen_mc*`, and accounts for partial shading.
* MPPT techniques are implemented on `mppt_*` files.
  * `mppt_inccond.h`: Classical Incremental Conductance MPPT. Slow, but the heuristic behavior ensures zero steady-state error.
  * `mppt_mlam.*`: MPP-Locus Accelerated Method. Fast, but being model-based it can not ensure zero steady-state error under most conditions.
  * `mppt_mlamhf.*`: MLAM+Heuristic Fusion. Combines MLAM and IncCond for fast and zero steady-state error, much like P-type and I-type controllers are combined to built a PI-type.
  * `mppt_temperature.h`: Open-loop temperature compensated voltage reference.
  * `mppt_temperaturehf.h`: Above+IncCond.

P.S.: The MLAM acronym matching our first names (Montiê, Lucas, Antonio and Maurício) is mere coincidence.
