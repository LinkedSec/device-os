/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPARK_WIRING_BLE_H
#define SPARK_WIRING_BLE_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "spark_wiring_platform.h"

#define Wiring_BLE 1
#if Wiring_BLE

#include "system_error.h"
#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_flags.h"
#include "ble_hal.h"
#include "check.h"

using spark::Vector;
using particle::Flags;

class BleCharacteristic;
class BleService;
class BleGattServer;
class BleBroadcaster;
class BleObserver;
class BlePeripheral;
class BleCentral;
class BlePeerDevice;
class BleLocalDevice;

enum class BleUuidType {
    SHORT = 0,
    LONG = 1
};

enum class BleUuidOrder {
    MSB = 0,
    LSB = 1
};

struct BleDevRoleType; // Tag type for BLE device role flags
typedef Flags<BleDevRoleType, uint8_t> BleDevRoles;
typedef BleDevRoles::FlagType BleDevRole;

namespace ROLE {
    const BleDevRole INVALID(BLE_ROLE_INVALID);
    const BleDevRole PERIPHERAL(BLE_ROLE_PERIPHERAL);
    const BleDevRole CENTRAL(BLE_ROLE_CENTRAL);
    const BleDevRole ALL(0xff);
}

struct BleAttrPropType; // Tag type for BLE attribute property flags
typedef Flags<BleAttrPropType, uint8_t> BleCharProps;
typedef BleCharProps::FlagType BleAttrProp;

namespace PROPERTY {
    const BleAttrProp NONE(0);
    const BleAttrProp READ(BLE_SIG_CHAR_PROP_READ);
    const BleAttrProp WRITE(BLE_SIG_CHAR_PROP_WRITE);
    const BleAttrProp WRITE_WO_RSP(BLE_SIG_CHAR_PROP_WRITE_WO_RESP);
    const BleAttrProp NOTIFY(BLE_SIG_CHAR_PROP_NOTIFY);
    const BleAttrProp INDICATE(BLE_SIG_CHAR_PROP_INDICATE);
}

namespace PARTICLE_BLE {
    const uint8_t BLE_USER_DEFAULT_SVC_UUID[BLE_SIG_UUID_128BIT_LEN] = {
        0x2d, 0x49, 0xc0, 0x26, 0xf0, 0xdb, 0xce, 0x06, 0x7a, 0x33, 0x6f, 0x12, 0x00, 0x00, 0xf3, 0x5d
    };
}

typedef uint16_t BleConnHandle;

class BleAddress : public hal_ble_addr_t {
public:
    void operator = (hal_ble_addr_t addr) {
        this->addr_type = addr.addr_type;
        memcpy(this->addr, addr.addr, BLE_SIG_ADDR_LEN);
    }

    bool operator == (const BleAddress& addr) const {
        if (this->addr_type == addr.addr_type && !memcmp(this->addr, addr.addr, BLE_SIG_ADDR_LEN)) {
            return true;
        }
        return false;
    }
};

class BleAdvParams : public hal_ble_adv_params_t {
};

class BleConnParams : public hal_ble_conn_params_t {
};

class BleScanParams : public hal_ble_scan_params_t {
};

class BleCharHandles : public hal_ble_char_handles_t {
};


class BleUuid {
public:
    BleUuid();
    BleUuid(const uint8_t* uuid128, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const uint8_t* uuid128, uint16_t uuid16, BleUuidOrder order = BleUuidOrder::LSB);
    BleUuid(const String& str);
    BleUuid(const char* string);
    ~BleUuid();

    BleUuidType type(void) const {
        return type_;
    }

    BleUuidOrder order(void) const {
        return order_;
    }

    uint16_t shortUuid(void) const {
        return shortUuid_;
    }

    void fullUuid(uint8_t uuid128[BLE_SIG_UUID_128BIT_LEN]) const {
        memcpy(uuid128, fullUuid_, BLE_SIG_UUID_128BIT_LEN);
    }

    const uint8_t* fullUuid(void) const {
        return fullUuid_;
    }

    bool operator == (const BleUuid& uuid) const;

private:
    BleUuidType type_;
    BleUuidOrder order_;
    uint16_t shortUuid_;
    uint8_t fullUuid_[BLE_SIG_UUID_128BIT_LEN];

    int8_t toInt(char c);
    void setUuid(const String& str);
};


class iBeacon {
public:
    iBeacon() : major_(0),minor_(0),uuid_(nullptr),measurePower_(0) {

    }
    iBeacon(uint16_t major, uint16_t minor, uint8_t* uuid, int8_t mp) : major_(major),minor_(minor),uuid_(uuid),measurePower_(mp) {

    }
    ~iBeacon() {

    }

    uint16_t major(void) const {
        return major_;
    }

    uint16_t minor(void) const {
        return minor_;
    }

    uint8_t* uuid(void) const {
        return uuid_;
    }

    int8_t measurePower(void) {
        return measurePower_;
    }

private:
    uint16_t major_;
    uint16_t minor_;
    uint8_t* uuid_;
    int8_t   measurePower_;
};


class BleAdvData {
public:
    uint8_t data[BLE_MAX_ADV_DATA_LEN];
    size_t len;

    BleAdvData();
    ~BleAdvData();

    bool contain(uint8_t type);
    bool contain(uint8_t type, const uint8_t* buf, size_t len);

    size_t fetch(uint8_t type, uint8_t* buf, size_t len);

    size_t locate(uint8_t type, size_t* offset);
};


class BleCharacteristic {
public:
    typedef void (*onDataReceivedCb)(const uint8_t* data, size_t len);

    BleCharProps properties;
    BleCharHandles attrHandles;
    BleUuid uuid;
    const char* description;

    bool valid;
    bool isLocal;
    Vector<BleConnHandle> cccdOfServer;
    bool cccdOfClient;
    BleService* service;

    BleCharacteristic();
    BleCharacteristic(const char* desc, BleCharProps properties, onDataReceivedCb cb = nullptr);
    BleCharacteristic(const char* desc, BleCharProps properties, BleUuid& charUuid, BleUuid& svcUuid, onDataReceivedCb cb = nullptr);
    ~BleCharacteristic();

    size_t getValue(uint8_t* buf, size_t len) const;
    size_t getValue(String& str) const;

    template<typename T>
    size_t getValue(T* val) const {
        size_t len = sizeof(T);
        return getValue(reinterpret_cast<uint8_t*>(val), len);
    }

    int setValue(const uint8_t* buf, size_t len);
    int setValue(const String& str);
    int setValue(const char* str);

    template<typename T>
    int setValue(T val) {
        uint8_t buf[BLE_MAX_CHAR_VALUE_LEN];
        size_t len = sizeof(T) > BLE_MAX_CHAR_VALUE_LEN ? BLE_MAX_CHAR_VALUE_LEN : sizeof(T);
        for (size_t i = 0, j = len - 1; i < len; i++, j--) {
            buf[i] = reinterpret_cast<const uint8_t*>(&val)[j];
        }
        return setValue(buf, len);
    }

    void onDataReceived(onDataReceivedCb callback);

    void syncStub(void) const;

    void processReceivedData(uint16_t attrHandle, const uint8_t* data, size_t len, const BlePeerDevice& peer);

private:
    static uint16_t defaultUuidCharCount_;

    BleCharacteristic* stub_;
    onDataReceivedCb dataCb_;

    void init(void);
    BleUuid generateDefaultCharUuid(void) const;
};


class BleService {
public:
    BleUuid uuid;
    uint16_t startHandle;
    uint16_t endHandle;

    BleGattServer* server;

    BleService();
    ~BleService();

    size_t characteristicCount(void) const {
        return characteristics_.size();
    }

    BleCharacteristic* findCharacteristic(const char* desc);
    BleCharacteristic* findCharacteristic(uint16_t attrHandle);
    BleCharacteristic* findCharacteristic(const BleUuid& charUuid);
    BleCharacteristic* findCharacteristic(size_t i);

    int addCharacteristic(BleCharacteristic& characteristic);

private:
    Vector<BleCharacteristic> characteristics_;
};


class BleGattServer {
public:
    BleGattServer();
    ~BleGattServer();

    void* device;

    size_t serviceCount(void) const {
        return services_.size();
    }

    BleService* findService(const BleUuid& svcUuid);
    BleService* findService(size_t i);

    BleService* addService(const BleUuid& svcUuid);

    size_t characteristicCount(void) const;

    template <typename T>
    BleCharacteristic* findCharacteristic(T& type) {
        for (size_t i = 0; i < serviceCount(); i++) {
            BleCharacteristic* characteristic = services_.at(i).findCharacteristic(type);
            if (characteristic != nullptr) {
                return characteristic;
            }
        }
        return nullptr;
    }

    int addCharacteristic(BleService& service, BleCharacteristic& characteristic);

    static int write(const BleCharacteristic& characteristic, const uint8_t* buf, size_t len);
    static int read(const BleCharacteristic& characteristic, uint8_t* buf, size_t len);

protected:
    void finalizeLocalGattServer(void);
    void gattServerProcessDisconnected(const BlePeerDevice& peer);
    void gattServerProcessDataWritten(uint16_t attrHandle, const uint8_t* buf, size_t len, BlePeerDevice& peer);

private:
    Vector<BleService> services_;
};


class BleGattClient {
public:
    BleGattClient();
    ~BleGattClient();

    static int write(const BleCharacteristic& characteristic, const uint8_t* buf, size_t len);
    static int read(const BleCharacteristic& characteristic, uint8_t* buf, size_t len);

protected:
    void gattClientProcessDataNotified(uint16_t attrHandle, const uint8_t* buf, size_t len, BlePeerDevice& peer);
};


class BleBroadcaster {
public:
    BleBroadcaster();
    ~BleBroadcaster();

    int appendAdvData(uint8_t type, const uint8_t* data, size_t len);
    int appendAdvDataLocalName(const char* name);
    int appendAdvDataCustomData(const uint8_t* buf, size_t len);
    int appendAdvDataUuid(const BleUuid& uuid);

    int appendScanRspData(uint8_t type, const uint8_t* buf, size_t len);
    int appendScanRspDataLocalName(const char* name);
    int appendScanRspDataCustomData(const uint8_t* buf, size_t len);
    int appendScanRspDataUuid(const BleUuid& uuid);

    int advDataBeacon(const iBeacon& beacon);

    int clearAdvData(void);
    int removeFromAdvData(uint8_t type);

    int clearScanRspData(void);
    int removeFromScanRspData(uint8_t type);

    int setTxPower(int8_t val) const;
    int8_t txPower(void) const;

    int advertise(void);
    int advertise(uint32_t interval);
    int advertise(uint32_t interval, uint32_t timeout);
    int advertise(const BleAdvParams& params);

    int stopAdvertise(void) const;

protected:
    void broadcasterProcessStopped(void);

private:
    BleAdvParams advParams_;
    BleAdvData advData_;
    BleAdvData srData_;

    int append(uint8_t type, const uint8_t* buf, size_t len, BleAdvData& data);
};


class BleScannedDevice {
public:
    BleAddress address;
    BleAdvData advData;
    BleAdvData srData;
    int8_t rssi;
};


class BleObserver {
public:
    BleObserver();
    ~BleObserver();

    static const size_t DEFAULT_COUNT = 5;

    typedef void (*BleScanCallback)(const BleScannedDevice *device);

    int scan(BleScanCallback callback);
    int scan(BleScanCallback callback, uint16_t timeout);
    int scan(BleScannedDevice* results, size_t resultCount);
    int scan(BleScannedDevice* results, size_t resultCount, uint16_t timeout);
    int scan(BleScannedDevice* results, size_t resultCount, const BleScanParams& params);

    int stopScan(void);

protected:
    void observerProcessScanResult(const hal_ble_gap_on_scan_result_evt_t* event);
    void observerProcessScanStopped(const hal_ble_gap_on_scan_stopped_evt_t* event);

private:
    BleScanParams scanParams_;
    size_t targetCount_;
    BleScanCallback callback_;
    BleScannedDevice* results_;
    size_t count_;
};


class BlePeripheral {
public:
    BlePeripheral();
    ~BlePeripheral();

    size_t centralCount(void) const {
        return centrals_.size();
    }

    int setPpcp(void);
    int setPpcp(uint16_t minInterval, uint16_t maxInterval, uint16_t latency = BLE_DEFAULT_SLAVE_LATENCY, uint16_t timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT);

    int disconnect(void);

    bool connected(void) const {
        return centrals_.size() > 0;
    }

protected:
    BlePeerDevice* centralAt(size_t i);
    void peripheralProcessConnected(const BlePeerDevice& peer);
    void peripheralProcessDisconnected(const BlePeerDevice& peer);

private:
    BleConnParams ppcp_;
    Vector<BlePeerDevice> centrals_;
};


class BleCentral {
public:
    BleCentral();
    ~BleCentral();

    size_t peripheralCount(void) const {
        return peripherals_.size();
    }

    BlePeerDevice* connect(const BleAddress& addr,
            uint16_t interval = BLE_DEFAULT_MIN_CONN_INTERVAL,
            uint16_t latency = BLE_DEFAULT_SLAVE_LATENCY,
            uint16_t timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT);

    int disconnect(const BlePeerDevice& peripheral);

    bool connectedAsCentral(void) const {
        return peripherals_.size() > 0;
    }

protected:
    BlePeerDevice* peripheralAt(size_t i);
    void centralProcessConnected(const BlePeerDevice& peer);
    void centralProcessDisconnected(const BlePeerDevice& peer);

private:
    BleConnParams connParams_;
    Vector<BlePeerDevice> peripherals_;
};


class BlePeerDevice : public BleGattServer {
public:
    BleDevRoles role;
    BleAddress address;
    BleConnParams connParams;
    BleConnHandle connHandle;
    int8_t rssi;

    BlePeerDevice();
    ~BlePeerDevice();

    bool operator == (const BlePeerDevice& device);
};


class BleLocalDevice : public BleBroadcaster,
                       public BleObserver,
                       public BlePeripheral,
                       public BleCentral,
                       public BleGattServer,
                       public BleGattClient {
public:
    typedef void (*onConnectedCb)(BlePeerDevice &peer);
    typedef void (*onDisconnectedCb)(BlePeerDevice &peer);

    BleAddress address;

    int on(void);
    void off(void);

    void onConnectionChangedCb(onConnectedCb connCb, onDisconnectedCb disconnCb);

    static BleLocalDevice* getInstance(void);

private:
    onConnectedCb connectedCb_;
    onDisconnectedCb disconnectedCb_;

    BleLocalDevice() : connectedCb_(nullptr), disconnectedCb_(nullptr) {
        device = this;
    }
    ~BleLocalDevice() {

    }

    static void onBleEvents(hal_ble_evts_t *event, void* context);
    BlePeerDevice* findPeerDevice(BleConnHandle connHandle);
};


extern BleLocalDevice& _fetch_ble();
#define BLE _fetch_ble()


#endif /* Wiring_BLE */

#endif /* SPARK_WIRING_BLE_H */