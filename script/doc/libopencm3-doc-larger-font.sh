#!/bin/sh
cp siuyin.css ../../libopencm3/doc
cp Doxyfile_Device ../../libopencm3/doc/templates

cd ../../libopencm3/doc
make html TARGETS='stm32/f4'
