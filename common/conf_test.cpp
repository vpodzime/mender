// Copyright 2023 Northern.tech AS
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <common/conf.hpp>

#include <vector>
#include <string>
#include <cstdlib>
#include <vector>

#include <gtest/gtest.h>

namespace conf = mender::common::conf;

using namespace std;

TEST(ConfTests, GetEnvTest) {
	auto value = conf::GetEnv("MENDER_CONF_TEST_VAR", "default_value");
	EXPECT_EQ(value, "default_value");

	char var[] = "MENDER_CONF_TEST_VAR=mender_conf_test_value";
	int ret = putenv(var);
	ASSERT_EQ(ret, 0);

	value = conf::GetEnv("MENDER_CONF_TEST_VAR", "default_value");
	EXPECT_EQ(value, "mender_conf_test_value");
}

TEST(ConfTests, CmdlineOptionsIteratorGoodTest) {
	vector<string> args = {
		"--opt1",
		"val1",
		"-o2",
		"val2",
		"--opt3",
		"arg1",
		"--opt4=val4",
		"arg2",
		"--opt5",
		"-o6=val6",
		"arg3",
		"-o7",
	};

	conf::CmdlineOptionsIterator opts_iter(args,
										   {"--opt1",
											"-o2",
											"--opt4",
											"-o6"},
										   {"--opt3", "--opt5", "-o7"});
	auto ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	auto opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "--opt1");
	EXPECT_EQ(opt_val.value, "val1");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "-o2");
	EXPECT_EQ(opt_val.value, "val2");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "--opt3");
	EXPECT_EQ(opt_val.value, "");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "arg1");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "--opt4");
	EXPECT_EQ(opt_val.value, "val4");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "arg2");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "--opt5");
	EXPECT_EQ(opt_val.value, "");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "-o6");
	EXPECT_EQ(opt_val.value, "val6");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "arg3");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "-o7");
	EXPECT_EQ(opt_val.value, "");

	// termination object
	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "");

	// should stay at the termination object and not fail
	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "");
}

TEST(ConfTests, CmdlineOptionsIteratorDoubleDashTest) {
	vector<string> args = {
		"--opt1",
		"val1",
		"-o2",
		"val2",
		"--",
		"--opt3",
		"arg1",
		"--opt4=val4",
	};

	conf::CmdlineOptionsIterator opts_iter(args,
										   {"--opt1",
											"-o2",
											"--opt4",
											"-o6"},
										   {"--opt3", "--opt5", "-o7"});
	auto ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	auto opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "--opt1");
	EXPECT_EQ(opt_val.value, "val1");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "-o2");
	EXPECT_EQ(opt_val.value, "val2");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "--");
	EXPECT_EQ(opt_val.value, "");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "--opt3");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "arg1");

	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "--opt4=val4");

	// termination object
	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "");

	// should stay at the termination object and not fail
	ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "");
	EXPECT_EQ(opt_val.value, "");
}

TEST(ConfTests, CmdlineOptionsIteratorBadOptionTest) {
	vector<string> args = {
		"--opt1",
		"val1",
		"-o2",
	};

	conf::CmdlineOptionsIterator opts_iter(args,
										   {"--opt1",
											"--opt4",
											"-o6"},
										   {"--opt3", "--opt5", "-o7"});
	auto ex_opt_val = opts_iter.Next();
	ASSERT_TRUE(ex_opt_val);
	auto opt_val = ex_opt_val.value();
	EXPECT_EQ(opt_val.option, "--opt1");
	EXPECT_EQ(opt_val.value, "val1");

	ex_opt_val = opts_iter.Next();
	ASSERT_FALSE(ex_opt_val);
	EXPECT_EQ(ex_opt_val.error().message, "Unrecognized option '-o2'");
}

TEST(ConfTests, CmdlineOptionsIteratorOptionMissingValueTest) {
	vector<string> args = {
		"--opt1",
		"-o2",
		"val2",
	};

	conf::CmdlineOptionsIterator opts_iter(args,
										   {"--opt1",
											"-o2",
											"--opt4",
											"-o6"},
										   {"--opt3", "--opt5", "-o7"});
	auto ex_opt_val = opts_iter.Next();
	ASSERT_FALSE(ex_opt_val);
	EXPECT_EQ(ex_opt_val.error().message, "Option --opt1 missing value");
}

TEST(ConfTests, CmdlineOptionsIteratorOptionMissingValueTrailingTest) {
	vector<string> args = {
		"--opt1",
	};

	conf::CmdlineOptionsIterator opts_iter(args,
										   {"--opt1",
											"-o2",
											"--opt4",
											"-o6"},
										   {"--opt3", "--opt5", "-o7"});
	auto ex_opt_val = opts_iter.Next();
	ASSERT_FALSE(ex_opt_val);
	EXPECT_EQ(ex_opt_val.error().message, "Option --opt1 missing value");
}

TEST(ConfTests, CmdlineOptionsIteratorOptionExtraValueTest) {
	vector<string> args = {
		"--opt3=val3",
		"-o2",
		"val2",
	};

	conf::CmdlineOptionsIterator opts_iter(args,
										   {"--opt1",
											"-o2",
											"--opt4",
											"-o6"},
										   {"--opt3", "--opt5", "-o7"});
	auto ex_opt_val = opts_iter.Next();
	ASSERT_FALSE(ex_opt_val);
	EXPECT_EQ(ex_opt_val.error().message, "Option --opt3 doesn't expect a value");
}
