#! /usr/bin/env python3

# IMPORTANT: remember to add the following paths to PYTHON_PATH before running:
#   - SUNBEAM_PATH : $INSTALL_DIR/opm-common/build/python
#   - SIMULATORS_PATH : $INSTALL_DIR/opm-simulators/build/python

import os
import time
import sys

try:
    import sunbeam
except ImportError:
    print( "Could not find 'sunbeam'. "
           "Please add the path to the sunbeam module to PYTHON_PATH. "
           "Typically something like: $OPM_INSTALL_DIR/opm-common/build/python" )
    sys.exit(1)

try:
    from simulators import simulators
except ImportError:
    print( "Could not find 'simulators'. "
           "Please add the path to the simulators module to PYTHON_PATH. "
           "Typically something like: $OPM_INSTALL_DIR/opm-simulators/build/python" )
    sys.exit(1)

p = simulators.BlackOilSimulator()

# For testing purposes, we use a simplified data set based on
#   https://github.com/OPM/opm-data/blob/master/spe1/SPE1CASE1.DATA
norne = sunbeam.parse(
    'SPE1CASE1.DATA', recovery=[('PARSE_RANDOM_SLASH', sunbeam.action.ignore)])
p.setEclipseState(norne._state())
p.setDeck(norne._deck())
p.setSchedule(norne._schedule())
p.setSummaryConfig(norne._summary_config())
p.step_init()

#Simple loop to test the step() mehtod:
done = False
while not done:
    p.step()
    result = input("Press Enter to continue...")
    if len(result) > 0:
        done = True
p.step_cleanup()

# The following code does not work yet..
#   (not able to reset to start of the simulation by calling step_init() for a
#    second time)
p.step_init()
done = False
while not done:
    p.step()
    result = input("Press Enter to continue...")
    if len(result) > 0:
        done = True
p.step_cleanup()

# The original run() method should still work,
#  currently it only works if it is run alone (without using the step() methods)
#p.run()
