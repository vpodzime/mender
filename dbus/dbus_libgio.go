// Copyright 2020 Northern.tech AS
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
// +build !nodbus,cgo

package dbus

// #cgo pkg-config: gio-2.0
// #include <gio/gio.h>
// #include "dbus_libgio.go.h"
import "C"
import (
	"fmt"
	"unsafe"

	"github.com/pkg/errors"
)

// Enumeration for well-known message buses.
const (
	GBusTypeSystem  = 1
	GBusTypeSession = 2
)

// Enumeration for GBusNameOwnerFlags
const (
	DBusNameOwnerFlagsNone             = 0
	DBusNameOwnerFlagsAllowReplacement = (1 << 0)
	DBusNameOwnerFlagsReplace          = (1 << 1)
	DBusNameOwnerFlagsDoNotQueue       = (1 << 2)
)

type dbusAPILibGio struct {
	MethodCallCallbacks map[string]MethodCallCallback
}

// GenerateGUID generates a D-Bus GUID that can be used with e.g. g_dbus_connection_new().
// https://developer.gnome.org/gio/stable/gio-D-Bus-Utilities.html#g-dbus-generate-guid
func (d *dbusAPILibGio) GenerateGUID() string {
	guid := C.g_dbus_generate_guid()
	defer C.g_free(C.gpointer(guid))
	return goString(guid)
}

// IsGUID checks if string is a D-Bus GUID.
// https://developer.gnome.org/gio/stable/gio-D-Bus-Utilities.html#g-dbus-is-guid
func (d *dbusAPILibGio) IsGUID(str string) bool {
	cstr := C.CString(str)
	return goBool(C.g_dbus_is_guid(cstr))
}

// BusGet synchronously connects to the message bus specified by bus_type
// https://developer.gnome.org/gio/stable/GDBusConnection.html#g-bus-get-sync
func (d *dbusAPILibGio) BusGet(busType uint) (Pointer, error) {
	var gerror *C.GError
	conn := C.g_bus_get_sync(C.GBusType(busType), nil, &gerror)
	if Pointer(gerror) != nil {
		return Pointer(nil), ErrorFromNative(Pointer(gerror))
	}
	return Pointer(conn), nil
}

// BusOwnNameOnConnection starts acquiring name on the bus
// https://developer.gnome.org/gio/stable/gio-Owning-Bus-Names.html#g-bus-own-name-on-connection
func (d *dbusAPILibGio) BusOwnNameOnConnection(conn Pointer, name string, flags uint) (uint, error) {
	gconn := C.to_gdbusconnection(unsafe.Pointer(conn))
	cname := C.CString(name)
	cflags := C.GBusNameOwnerFlags(flags)
	gid := C.g_bus_own_name_on_connection(gconn, cname, cflags, nil, nil, nil, nil)
	if gid <= 0 {
		return 0, errors.New(fmt.Sprintf("failed to own name on bus (gid = %d)", gid))
	}
	return uint(gid), nil
}

// BusRegisterObjectForInterface registers an object for a given interface
// https://developer.gnome.org/gio/stable/gio-D-Bus-Introspection-Data.html#g-dbus-node-info-new-for-xml
// https://developer.gnome.org/gio/stable/GDBusConnection.html#g-dbus-connection-register-object
func (d *dbusAPILibGio) BusRegisterInterface(conn Pointer, path string, interfaceXML string) (uint, error) {
	var gerror *C.GError
	// extract interface from XML using introspection
	introspection := C.CString(interfaceXML)
	nodeInfo := C.g_dbus_node_info_new_for_xml(introspection, &gerror)
	if Pointer(gerror) != nil {
		return 0, ErrorFromNative(Pointer(gerror))
	}
	// register the interface in the bus
	gconn := C.to_gdbusconnection(unsafe.Pointer(conn))
	cpath := C.CString(path)
	gid := C.g_dbus_connection_register_object(gconn, cpath, *nodeInfo.interfaces, C.get_interface_vtable(), nil, nil, &gerror)
	if Pointer(gerror) != nil {
		return 0, ErrorFromNative(Pointer(gerror))
	} else if gid <= 0 {
		return 0, errors.New(fmt.Sprintf("failed to register the object interface (gid = %d)", gid))
	}
	return uint(gid), nil
}

// RegisterMethodCallCallback registers a method call callback
func (d *dbusAPILibGio) RegisterMethodCallCallback(path string, interfaceName string, method string, callback MethodCallCallback) {
	key := keyForPathInterfaceNameAndMethod(path, interfaceName, method)
	d.MethodCallCallbacks[key] = callback
}

// MainLoopNew creates a new GMainLoop structure
func (d *dbusAPILibGio) MainLoopNew() Pointer {
	return Pointer(C.g_main_loop_new(nil, 0))
}

// MainLoopRun runs a main loop until MainLoopQuit() is called
func (d *dbusAPILibGio) MainLoopRun(loop Pointer) {
	gloop := C.to_gmainloop(unsafe.Pointer(loop))
	go C.g_main_loop_run(gloop)
}

// MainLoopQuit stops a main loop from running
func (d *dbusAPILibGio) MainLoopQuit(loop Pointer) {
	gloop := C.to_gmainloop(unsafe.Pointer(loop))
	defer C.g_main_loop_quit(gloop)
}

//export handle_method_call_callback
func handle_method_call_callback(objectPath, interfaceName, methodName *C.gchar) *C.GVariant {
	goObjectPath := C.GoString(objectPath)
	goInterfaceName := C.GoString(interfaceName)
	goMethodName := C.GoString(methodName)
	key := keyForPathInterfaceNameAndMethod(goObjectPath, goInterfaceName, goMethodName)
	api, _ := GetDBusAPI()
	if callback, ok := api.(*dbusAPILibGio).MethodCallCallbacks[key]; ok {
		result, err := callback(goObjectPath, goInterfaceName, goMethodName)
		if err != nil {
			return nil
		} else if v, ok := result.(string); ok {
			return C.g_variant_new_from_string((*C.gchar)(C.CString(v)))
		} else if v, ok := result.(bool); ok {
			var vbool C.gboolean
			if v {
				vbool = 1
			} else {
				vbool = 0
			}
			return C.g_variant_new_from_boolean(vbool)
		}
	}
	return nil
}

func keyForPathInterfaceNameAndMethod(path string, interfaceName string, method string) string {
	return path + "/" + interfaceName + "." + method
}

func init() {
	dbusAPI = &dbusAPILibGio{
		MethodCallCallbacks: make(map[string]MethodCallCallback),
	}
}
