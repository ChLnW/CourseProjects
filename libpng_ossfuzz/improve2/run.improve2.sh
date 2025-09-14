#!/usr/bin/env bash
set -Eeuo pipefail

# export report to current working directory (NOT NECESSARILY WHERE THE SCRIPT LOCATES)
export REPORT_EXPORT_DIR="$(dirname $(realpath $0))"

###################################################################################################
############################## CONFIGURE A TEMPORARY WORK DIRECTORY ###############################
###################################################################################################
export WORKDIR="$REPORT_EXPORT_DIR"
echo "Using temp directory: $WORKDIR"

# TODO:
# cleanup() {
#     echo "cleaning the working directory: $WORKDIR"
#     rm -rf "$WORKDIR";
# }
# trap cleanup EXIT

cd "$WORKDIR"

###################################################################################################
###################################### CONFIGURE THE SCRIPT #######################################
###################################################################################################

# configure the script
export PROJECT=libpng_cs412
export CORPUS="$WORKDIR/build/out/corpus"
export HARNESS=libpng_read_fuzzer
export REPOSITORY=https://github.com/hamzaremmal/fuzz-libpng.git
# export LIBPNG_REPO=https://github.com/mprTest1/libpng_cs412.git
export LIBPNG_REPO="$REPOSITORY"
export DURATION=14400  # 4 hours in seconds (4 * 60 * 60)
export ARCHITECTURE=x86_64
export LANGUAGE="c++"
export IMPROVE=improve2/libpng

PROJECT_YAML=$(cat <<EOM
homepage: "http://www.libpng.org/pub/png/libpng.html"
language: c++
primary_contact: "ctruta@gmail.com"
auto_ccs:
  - "barbaro.alberto@gmail.com"
  - "oss-fuzz@jbowler.com"
vendor_ccs:
  - "aosmond@mozilla.com"
  - "tnikkel@mozilla.com"
  - "twsmith@mozilla.com"
sanitizers:
  - address
  - memory
  - undefined
architectures:
  - x86_64
main_repo: '$LIBPNG_REPO'

fuzzing_engines:
  - afl
  - honggfuzz
  - libfuzzer
EOM
)

DOCKER_FILE=$(cat <<EOM
# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

FROM gcr.io/oss-fuzz-base/base-builder@sha256:60965728fe2f95a1aff980e33f7c16ba378b57b4b4c9e487b44938a4772d0d2d
RUN apt-get update && \
    apt-get install -y make autoconf automake libtool zlib1g-dev

RUN git clone --depth 1 https://github.com/hamzaremmal/fuzz-libpng.git -b zlib zlib
RUN git clone --depth 1 --single-branch -b $IMPROVE $LIBPNG_REPO libpng_cs412
RUN cp libpng_cs412/contrib/oss-fuzz/build.sh \$SRC
RUN cp libpng_cs412/contrib/oss-fuzz/png_generator1.py libpng_cs412/
WORKDIR libpng_cs412
EOM
)

###################################################################################################
########################################## FUZZING SCRIPT #########################################
###################################################################################################

# clone the oss-fuzz repository with corpus
echo "Cloning the oss-fuzz tree from $REPOSITORY"
git clone --depth 1 "$REPOSITORY" -b oss-fuzz oss-fuzz
cd oss-fuzz

# create project
python3 infra/helper.py generate "$PROJECT" --language="$LANGUAGE"

rm -rf projects/$PROJECT/*

echo "${PROJECT_YAML}" > projects/$PROJECT/project.yaml

echo "${DOCKER_FILE}" > projects/$PROJECT/Dockerfile

# TODO Dockerfile clone from hamza's repo
# for i in {1..3}
# do
i=1
# Make sure the corpus directory exists
CORPUS_I="$CORPUS""_$i"
echo "Making sure the corpus directory exists"
mkdir -p "$CORPUS_I"
# build the image for libpng
# NOTE: this builds for the host architecture anyways
echo "Building images for project: $PROJECT"
python3 infra/helper.py build_image \
    --no-pull \
    --architecture "$ARCHITECTURE" \
    "$PROJECT"

# build the fuzzers for libpng
echo "Building fuzzers for project: $PROJECT"
python3 infra/helper.py build_fuzzers \
    --clean \
    --architecture "$ARCHITECTURE" \
    "$PROJECT"

# run the fuzzer for 4 hours
echo "Running fuzzer for project '$PROJECT' with harness '$HARNESS'"
python3 infra/helper.py run_fuzzer \
    --architecture "$ARCHITECTURE" \
    --corpus-dir "$CORPUS_I" \
    "$PROJECT" "$HARNESS" \
    -e FUZZER_ARGS=-max_total_time=$DURATION \

# build the fuzzer for coverage
echo "Building the coverage fuzzer for project: $PROJECT"
python3 infra/helper.py build_fuzzers \
    --architecture "$ARCHITECTURE" \
    --sanitizer coverage \
    "$PROJECT"

# # build the coverage
# if [[ $(ps ax | grep 8008 | fgrep -v grep | awk '{ print $1 }') ]]; then
#     kill -9 $(ps ax | grep 8008 | fgrep -v grep | awk '{ print $1 }')
# fi
echo "Building the coverage for project '$PROJECT' for harness '$HARNESS'"
python3 infra/helper.py coverage \
    --architecture "$ARCHITECTURE" \
    --corpus-dir "$CORPUS_I" \
    --fuzz-target "$HARNESS" \
    --no-corpus-download \
    "$PROJECT"
#  & pid=$! & sleep 30
# echo "Killing https service"
# kill -INT $(ps ax | grep "8008" | fgrep -v grep | awk '{ print $1 }')

# copy report out
# echo "Copying report to $REPORT_EXPORT_DIR/fuzzing_reports_$i" 
# cp -r build/out/"$PROJECT"/report "$REPORT_EXPORT_DIR/fuzzing_report_$i"

# done