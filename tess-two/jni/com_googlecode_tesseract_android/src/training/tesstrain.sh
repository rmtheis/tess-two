#!/bin/bash
# (C) Copyright 2014, Google Inc.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This script provides an easy way to execute various phases of training
# Tesseract.  For a detailed description of the phases, see
# https://github.com/tesseract-ocr/tesseract/wiki/TrainingTesseract
#
# USAGE:
#
# tesstrain.sh
#    --fontlist FONTS           # A list of fontnames to train on.
#    --fonts_dir FONTS_PATH     # Path to font files.
#    --lang LANG_CODE           # ISO 639 code.
#    --langdata_dir DATADIR     # Path to tesseract/training/langdata directory.
#    --output_dir OUTPUTDIR     # Location of output traineddata file.
#    --overwrite                # Safe to overwrite files in output_dir.
#    --linedata_only            # Only generate training data for lstmtraining.
#    --run_shape_clustering     # Run shape clustering (use for Indic langs).
#    --exposures EXPOSURES      # A list of exposure levels to use (e.g. "-1 0 1").
#
# OPTIONAL flags for input data. If unspecified we will look for them in
# the langdata_dir directory.
#    --training_text TEXTFILE   # Text to render and use for training.
#    --wordlist WORDFILE        # Word list for the language ordered by
#                               # decreasing frequency.
#
# OPTIONAL flag to specify location of existing traineddata files, required
# during feature extraction. If unspecified will use TESSDATA_PREFIX defined in
# the current environment.
#    --tessdata_dir TESSDATADIR     # Path to tesseract/tessdata directory.
#
# NOTE:
# The font names specified in --fontlist need to be recognizable by Pango using
# fontconfig. An easy way to list the canonical names of all fonts available on
# your system is to run text2image with --list_available_fonts and the
# appropriate --fonts_dir path.


source "$(dirname $0)/tesstrain_utils.sh"

ARGV=("$@")
parse_flags

mkdir -p ${TRAINING_DIR}
tlog "\n=== Starting training for language '${LANG_CODE}'"

source "$(dirname $0)/language-specific.sh"
set_lang_specific_parameters ${LANG_CODE}

initialize_fontconfig

phase_I_generate_image 8
phase_UP_generate_unicharset
phase_D_generate_dawg
if ((LINEDATA)); then
  phase_E_extract_features "lstm.train" 8 "lstmf"
  make__lstmdata
else
  phase_E_extract_features "box.train" 8 "tr"
  phase_C_cluster_prototypes "${TRAINING_DIR}/${LANG_CODE}.normproto"
  if [[ "${ENABLE_SHAPE_CLUSTERING}" == "y" ]]; then
      phase_S_cluster_shapes
  fi
  phase_M_cluster_microfeatures
  phase_B_generate_ambiguities
  make__traineddata
fi

tlog "\nCompleted training for language '${LANG_CODE}'\n"
