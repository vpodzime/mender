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

#include <string>
#include <common/conf.hpp>

namespace mender {
namespace common {
namespace conf {
namespace paths {

using namespace std;
namespace conf = mender::common::conf;

string DefaultPathConfDir = conf::GetEnv("MENDER_CONF_DIR", "/etc/mender");
string DefaultPathDataDir = conf::GetEnv("MENDER_DATA_DIR", "/usr/share/mender");
string DefaultDataStore = conf::GetEnv("MENDER_DATASTORE_DIR", "/var/lib/mender");
string DefaultKeyFile = "mender-agent.pem";

string DefaultConfFile = DefaultPathConfDir + "/mender.conf";
string DefaultFallbackConfFile = DefaultDataStore + "/mender.conf";

// device specific paths
string DefaultArtScriptsPath = DefaultDataStore + "/scripts";
string DefaultRootfsScriptsPath = DefaultPathConfDir + "/scripts";
string DefaultModulesPath = DefaultPathDataDir + "/modules" + "/v3";
string DefaultModulesWorkPath = DefaultDataStore + "/modules" + "/v3";
string DefaultBootstrapArtifactFile = DefaultDataStore + "/bootstrap.mender";

} // namespace paths
} // namespace conf
} // namespace common
} // namespace mender
