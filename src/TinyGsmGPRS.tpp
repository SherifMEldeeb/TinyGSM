/**
 * @file       TinyGsmGPRS.tpp
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef SRC_TINYGSMGPRS_H_
#define SRC_TINYGSMGPRS_H_

#include "TinyGsmCommon.h"

#define TINY_GSM_MODEM_HAS_GPRS

enum SimStatus {
  SIM_ERROR            = 0,
  SIM_READY            = 1,
  SIM_LOCKED           = 2,
  SIM_ANTITHEFT_LOCKED = 3,
};

template <class modemType>
class TinyGsmGPRS {
 public:
  /*
   * SIM card functions
   */
  // Unlocks the SIM
  bool simUnlock(const char* pin) const {
    return thisModem().simUnlockImpl(pin);
  }
  // Gets the CCID of a sim card via AT+CCID
  std::string getSimCCID() const {
    return thisModem().getSimCCIDImpl();
  }
  // Asks for TA Serial Number Identification (IMEI)
  String getIMEI() const {
    return thisModem().getIMEIImpl();
  }
  // Asks for International Mobile Subscriber Identity IMSI
  String getIMSI() const {
    return thisModem().getIMSIImpl();
  }
  SimStatus getSimStatus(uint32_t timeout_ms = 10000L) const {
    return thisModem().getSimStatusImpl(timeout_ms);
  }

  /*
   * GPRS functions
   */
  bool gprsConnect(const char* apn, const char* user = NULL,
                   const char* pwd = NULL) const {
    return thisModem().gprsConnectImpl(apn, user, pwd);
  }
  bool gprsDisconnect() const {
    return thisModem().gprsDisconnectImpl();
  }
  // Checks if current attached to GPRS/EPS service
  bool isGprsConnected() const {
    return thisModem().isGprsConnectedImpl();
  }
  // Gets the current network operator
  String getOperator() const {
    return thisModem().getOperatorImpl();
  }

  /*
   * CRTP Helper
   */
 protected:
  inline const modemType& thisModem() const {
    return static_cast<const modemType&>(*this);
  }
  inline modemType& thisModem() {
    return static_cast<modemType&>(*this);
  }

  /*
   * SIM card functions
   */
 protected:
  // Unlocks a sim via the 3GPP TS command AT+CPIN
  bool simUnlockImpl(const char* pin) const {
    if (pin && strlen(pin) > 0) {
      thisModem().sendAT(GF("+CPIN=\""), pin, GF("\""));
      return thisModem().waitResponse() == 1;
    }
    return true;
  }

  // Gets the CCID of a sim card via AT+CCID
  std::string getSimCCIDImpl() const {
    thisModem().sendAT(GF("+CCID"));
    if (thisModem().waitResponse(GF("+CCID:")) != 1) { return ""; }
    std::string res = thisModem().stream.readStringUntil('\n').c_str();
    thisModem().waitResponse();
//    res.trim();
    return res;
  }

  // Asks for TA Serial Number Identification (IMEI) via the V.25TER standard
  // AT+GSN command
  String getIMEIImpl() const {
    thisModem().sendAT(GF("+GSN"));
    thisModem().streamSkipUntil('\n');  // skip first newline
    String res = thisModem().stream.readStringUntil('\n');
    thisModem().waitResponse();
    res.trim();
    return res;
  }

  // Asks for International Mobile Subscriber Identity IMSI via the AT+CIMI
  // command
  String getIMSIImpl() const {
    thisModem().sendAT(GF("+CIMI"));
    thisModem().streamSkipUntil('\n');  // skip first newline
    String res = thisModem().stream.readStringUntil('\n');
    thisModem().waitResponse();
    res.trim();
    return res;
  }

  SimStatus getSimStatusImpl(uint32_t timeout_ms = 10000L) const {
    for (uint32_t start = millis(); millis() - start < timeout_ms;) {
      thisModem().sendAT(GF("+CPIN?"));
      if (thisModem().waitResponse(GF("+CPIN:")) != 1) {
        delay(1000);
        continue;
      }
      int8_t status =
          thisModem().waitResponse(GF("READY"), GF("SIM PIN"), GF("SIM PUK"),
                                   GF("NOT INSERTED"), GF("NOT READY"));
      thisModem().waitResponse();
      switch (status) {
        case 2:
        case 3: return SIM_LOCKED;
        case 1: return SIM_READY;
        default: return SIM_ERROR;
      }
    }
    return SIM_ERROR;
  }

  /*
   * GPRS functions
   */
 protected:
  // Checks if current attached to GPRS/EPS service
  bool isGprsConnectedImpl() const {
    thisModem().sendAT(GF("+CGATT?"));
    if (thisModem().waitResponse(GF("+CGATT:")) != 1) { return false; }
    int8_t res = thisModem().streamGetIntBefore('\n');
    thisModem().waitResponse();
    if (res != 1) { return false; }

    return thisModem().localIP() != IPAddress(0, 0, 0, 0);
  }

  // Gets the current network operator via the 3GPP TS command AT+COPS
  String getOperatorImpl() const {
    thisModem().sendAT(GF("+COPS?"));
    if (thisModem().waitResponse(GF("+COPS:")) != 1) { return ""; }
    thisModem().streamSkipUntil('"'); /* Skip mode and format */
    String res = thisModem().stream.readStringUntil('"');
    thisModem().waitResponse();
    return res;
  }
};

#endif  // SRC_TINYGSMGPRS_H_
