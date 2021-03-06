#!/bin/bash

#
# Installs score-P on $HOME.
#
#
#

# todo: add --help
# todo: add --installed-cleanall (rm -rf $HOME/cube $HOME/opari2 $HOME/openmpi $HOME/scorep)
# todo: add --temp-cleanall (rm -rf $HOME/tmp/cube $HOME/tmp/opari2 $HOME/tmp/openmpi $HOME/tmp/scorep)


# change your config here
#
function configuration {

    #
    # CONFIGURATION. Change as desired.
    #

    temporaldirectory_prefix="$HOME/tmp/"
    installdirectory_prefix="$HOME/"

    # let number_of_apps++
    # array_of_apps[$number_of_apps,1]="OpenMPI" # Name of application
    # array_of_apps[$number_of_apps,2]="http://www.open-mpi.de/software/ompi/v1.10/downloads/openmpi-1.10.0.tar.gz" # Download url
    # array_of_apps[$number_of_apps,3]="--enable-mpirun-prefix-by-default" # ./config script's parameters # CC=$cc CXX=$cxx
    # array_of_apps[$number_of_apps,4]="no dependencies." # notes about the app. For user information
    # get_shortfilename ${array_of_apps[$number_of_apps,2]} shortname
    # add_to_path ${installdirectory_prefix}$shortname
    # confirm_install $number_of_apps number_of_apps

    let number_of_apps++
    array_of_apps[$number_of_apps,1]="Opari2" # Name of application
    array_of_apps[$number_of_apps,2]="http://www.vi-hps.org/upload/packages/opari2/opari2-1.1.2.tar.gz" # Download url
    array_of_apps[$number_of_apps,3]="" # ./config script's parameters # CC=$cc CXX=$cxx
    array_of_apps[$number_of_apps,4]="no dependencies." # notes about the app. For user information
    get_shortfilename ${array_of_apps[$number_of_apps,2]} shortname
    add_to_path ${installdirectory_prefix}$shortname
    opari_installdirectory=${installdirectory_prefix}$shortname
    confirm_install $number_of_apps number_of_apps

    let number_of_apps++
    array_of_apps[$number_of_apps,1]="Cube" # Name of application
    array_of_apps[$number_of_apps,2]="http://apps.fz-juelich.de/scalasca/releases/cube/4.3/dist/cube-4.3.2.tar.gz" # Download url
    array_of_apps[$number_of_apps,3]="--without-gui --without-plugin-example --without-java-reader" # ./config script's parameters #
    array_of_apps[$number_of_apps,4]="no dependencies." # notes about the app. For user information
    get_shortfilename ${array_of_apps[$number_of_apps,2]} shortname
    add_to_path ${installdirectory_prefix}$shortname
    cube_installdirectory=${installdirectory_prefix}$shortname
    confirm_install $number_of_apps number_of_apps

    let number_of_apps++
    array_of_apps[$number_of_apps,1]="Score-P" # Name of application
    array_of_apps[$number_of_apps,2]="http://www.vi-hps.org/upload/packages/scorep/scorep-1.4.1.tar.gz" # Download url
    array_of_apps[$number_of_apps,3]="--with-mpi=openmpi --with-cube=${cube_installdirectory}/bin --with-opari2=${opari_installdirectory}/bin --with-libcudart=$cuda_dir --enable-mpi --enable-cuda" # ./config script's parameters
    array_of_apps[$number_of_apps,4]="this built requires openmpi and Cuda. dependencies: opari2 and cube. \nIMPORTANT: due to an error in v1.4.1 you must add \"#include <iomanip>\" at the begining of ${temporaldirectory_prefix}scorep/src/tools/score/SCOREP_Score_Estimator.cpp" # notes about the app. For user information
    get_shortfilename ${array_of_apps[$number_of_apps,2]} shortname
    add_to_path ${installdirectory_prefix}$shortname
    scorep_installdirectory=${installdirectory_prefix}$shortname
    confirm_install $number_of_apps number_of_apps

    let number_of_apps++
    array_of_apps[$number_of_apps,1]="Scalasca" # Name of application
    array_of_apps[$number_of_apps,2]="http://apps.fz-juelich.de/scalasca/releases/scalasca/2.2/dist/scalasca-2.2.2.tar.gz" # Download url
    array_of_apps[$number_of_apps,3]="--with-cube=${cube_installdirectory}/bin --with-mpi=openmpi --with-otf2=${scorep_installdirectory}/bin" # ./config script's parameters
    array_of_apps[$number_of_apps,4]="this built require openmpi and Cuda. dependencies: score-p." # notes about the app. For user information
    add_to_path ${installdirectory_prefix}$shortname
    confirm_install $number_of_apps number_of_apps

    #
    # Add more apps here...
    #
}

# banners stuff
#
function banner {
    local text="$1"
    echo
    echo "============================================================================"
    echo "== $text"
    echo "============================================================================"
}

function section_banner {
    local text="$1"
    echo
    echo "--- $text"
    sleep 3
}

function get_bits {
  system="`uname -m`"
  system=${system##*_}
  if [ "x$system" != "x32" ] && [ "x$system" != "x64" ] && [ "x$system" != "x128" ]; then
    exit_error 1 "Could not autodetect the number of bits in the architecture of the system. Set this manually in script variable \$bits"
  fi
  eval "$1='$system'"
}

# Enforces makedir.
#
function makedir {
    local dir="$1"
    if [ ! -d $dir ]; then
      mkdir -p $dir
      exit_error $? "Cannot create directory $dir"
    fi
}

# pre download test. check if url return a proper header. use preferably curl command if available
#
function test_url {
    local url="$1"
    # output=`curl -s --head $url | head -n 1 | grep "HTTP/1.[01] [23].." > /dev/null`
    output="`wget -S --spider $url  2>&1 | grep 'HTTP/1.1 200 OK'`"
    if [ "x$output" = "x" ];then
      exit_error $? "Cannot access file $url . check that the url is correct."
    fi
}

# downloads and check file. use preferably curl command if available
#
function download {
    local url="$1"
    local file="$2"
    # curl $url --output $file -o nul -#
    wget $url -O $file
    if [ ! -f $file ]; then
      exit_error 1 "Error downloading $file. Is the url valid?"
      exit 1
    fi
    header=`head -20 $file | grep -i "html"`
    if [ "x$header" != "x" ]; then
      exit_error 1 "Downloaded file $file is invalid. Is the url valid?"
      exit 1
    fi
}

# error output & exit
#
function exit_error {
    local error=$1
    local msg="error:: $2"
    if [ "x$msg" = "x" ]; then
      msg="Error detected. Quitting..."
    fi
    if [ $error -ne 0 ]; then
      echo ""
      echo $msg
      exit 1
    fi
}

# let user review error
#
function review_error {
    local error=$1
    local msg="error:: $2"

    if [ $yestoall ];then
      return
    fi
    if [ "x$msg" = "x" ]; then
      msg="Error detected. Quitting..."
    fi
    if [ $error -ne 0 ]; then
      echo ""
      echo $msg
      echo -n "Continue installation? [Y/n] "
      read yesno < /dev/tty
      if [ "x$yesno" = "xn" ] || [ "x$yesno" = "xN" ];then
        exit 1
      fi
    fi
}

# text convert f(). e.g: from "http://user:pass@www.domain.tld/path/path/path/filename-1.2.3.tar.gz?query&option=op" -> "filename-1.2.3.tar.gz"
#
function get_filename {
    local url="$1"
    temporal_url=${url##*/}
    temporal_url=${temporal_url%%\?*}
    eval "$2='$temporal_url'"
}

# text convert f(). e.g: from "http://user:pass@www.domain.tld/path/path/path/filename-1.2.3.tar.gz?query&option=op" -> "filename"
#
function get_shortfilename {
    local url="$1"
    temporal_url=${url##*/}
    temporal_url=${temporal_url%%\?*}
    temporal_url=${temporal_url%%\-*}
    temporal_url=${temporal_url%%\.*}
    eval "$2='$temporal_url'"
}

function add_to_path {
    dir="$1"
    if [ -d ${dir}/lib ]; then
      export LD_LIBRARY_PATH=$dir/lib:$LD_LIBRARY_PATH
    fi
    if [ -d ${dir}/bin ]; then
      export PATH=$dir/bin:$PATH
    fi
}

function confirm_install {
    counter=$1
    #
    # TODO:
    # if --yes
    #   eval "$2='$counter'"
    #   echo Setting up ${array_of_apps[$counter,1]} for installation ...

    echo "Installing ${array_of_apps[$counter,1]}"
    echo "[+] notes:: ${array_of_apps[$counter,4]}"
    echo -n "[+] Do you want to continue? [Y/n] "
    read yesno < /dev/tty
    if [ "x$yesno" = "xn" ] || [ "x$yesno" = "xN" ];then
      let counter--
    else
      get_shortfilename ${array_of_apps[$counter,2]} shortname
    fi
    eval "$2='$counter'"
}

# check available cuda libraries
#
function get_cudaconfig {
    local cudas=`locate bin/nvcc | grep bin/nvcc$`
    local number='^[0-9]+$'
    local cudalibs=`echo "$cudas" | wc -l`

    if [ $cudalibs -eq 0 ]; then
      exit_error 1 "CUDA must be installed."
    elif [ $cudalibs -gt 1 ]; then
      echo ""
      echo "Several CUDA libraries were found"
    fi
    declare -A cuda_array
    let i=0
    for cuda in $cudas; do
      cuda_trimmed=`echo $cuda | sed 's,/bin/nvcc$,,'`
      echo "[$i] $cuda_trimmed"
      cuda_array[$i]=$cuda_trimmed
      let i++
    done
    echo -n "Select one: "
    read cudanum < /dev/tty
    if [[ $cudanum =~ $number ]] && [ $cudanum -lt $i ]; then
      cuda_dir=${cuda_array[$cudanum]}
    else
      exit_error 1 "The selected CUDA libary is not correct. ${cuda_array[$cudanum]}"
    fi
    if [ ! -d ${cuda_dir}/include ]; then
      exit_error 1 "CUDA includes directory (${cuda_dir}/include) must exist."
    fi
    echo ""
    echo "Using CUDA:: $cuda_dir"
    add_to_path $cuda_dir
    if [ ! -d $temporaldirectory_prefix ]; then
      makedir $temporaldirectory_prefix
    fi
    eval "$1='$cuda_dir'"
}

# main install function
#
function install {
    local banner="$1"
    local url="$2"
    local configureparams="$3"
    get_filename $url filename # filename.tar.gz
    get_shortfilename $url shortname # filename
    installdirectory=${installdirectory_prefix}${shortname} #~/filename
    banner $banner
    # re-download if neccesary
    if [ ! -f ${temporaldirectory_prefix}${filename} ]; then
      section_banner "Downloading ${filename} to ${temporaldirectory_prefix}${filename}"
      test_url $url
      exit_error $? "url ($url) is invalid."
      download $url "${temporaldirectory_prefix}${filename}"
    fi
    # decompress and clean previous installation if neccesary
    configfile="`ls ${temporaldirectory_prefix}${shortname}/configure 2> /dev/null`"
    if [ ! -d ${temporaldirectory_prefix}${shortname} ] || [ "x$configfile" = "x" ]; then
      section_banner "Decompressing ${filename} to ${temporaldirectory_prefix}${shortname}"
      if [ ! -d ${temporaldirectory_prefix}${shortname} ]; then
        # directory to be deleted is inside /home/. Safety.
        if [ "x`echo ${temporaldirectory_prefix}${shortname}| grep -i "/home/"`" != "x" ]; then
        section_banner "Deleting previous directory ${temporaldirectory_prefix}${shortname}"
          rm -rf ${temporaldirectory_prefix}${shortname} 2> /dev/null
        fi
        makedir ${temporaldirectory_prefix}${shortname}
      fi
      tar -xzvf ${temporaldirectory_prefix}${filename} -C ${temporaldirectory_prefix}${shortname} --strip-components 1 2> /dev/null
      exit_error $? "Error decompressing file ${temporaldirectory_prefix}${filename} to ${temporaldirectory_prefix}${shortname}"
    else
      cd ${temporaldirectory_prefix}${shortname}
      section_banner "Un-Installing ${temporaldirectory_prefix}${shortname}"
      make uninstall 2> /dev/null
      section_banner "Cleaning ${temporaldirectory_prefix}${shortname}"
      make clean 2> /dev/null
      cd ..
    fi
    # Installation: ./configure; make; make install
    makedir ${installdirectory}
    cd ${temporaldirectory_prefix}${shortname}
    section_banner "Configuring ${temporaldirectory_prefix}${shortname}"
    if [ ! -f "${temporaldirectory_prefix}${shortname}/configure" ]; then
      exit_error $? "Error in installed application. Consider deleting ${temporaldirectory_prefix}${shortname} and reinstall."
    else
      echo "running ./configure --prefix=${installdirectory} ${configureparams}"
      ./configure --prefix=${installdirectory} ${configureparams}
      review_error $? "Error running ./config script for $banner."
      # exit_error $? "Error running ./config script."
    fi
    section_banner "Making ${temporaldirectory_prefix}${shortname}"
    make
    review_error $? "Error making application $banner."
    section_banner "Installing ${temporaldirectory_prefix}${shortname}"
    make install
    review_error $? "Error installing application $banner. Try removing temporal directory (rm -rf ${temporaldirectory_prefix}${shortname})"
}


# main function
#
function main {
    for i in `seq 1 $number_of_apps`; do
      install "${array_of_apps[$i,1]}" "${array_of_apps[$i,2]}" "${array_of_apps[$i,3]}"
    done

    section_banner "Installation summary"
    for i in `seq 1 $number_of_apps`; do
      get_shortfilename ${array_of_apps[$i,2]} shortname
      echo "==> Installed ${array_of_apps[$i,1]} in directory [${installdirectory_prefix}${shortname}]"
      # todo: print "export LD_LIBRARY=/app/lib:$LD_LIBRARY ... ETC" & PATH=/app/bin:$PATH ETC
    done

    for i in `seq 1 $number_of_apps`; do
      echo -n ""
      # todo: print "export LD_LIBRARY=/app/lib:$LD_LIBRARY ... ETC" & PATH=/app/bin:$PATH ETC
    done
    exit $?
}

# usage
#
function show_usage {
    echo "\nthis script will build and install external applications on your /home/ directory."
    echo "usage:: $0 [--help] [--yes] [--clean-installed] [--clean-temp]"
}


# global variables
#
declare -A array_of_apps
number_of_apps=0
cuda_dir=""
temporaldirectory_prefix=""
installdirectory_prefix=""
get_bits bits



show_usage

if [ "x$1" = "x--yes" ];then
  yestoall=true
else
  yestoall=false
fi

get_cudaconfig cuda_dir
configuration
main
