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

#include <common/conf/paths.hpp>

#include <string>
#include <common/conf.hpp>
#include <boost/filesystem.hpp>

namespace mender {
namespace common {
namespace conf {
namespace paths {

using namespace std;
namespace conf = mender::common::conf;
namespace fs = boost::filesystem;

string DefaultPathConfDir = conf::GetEnv("MENDER_CONF_DIR", fs::path("/etc") / "mender");
string DefaultPathDataDir = conf::GetEnv("MENDER_DATA_DIR", fs::path("/usr/share") / "mender");
string DefaultDataStore = conf::GetEnv("MENDER_DATASTORE_DIR", fs::path("/var/lib") / "mender");
string DefaultKeyFile = "mender-agent.pem";

string DefaultConfFile = (fs::path(DefaultPathConfDir) / "mender.conf").string();
string DefaultFallbackConfFile = (fs::path(DefaultDataStore) / "mender.conf").string();

// device specific paths
string DefaultArtScriptsPath = (fs::path(DefaultDataStore) / "scripts").string();
string DefaultRootfsScriptsPath = (fs::path(DefaultPathConfDir) / "scripts").string();
string DefaultModulesPath = (fs::path(DefaultPathDataDir) / "/modules" / "/v3").string();
string DefaultModulesWorkPath = (fs::path(DefaultDataStore) / "/modules" / "/v3").string();
string DefaultBootstrapArtifactFile = (fs::path(DefaultDataStore) / "bootstrap.mender").string();

} // namespace paths
} // namespace conf
} // namespace common
} // namespace mender
