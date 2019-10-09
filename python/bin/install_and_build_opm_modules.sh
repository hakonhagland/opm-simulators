#! /bin/bash

run_cmake_and_make() {
    mkdir "$build_dir_name"
    cd "$build_dir_name"
    # NOTE: -DOPM_ENABLE_PYTHON=ON is only needed/used for opm-common
    cmake -DOPM_ENABLE_PYTHON=ON -DCMAKE_BUILD_TYPE="$build_type" ..
    make -j12
}

clone_and_build_specific_commit_master() {
    repo="$1"
    commit="$2"
    echo "=== Cloning and building module: $repo"
    if [[ -d $repo ]] ; then
        echo "Repository $repo already exists. Abort.!"
        exit
    fi
    git clone git@github.com:OPM/"$repo".git
    cd "$repo"
    git checkout -b python "$commit"
    run_cmake_and_make
}

# check out commit 0aaf9823c131c9a7d5fccd501f3ac10f104f9cc5 from
#   jmsargado/bloodmoon
#   Date:   Wed Jul 3 12:38:33 2019 +0200
clone_and_build_jms_opm_common() {
    echo "=== Cloning and building module: opm-common"
    git clone git@github.com:jmsargado/opm-common.git
    cd opm-common
    git checkout origin/bloodmoon
    git checkout -b python 0aaf9823c131c9a7d5fccd501f3ac10f104f9cc5
    run_cmake_and_make
}

# check out commit d8490740ae6a95d3d54e6b6ea4b84148984c80f2 from
#  jmsargado/python
#  Date:   Tue Sep 10 17:15:29 2019 +0200
clone_and_build_jms_opm_simulators() {
    echo "=== Cloning and building module: opm-simulators"
    git clone git@github.com:jmsargado/opm-simulators.git
    cd opm-simulators
    # check out lates version of python branch
    git checkout origin/python
    git checkout -b python d8490740ae6a95d3d54e6b6ea4b84148984c80f2
    run_cmake_and_make
}

# check out commit d8490740ae6a95d3d54e6b6ea4b84148984c80f2 from
#  jmsargado/python
#  Date:   Tue Sep 10 17:15:29 2019 +0200
build_hakonh_opm_simulators() {
    echo "=== Building module: opm-simulators"
    cd opm-simulators
    # check out lates version of python branch
    run_cmake_and_make
}

clone_and_build_jms_ewoms() {
    echo "=== Cloning and building module: ewoms"
    git clone git@github.com:jmsargado/ewoms.git
    cd ewoms
    git checkout origin/bloodmoon
    git checkout -b python 34bd25160d3b59237bbe1612fd76d449833362cf
    run_cmake_and_make
}

clone_opm_data() {
    repo="opm-data"
    echo "=== Cloning module: opm-data"
    if [[ -d $repo ]] ; then
        echo "Repository $repo already exists. Abort.!"
        exit
    fi
    git clone git@github.com:OPM/"$repo".git
}

# -DCMAKE_BUILD_TYPE=Debug 
# -DCMAKE_BUILD_TYPE=Release 

build_dir_name=build
build_type=Release
curdir="$PWD"
log_fn=clone_and_build_log.txt
if [[ -z $ecl_DIR ]] ; then
    echo "ecl_DIR environment variable is not set"\
         "see https://github.com/hakonhagland/opm-simulators for installation"\
         "instructions"
    exit 1
fi
    
# Run these commands in subshells to keep current directory fixed
(clone_and_build_jms_opm_common 2>&1 | tee -a $log_fn)
(clone_and_build_specific_commit_master \
     opm-grid d6e4ba1f290ad92632769b306ac438851283d7da 2>&1 | tee -a $log_fn)
(clone_and_build_specific_commit_master \
     opm-material 77202e291ddc05ebb8fa133d896a25765308b8a9 2>&1 | tee -a $log_fn)
(clone_and_build_jms_ewoms 2>&1 | tee -a $log_fn)
(clone_and_build_specific_commit_master \
     opm-models 1452b575104b50b7f1bd6fce99d34a50e65a39bb 2>&1 | tee -a $log_fn)
# (clone_and_build_jms_opm_simulators 2>&1 | tee -a $log_fn)
(build_hakonh_opm_simulators 2>&1 | tee -a $log_fn)
(clone_opm_data 2>&1 | tee -a $log_fn)

# Add the path to the bin folder
echo
echo "=== Done."
echo "Remember to appending the following paths to the PATH variable"\
     "in your ~/.bashrc initialization file:"
echo "$curdir"/opm-simulators/"$build_dir_name"/bin
echo "$curdir"/opm-common/"$build_dir_name"/bin
