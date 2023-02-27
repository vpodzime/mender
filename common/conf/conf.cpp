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

#include <string>
#include <cstdlib>

#include <common/error.hpp>
#include <common/expected.hpp>
#include <common/conf/paths.hpp>
#include <common/log.hpp>

namespace mender {
namespace common {
namespace conf {

using namespace std;
namespace error = mender::common::error;
namespace expected = mender::common::expected;
namespace log = mender::common::log;

const ConfigErrorCategoryClass ConfigErrorCategory;

const char *ConfigErrorCategoryClass::name() const noexcept {
	return "ConfigErrorCategory";
}

string ConfigErrorCategoryClass::message(int code) const {
	switch (code) {
	case NoError:
		return "Success";
	case InvalidOptionsError:
		return "Invalid options given";
	default:
		return "Unknown";
	}
}

error::Error MakeError(ConfigErrorCode code, const string &msg) {
	return error::Error(error_condition(code, ConfigErrorCategory), msg);
}


string GetEnv(const string &var_name, const string &default_value) {
	const char *value = getenv(var_name.c_str());
	if (value == nullptr) {
		return string(default_value);
	} else {
		return string(value);
	}
}

ExpectedOptionValue CmdlineOptionsIterator::Next() {
	string option = "";
	string value = "";

	if (pos_ >= args_.size()) {
		return ExpectedOptionValue({"", ""});
	}

	if (past_double_dash_) {
		OptionValue opt_val{"", args_[pos_]};
		pos_++;
		return ExpectedOptionValue(opt_val);
	}

	if (args_[pos_] == "--") {
		past_double_dash_ = true;
		pos_++;
		return ExpectedOptionValue({"--", ""});
	}

	if (args_[pos_][0] == '-') {
		auto eq_idx = args_[pos_].find('=');
		if (eq_idx != string::npos) {
			option = args_[pos_].substr(0, eq_idx);
			value = args_[pos_].substr(eq_idx + 1, args_[pos_].size() - eq_idx - 1);
			pos_++;
		} else {
			option = args_[pos_];
			pos_++;
		}

		if (opts_with_value_.count(option) != 0) {
			// option with value
			if ((value == "") && ((pos_ >= args_.size()) || (args_[pos_][0] == '-'))) {
				// the next item is not a value
				error::Error err = MakeError(
					ConfigErrorCode::InvalidOptionsError,
					"Option " + option + " missing value");
				return ExpectedOptionValue(err);
			} else if (value == "") {
				// only assign the next item as value if there was no value
				// specified as '--opt=value' (parsed above)
				value = args_[pos_];
				pos_++;
			}
		} else if (opts_wo_value_.count(option) == 0) {
			// unknown option
			error::Error err = MakeError(
				ConfigErrorCode::InvalidOptionsError, "Unrecognized option '" + option + "'");
			return ExpectedOptionValue(err);
		} else if (value != "") {
			// option without a value, yet, there was a value specified as '--opt=value' (parsed
			// above)
			error::Error err = MakeError(
				ConfigErrorCode::InvalidOptionsError,
				"Option " + option + " doesn't expect a value");
			return ExpectedOptionValue(err);
		}
	} else {
		value = args_[pos_];
		pos_++;
	}

	return ExpectedOptionValue({std::move(option), std::move(value)});
}

error::Error MenderConfig::ProcessCmdlineArgs(const vector<string> &args) {
	string config_path = paths::DefaultConfFile;
	string fallback_config_path = paths::DefaultFallbackConfFile;
	string log_file = "";
	string log_level = log::ToStringLogLevel(log::kDefaultLogLevel);

	CmdlineOptionsIterator opts_iter(
		args,
		{"--config",
		 "-c",
		 "--fallback-config",
		 "-b",
		 "--data",
		 "-d",
		 "--log-file",
		 "-L",
		 "--log-level",
		 "-l"},
		{});
	auto ex_opt_val = opts_iter.Next();
	while (ex_opt_val && ((ex_opt_val.value().option != "") || (ex_opt_val.value().value != ""))) {
		auto opt_val = ex_opt_val.value();
		if ((opt_val.option == "--config") || (opt_val.option == "-c")) {
			config_path = opt_val.value;
		} else if ((opt_val.option == "--fallback-config") || (opt_val.option == "-b")) {
			fallback_config_path = opt_val.value;
		} else if ((opt_val.option == "--data") || (opt_val.option == "-d")) {
			data_store_dir = opt_val.value;
		} else if ((opt_val.option == "--log-file") || (opt_val.option == "-L")) {
			log_file = opt_val.value;
		} else if ((opt_val.option == "--log-level") || (opt_val.option == "-l")) {
			log_level = opt_val.value;
		}
	}
	if (!ex_opt_val) {
		return ex_opt_val.error();
	}

	if (log_file != "") {
		auto err = log::SetupFileLogging(log_file, true);
		if (err) {
			return err;
		}
	}

	auto ex_log_level = log::StringToLogLevel(log_level);
	if (!ex_log_level) {
		return ex_log_level.error();
	}
	SetLevel(ex_log_level.value());

	auto ret = this->LoadFile(fallback_config_path);
	if (!ret || !this->ValidateConfig()) {
		this->Reset();
	}
	ret = this->LoadFile(config_path);
	if (!ret || !this->ValidateConfig()) {
		this->Reset();
	}

	return error::NoError;
}

} // namespace conf
} // namespace common
} // namespace mender
