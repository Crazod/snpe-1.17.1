#!/bin/bash
#==============================================================================
#  @@-COPYRIGHT-START-@@
#
#  Copyright 2016 Qualcomm Technologies, Inc. All rights reserved.
#  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#
#  The party receiving this software directly from QTI (the "Recipient")
#  may use this software as reasonably necessary solely for the purposes
#  set forth in the agreement between the Recipient and QTI (the
#  "Agreement"). The software may be used in source code form solely by
#  the Recipient's employees (if any) authorized by the Agreement. Unless
#  expressly authorized in the Agreement, the Recipient may not sublicense,
#  assign, transfer or otherwise provide the source code to any third
#  party. Qualcomm Technologies, Inc. retains all ownership rights in and
#  to the software
#
#  This notice supersedes any other QTI notices contained within the software
#  except copyright notices indicating different years of publication for
#  different portions of the software. This notice does not supersede the
#  application of any third party copyright notice to that third party's
#  code.
#
#  @@-COPYRIGHT-END-@@
#==============================================================================

# This script checks if the python dependencies are met

PYV=`python -c "import sys;t='{v[0]}.{v[1]}'.format(v=list(sys.version_info[:2]));sys.stdout.write(t)";`
if [ $PYV = '2.7' ]; then
    echo Supported version of Python found: $PYV
else
    echo Supported version of Python is 2.7. Found instead:  $PYV
fi

# Check if there are multiple versions of python modules and warn user
#Dependencies that are needed for running snpe
needed_depends=()
needed_depends+=('python-numpy')
needed_depends+=('python-sphinx')
needed_depends+=('python-scipy')
needed_depends+=('python-matplotlib')
needed_depends+=('python-skimage')
needed_depends+=('python-protobuf')
needed_depends+=('python-yaml')

#Dependencies that are needed for running snpe
needed_depends_pip=()
needed_depends_pip+=('numpy')
needed_depends_pip+=('sphinx')
needed_depends_pip+=('scipy')
needed_depends_pip+=('matplotlib')
needed_depends_pip+=('scikit-image')
needed_depends_pip+=('protobuf')
needed_depends_pip+=('pyyaml')

#Unmet dependencies
need_to_install=()

#Check if pip is installed
PIP_INSTALLED=false
if type pip &> /dev/null; then
    PIP_INSTALLED=true
else
    PIP_INSTALLED=false
fi

i=0
while [ $i -lt ${#needed_depends[*]} ]; do
  PKG_INSTALLED=$(dpkg-query -W --showformat='${Status}\n' ${needed_depends[$i]}|grep "install ok installed")
  echo "Checking for ${needed_depends[$i]}: $PKG_INSTALLED"
  if [ "$PKG_INSTALLED" != "" ]; then
      if [ "$PIP_INSTALLED" = "true" ]; then
          pip_version_str=$(pip show ${needed_depends_pip[$i]} | grep "Version")
          if [[ ! -z "$pip_version_str" ]]; then
              echo "WARNING: It appears the python module ${needed_depends_pip[$i]} is installed on this system using the apt-get distribution as well as pip. If you encounter errors, please use only one distribution."
          fi
      fi
      echo "==========================================="
  fi
  i=$(( $i +1));
done

