rm build/* -rf
cmake -B build -DCONFIG_LV_BUILD_EXAMPLES=OFF \
  -DCONFIG_LV_BUILD_DEMOS=OFF
make -C build -j
