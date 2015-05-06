/**********************************************************************
 * File:        tprintf.h
 * Description: Trace version of printf - portable between UX and NT
 * Author:      Phil Cheatle
 * Created:     Wed Jun 28 15:01:15 BST 1995
 *
 * (C) Copyright 1995, Hewlett-Packard Ltd.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#ifndef           TESSERACT_CCUTIL_TPRINTF_H
#define           TESSERACT_CCUTIL_TPRINTF_H

#include "params.h"

extern DLLSYM STRING_VAR_H(debug_file, "",
                           "File to send tprintf output to");
extern DLLSYM BOOL_VAR_H(debug_window_on, TRUE,
                         "Send tprintf to window unless file set");

// Main logging function.
#define tprintf(...) tprintf_internal(__VA_ARGS__)

extern TESS_API void tprintf_internal(  // Trace printf
    const char *format, ...);           // Message

#endif  // define TESSERACT_CCUTIL_TPRINTF_H
