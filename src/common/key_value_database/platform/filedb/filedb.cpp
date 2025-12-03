// Copyright 2025 Northern.tech AS
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

#include <common/key_value_database_filedb.hpp>

// We need these two C headers for open() and close(), respectively, because we
// need to work with file descriptors to directories (to do flock() on them) and
// there's no C++ API for that.
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h> // flock()

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <set>

#include <common/common.hpp>
#include <common/error.hpp>
#include <common/expected.hpp>
#include <common/io.hpp>
#include <common/log.hpp>
#include <common/path.hpp>

namespace mender {
namespace common {
namespace key_value_database {

namespace fs = std::filesystem;

namespace io = mender::common::io;
namespace log = mender::common::log;
namespace path = mender::common::path;

class FiledbTransaction : public Transaction {
public:
	FiledbTransaction(const string &db_dir_path, bool write) :
		db_dir_path_ {db_dir_path},
		write_ {write} {};
	~FiledbTransaction(); // aborts unless committed (or aborted explicitly)
	expected::ExpectedBytes Read(const string &key) override;
	error::Error Write(const string &key, const vector<uint8_t> &value) override;
	error::Error Remove(const string &key) override;

	error::Error Commit();
	error::Error Abort();

private:
	const string &db_dir_path_;
	bool write_;
	set<pair<string, string>> new_files_;
	set<pair<string, string>> removed_files_;
};
static const string kNewFileSuffix {"~~~new"};

expected::ExpectedBytes FiledbTransaction::Read(const string &key) {
	string data_fpath = path::Join(db_dir_path_, key);
	string data_fpath_new = path::Join(db_dir_path_, key + kNewFileSuffix);
	if ((!path::FileExists(data_fpath)
		 || (removed_files_.count(pair<string, string> {key, data_fpath}) != 0))
		&& (!path::FileExists(data_fpath_new)
			|| (removed_files_.count(pair<string, string> {key, data_fpath_new}) != 0))) {
		return expected::unexpected(MakeError(KeyError, "Key " + key + " not found in database"));
	}

	vector<uint8_t> ret;
	error::Error err;

	// In case the file was modified in this transaction, we need to read the
	// new value.
	if (path::FileExists(data_fpath_new)) {
		err = io::ReadFileContents(data_fpath_new, ret);
	} else {
		err = io::ReadFileContents(data_fpath, ret);
	}
	if (err != error::NoError) {
		return expected::unexpected(err);
	}
	return std::move(ret);
}

error::Error FiledbTransaction::Write(const string &key, const vector<uint8_t> &value) {
	if (!write_) {
		return MakeError(TransactionError, "Cannot write in a read transaction");
	}
	string data_fpath = path::Join(db_dir_path_, key);
	string data_fpath_new = path::Join(db_dir_path_, key + kNewFileSuffix);
	error::Error err;
	string db_key_dir = path::DirName(data_fpath_new);
	if (!path::FileExists(db_key_dir)) {
		err = path::CreateDirectories(path::DirName(db_key_dir));
		if (err != error::NoError) {
			return err;
		}
	}
	err = io::WriteDataIntoFile(data_fpath_new, value);
	if (err != error::NoError) {
		return err;
	}
	removed_files_.erase(pair<string, string> {key, std::move(data_fpath)});
	new_files_.insert(pair<string, string> {key, std::move(data_fpath_new)});

	return error::NoError;
}

error::Error FiledbTransaction::Remove(const string &key) {
	if (!write_) {
		return MakeError(TransactionError, "Cannot remove in a read transaction");
	}
	string data_fpath = path::Join(db_dir_path_, key);
	if (path::FileExists(data_fpath)) {
		removed_files_.insert(pair<string, string> {key, std::move(data_fpath)});
	}
	string data_fpath_new = path::Join(db_dir_path_, key + kNewFileSuffix);
	if (path::FileExists(data_fpath_new)) {
		new_files_.erase(pair<string, string> {key, data_fpath_new});
		removed_files_.insert(pair<string, string> {key, std::move(data_fpath_new)});
	}
	return error::NoError;
}

error::Error FiledbTransaction::Commit() {
	for (auto key_data_fpath : removed_files_) {
		int ret = std::remove(key_data_fpath.second.c_str());
		if (ret != 0) {
			return error::Error(
				generic_category().default_error_condition(errno),
				"Failed to delete data for key '" + key_data_fpath.first + "'");
		}
	}
	removed_files_.clear();

	for (auto key_new_file : new_files_) {
		int ret = std::rename(
			key_new_file.second.c_str(),
			key_new_file.second.substr(0, key_new_file.second.size() - kNewFileSuffix.size())
				.c_str());
		if (ret != 0) {
			return error::Error(
				generic_category().default_error_condition(errno),
				"Failed to commit data for key '" + key_new_file.first + "'");
		}
	}
	new_files_.clear();
	return error::NoError;
}

error::Error FiledbTransaction::Abort() {
	for (auto key_new_file : new_files_) {
		int ret = std::remove(key_new_file.second.c_str());
		if (ret != 0) {
			return error::Error(
				generic_category().default_error_condition(errno),
				"Failed to delete uncommitted data for key '" + key_new_file.first
					+ "': " + strerror(errno));
		}
	}
	new_files_.clear();
	removed_files_.clear();

	return error::NoError;
}

FiledbTransaction::~FiledbTransaction() {
	auto err = Abort();
	if (err != error::NoError) {
		log::Error(
			"Uncommitted key-value DB transaction destroyed and abort failed: " + err.message);
	}
}

error::Error KeyValueDatabaseFiledb::Open(const string &db_path) {
	if (!path::FileExists(db_path)) {
		auto err = path::CreateDirectories(db_path);
		if (err != error::NoError) {
			return err.WithContext("Opening DB failed");
		}
	}
	db_dir_path_ = db_path;

	return error::NoError;
}

error::Error KeyValueDatabaseFiledb::Close() {
	return error::NoError;
}

error::Error KeyValueDatabaseFiledb::RunTransaction(
	bool write, function<error::Error(Transaction &)> txnFunc) {
	int db_dir_fd = open(db_dir_path_.c_str(), O_RDONLY);
	if (db_dir_fd == -1) {
		return error::Error(
			generic_category().default_error_condition(errno),
			"Failed to open DB at '" + db_dir_path_ + "'");
	}

	int ret;
	if (write) {
		ret = flock(db_dir_fd, LOCK_EX);
	} else {
		ret = flock(db_dir_fd, LOCK_SH);
	}
	if (ret != 0) {
		return error::Error(
			generic_category().default_error_condition(errno),
			"Failed to lock DB '" + db_dir_path_ + "' for transaction");
	}

	FiledbTransaction txn {db_dir_path_, write};
	auto err = txnFunc(txn);
	if (err == error::NoError) {
		txn.Commit();
	} else {
		txn.Abort();
	}

	errno = 0;
	close(db_dir_fd);
	if (errno != 0) {
		if (err != error::NoError) {
			return err.FollowedBy(error::Error(
				generic_category().default_error_condition(errno),
				"Failed to close DB '" + db_dir_path_ + "' after transaction"));
		} else {
			return error::Error(
				generic_category().default_error_condition(errno),
				"Failed to close DB '" + db_dir_path_ + "' after transaction");
		}
	}
	return err;
}

error::Error KeyValueDatabaseFiledb::WriteTransaction(
	function<error::Error(Transaction &)> txnFunc) {
	return RunTransaction(true, txnFunc);
}

error::Error KeyValueDatabaseFiledb::ReadTransaction(
	function<error::Error(Transaction &)> txnFunc) {
	return RunTransaction(false, txnFunc);
}

} // namespace key_value_database
} // namespace common
} // namespace mender
