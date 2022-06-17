ABS=$(pwd)
mkdir tests_tmp_dll tests_tmp_app
$ABS/asar-tests/asar-dll-test $ABS/asar/libasar.so $ABS/tests $ABS/dummy_rom.sfc $ABS/tests_tmp_dll
$ABS/asar-tests/asar-app-test $ABS/asar/asar $ABS/tests $ABS/dummy_rom.sfc $ABS/tests_tmp_app
