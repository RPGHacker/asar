;`20 00 80 20 00 80 20 0C 80 20 03 80 20 0C 80 20 03 80

org $008000
namespace test
namespace nested on
namespace test2
test3:
pushns
namespace asdf
jsr test_test2_test3
test3:
test2:
pullns
jsr test3
jsr asdf_test3
jsr asdf_test2
namespace asdf
test3:
namespace nested off
namespace off
jsr test_test2_asdf_test3
jsr asdf_test3
pushns
pullns
