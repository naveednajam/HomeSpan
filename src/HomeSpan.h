
#pragma once

#ifndef ARDUINO_ARCH_ESP32
#error ERROR: HOMESPAN IS ONLY AVAILABLE FOR ESP32 MICROCONTROLLERS!
#endif

#include <Arduino.h>
#include <unordered_map>

#include "Settings.h"
#include "Utils.h"
#include "Network.h"
#include "HAPConstants.h"

using std::vector;
using std::unordered_map;

enum {
  GET_AID=1,
  GET_META=2,
  GET_PERMS=4,
  GET_TYPE=8,
  GET_EV=16,
  GET_DESC=32,
  GET_NV=64,
  GET_ALL=255
};

// Forward-Declarations

struct Span;
struct SpanAccessory;
struct SpanService;
struct SpanCharacteristic;
struct SpanRange;
struct SpanBuf;
struct SpanButton;

///////////////////////////////

struct SpanConfig {                         
  int configNumber=0;                         // configuration number - broadcast as Bonjour "c#" (computed automatically)
  uint8_t hashCode[48]={0};                   // SHA-384 hash of Span Database stored as a form of unique "signature" to know when to update the config number upon changes
};

///////////////////////////////

struct Span{

  char *displayName;                            // display name for this device - broadcast as part of Bonjour MDNS
  char *hostNameBase;                           // base of host name of this device - full host name broadcast by Bonjour MDNS will have 6-byte accessoryID as well as '.local' automatically appended
  char *hostName;                               // full host name of this device - constructed from hostNameBase and 6-byte AccessoryID
  char *modelName;                              // model name of this device - broadcast as Bonjour field "md" 
  char category[3]="";                          // category ID of primary accessory - broadcast as Bonjour field "ci" (HAP Section 13)
  unsigned long snapTime;                       // current time (in millis) snapped before entering Service loops() or updates()
  boolean isInitialized=false;                  // flag indicating HomeSpan has been initialized
  int nFatalErrors=0;                           // number of fatal errors in user-defined configuration
  String configLog;                             // log of configuration process, including any errors
  boolean isBridge=true;                        // flag indicating whether device is configured as a bridge (i.e. first Accessory contains nothing but AccessoryInformation and HAPProtocolInformation)
  
  char *defaultSetupCode=DEFAULT_SETUP_CODE;                  // Setup Code used for pairing
  uint8_t statusPin=DEFAULT_STATUS_PIN;                       // pin for status LED    
  uint8_t controlPin=DEFAULT_CONTROL_PIN;                     // pin for Control Pushbutton
  uint8_t logLevel=DEFAULT_LOG_LEVEL;                         // level for writing out log messages to serial monitor
  uint8_t maxConnections=DEFAULT_MAX_CONNECTIONS;             // number of simultaneous HAP connections
  unsigned long comModeLife=DEFAULT_COMMAND_TIMEOUT*1000;     // length of time (in milliseconds) to keep Command Mode alive before resuming normal operations

  Blinker statusLED;                                // indicates HomeSpan status
  PushButton controlButton;                         // controls HomeSpan configuration and resets
  Network network;                                  // configures WiFi and Setup Code via either serial monitor or temporary Access Point
    
  SpanConfig hapConfig;                             // track configuration changes to the HAP Accessory database; used to increment the configuration number (c#) when changes found
  vector<SpanAccessory *> Accessories;              // vector of pointers to all Accessories
  vector<SpanService *> Loops;                      // vector of pointer to all Services that have over-ridden loop() methods
  vector<SpanBuf> Notifications;                    // vector of SpanBuf objects that store info for Characteristics that are updated with setVal() and require a Notification Event
  vector<SpanButton *> PushButtons;                 // vector of pointer to all PushButtons
  unordered_map<uint64_t, uint32_t> TimedWrites;    // map of timed-write PIDs and Alarm Times (based on TTLs)

  HapCharList chr;                                  // list of all HAP Characteristics

  void begin(Category catID,
             char *displayName=DEFAULT_DISPLAY_NAME,
             char *hostNameBase=DEFAULT_HOST_NAME,
             char *modelName=DEFAULT_MODEL_NAME);        
             
  void poll();                                  // poll HAP Clients and process any new HAP requests
  int getFreeSlot();                            // returns free HAPClient slot number. HAPClients slot keep track of each active HAPClient connection
  void initWifi();                              // initialize and connect to WiFi network
  void commandMode();                           // allows user to control and reset HomeSpan settings with the control button
  void processSerialCommand(char *c);           // process command 'c' (typically from readSerial, though can be called with any 'c')

  int sprintfAttributes(char *cBuf);            // prints Attributes JSON database into buf, unless buf=NULL; return number of characters printed, excluding null terminator, even if buf=NULL
  void prettyPrint(char *buf, int nsp=2);       // print arbitrary JSON from buf to serial monitor, formatted with indentions of 'nsp' spaces
  SpanCharacteristic *find(int aid, int iid);   // return Characteristic with matching aid and iid (else NULL if not found)
  
  int countCharacteristics(char *buf);                                    // return number of characteristic objects referenced in PUT /characteristics JSON request
  int updateCharacteristics(char *buf, SpanBuf *pObj);                    // parses PUT /characteristics JSON request 'buf into 'pObj' and updates referenced characteristics; returns 1 on success, 0 on fail
  int sprintfAttributes(SpanBuf *pObj, int nObj, char *cBuf);             // prints SpanBuf object into buf, unless buf=NULL; return number of characters printed, excluding null terminator, even if buf=NULL
  int sprintfAttributes(char **ids, int numIDs, int flags, char *cBuf);   // prints accessory.characteristic ids into buf, unless buf=NULL; return number of characters printed, excluding null terminator, even if buf=NULL

  void clearNotify(int slotNum);                                          // set ev notification flags for connection 'slotNum' to false across all characteristics 
  int sprintfNotify(SpanBuf *pObj, int nObj, char *cBuf, int conNum);     // prints notification JSON into buf based on SpanBuf objects and specified connection number

  void setControlPin(uint8_t pin){controlPin=pin;}                        // sets Control Pin
  void setStatusPin(uint8_t pin){statusPin=pin;}                          // sets Status Pin
  void setApSSID(char *ssid){network.apSSID=ssid;}                        // sets Access Point SSID
  void setApPassword(char *pwd){network.apPassword=pwd;}                  // sets Access Point Password
  void setApTimeout(uint16_t nSec){network.lifetime=nSec*1000;}           // sets Access Point Timeout (seconds)
  void setCommandTimeout(uint16_t nSec){comModeLife=nSec*1000;}           // sets Command Mode Timeout (seconds)
  void setLogLevel(uint8_t level){logLevel=level;}                        // sets Log Level for log messages (0=baseline, 1=intermediate, 2=all)
  void setMaxConnections(uint8_t nCon){maxConnections=nCon;}              // sets maximum number of simultaneous HAP connections (HAP requires devices support at least 8)
};

///////////////////////////////

struct SpanAccessory{
    
  int aid=0;                                // Accessory Instance ID (HAP Table 6-1)
  int iidCount=0;                           // running count of iid to use for Services and Characteristics associated with this Accessory                                 
  vector<SpanService *> Services;           // vector of pointers to all Services in this Accessory  

  SpanAccessory();

  int sprintfAttributes(char *cBuf);        // prints Accessory JSON database into buf, unless buf=NULL; return number of characters printed, excluding null terminator, even if buf=NULL  
  void validate();                          // error-checks Accessory
};

///////////////////////////////

struct SpanService{

  int iid=0;                                              // Instance ID (HAP Table 6-2)
  const char *type;                                       // Service Type
  const char *hapName;                                    // HAP Name
  boolean hidden=false;                                   // optional property indicating service is hidden
  boolean primary=false;                                  // optional property indicating service is primary
  vector<SpanCharacteristic *> Characteristics;           // vector of pointers to all Characteristics in this Service  
  vector<HapCharType *> req;                              // vector of pointers to all required HAP Characteristic Types for this Service
  vector<HapCharType *> opt;                              // vector of pointers to all optional HAP Characteristic Types for this Service
  
  SpanService(const char *type, const char *hapName);

  SpanService *setPrimary();                              // sets the Service Type to be primary and returns pointer to self
  SpanService *setHidden();                               // sets the Service Type to be hidden and returns pointer to self

  int sprintfAttributes(char *cBuf);                      // prints Service JSON records into buf; return number of characters printed, excluding null terminator
  void validate();                                        // error-checks Service
  
  virtual boolean update() {return(true);}                // placeholder for code that is called when a Service is updated via a Controller.  Must return true/false depending on success of update
  virtual void loop(){}                                   // loops for each Service - called every cycle and can be over-ridden with user-defined code
  virtual void button(int pin, int pressType){}           // method called for a Service when a button attached to "pin" has a Single, Double, or Long Press, according to pressType
};

///////////////////////////////

struct SpanCharacteristic{

  enum {          // create bitflags based on HAP Table 6-4
    PR=1,
    PW=2,
    EV=4,
    AA=8,
    TW=16,
    HD=32,
    WR=64,
    NV=128
  };

  enum FORMAT {   // HAP Table 6-5
    BOOL=0,
    UINT8=1,
    UINT16=2,
    UINT32=3,
    UINT64=4,
    INT=5,
    FLOAT=6,
    STRING=7
  };
    
  union UVal {                                  
    boolean BOOL;
    uint8_t UINT8;
    uint16_t UINT16;
    uint32_t UINT32;
    uint64_t UINT64;
    int32_t INT;
    double FLOAT;
    const char *STRING;      
  };
     
  int iid=0;                               // Instance ID (HAP Table 6-3)
  char *type;                              // Characteristic Type
  const char *hapName;                     // HAP Name
  UVal value;                              // Characteristic Value
  uint8_t perms;                           // Characteristic Permissions
  FORMAT format;                           // Characteristic Format        
  char *desc=NULL;                         // Characteristic Description (optional)
  SpanRange *range=NULL;                   // Characteristic min/max/step; NULL = default values (optional)
  boolean *ev;                             // Characteristic Event Notify Enable (per-connection)
  
  int aid=0;                               // Accessory ID - passed through from Service containing this Characteristic
  boolean isUpdated=false;                 // set to true when new value has been requested by PUT /characteristic
  unsigned long updateTime;                // last time value was updated (in millis) either by PUT /characteristic OR by setVal()
  UVal newValue;                           // the updated value requested by PUT /characteristic
  SpanService *service=NULL;               // pointer to Service containing this Characteristic
      
  SpanCharacteristic(char *type, uint8_t perms, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, boolean value, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, uint8_t value, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, uint16_t value, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, uint32_t value, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, uint64_t value, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, int32_t value, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, double value, char *hapName);
  SpanCharacteristic(char *type, uint8_t perms, const char* value, char *hapName);

  int sprintfAttributes(char *cBuf, int flags);   // prints Characteristic JSON records into buf, according to flags mask; return number of characters printed, excluding null terminator  
  StatusCode loadUpdate(char *val, char *ev);     // load updated val/ev from PUT /characteristic JSON request.  Return intiial HAP status code (checks to see if characteristic is found, is writable, etc.)
  
  template <class T=int> T getVal(){return(getValue<T>(value));}                    // returns UVal value
  template <class T=int> T getNewVal(){return(getValue<T>(newValue));}              // returns UVal newValue
  template <class T> T getValue(UVal v);                                            // returns UVal v

  void setVal(int value);                                                           // sets value of UVal value for all integer-based Characterstic types
  void setVal(double value);                                                        // sets value of UVal value for FLOAT Characteristic type

  boolean updated(){return(isUpdated);}                                             // returns isUpdated
  int timeVal();                                                                    // returns time elapsed (in millis) since value was last updated
  
};

///////////////////////////////

template <class T> T SpanCharacteristic::getValue(UVal v){

  switch(format){
    case BOOL:
      return((T) v.BOOL);
    case INT:
      return((T) v.INT);
    case UINT8:
      return((T) v.UINT8);
    case UINT16:
      return((T) v.UINT16);
    case UINT32:
      return((T) v.UINT32);
    case UINT64:
      return((T) v.UINT64);
    case FLOAT:
      return((T) v.FLOAT);
    case STRING:
      Serial.print("*** ERROR:  Can't use getVal() or getNewVal() for string Characteristics.\n\n");
      return(0);
  }
    
};

///////////////////////////////

struct SpanRange{
  int min;
  int max;
  int step;

  SpanRange(int min, int max, int step);
};

///////////////////////////////

struct SpanBuf{                               // temporary storage buffer for use with putCharacteristicsURL() and checkTimedResets() 
  int aid=0;                                  // updated aid 
  int iid=0;                                  // updated iid
  char *val=NULL;                             // updated value (optional, though either at least 'val' or 'ev' must be specified)
  char *ev=NULL;                              // updated event notification flag (optional, though either at least 'val' or 'ev' must be specified)
  StatusCode status;                          // return status (HAP Table 6-11)
  SpanCharacteristic *characteristic=NULL;    // Characteristic to update (NULL if not found)
};
  
///////////////////////////////

struct SpanButton{

  enum {
    SINGLE=0,
    DOUBLE=1,
    LONG=2
  };
  
  int pin;                       // pin number  
  uint16_t singleTime;           // minimum time (in millis) required to register a single press
  uint16_t longTime;             // minimum time (in millis) required to register a long press
  uint16_t doubleTime;           // maximum time (in millis) between single presses to register a double press instead
  SpanService *service;          // Service to which this PushButton is attached

  PushButton *pushButton;        // PushButton associated with this SpanButton

  SpanButton(int pin, uint16_t longTime=2000, uint16_t singleTime=5, uint16_t doubleTime=200);
};

/////////////////////////////////////////////////
// Extern Variables

extern Span homeSpan;

/////////////////////////////////////////////////

#include "Services.h"
