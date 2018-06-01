# Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
# co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
# You may use, distribute and modify this code under the terms of the GPL v.3  license.


########################################################################
# Setup dependencies
########################################################################
find_package(Doxygen)

########################################################################
# Begin conditional configuration
########################################################################
if(ENABLE_DOXYGEN)

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(doxygen)

endif(ENABLE_DOXYGEN)
