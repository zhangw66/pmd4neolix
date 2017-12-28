#!/bin/bash
./clean.sh
ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=Application.mk
