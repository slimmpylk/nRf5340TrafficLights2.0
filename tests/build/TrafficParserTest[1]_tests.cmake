add_test([=[TrafficParserTest.ValidCharacterOnlySequence]=]  C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build/TrafficParserTest.exe [==[--gtest_filter=TrafficParserTest.ValidCharacterOnlySequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TrafficParserTest.ValidCharacterOnlySequence]=]  PROPERTIES WORKING_DIRECTORY C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
add_test([=[TrafficParserTest.ValidDurationSequence]=]  C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build/TrafficParserTest.exe [==[--gtest_filter=TrafficParserTest.ValidDurationSequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TrafficParserTest.ValidDurationSequence]=]  PROPERTIES WORKING_DIRECTORY C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
add_test([=[TrafficParserTest.InvalidCharacterSequence]=]  C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build/TrafficParserTest.exe [==[--gtest_filter=TrafficParserTest.InvalidCharacterSequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TrafficParserTest.InvalidCharacterSequence]=]  PROPERTIES WORKING_DIRECTORY C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
add_test([=[TrafficParserTest.EmptySequence]=]  C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build/TrafficParserTest.exe [==[--gtest_filter=TrafficParserTest.EmptySequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TrafficParserTest.EmptySequence]=]  PROPERTIES WORKING_DIRECTORY C:/Users/sammi/Desktop/vko5Pylkkonen/tests/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  TrafficParserTest_TESTS TrafficParserTest.ValidCharacterOnlySequence TrafficParserTest.ValidDurationSequence TrafficParserTest.InvalidCharacterSequence TrafficParserTest.EmptySequence)
