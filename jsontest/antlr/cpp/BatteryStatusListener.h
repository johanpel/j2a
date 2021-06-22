
// Generated from BatteryStatus.g4 by ANTLR 4.9

#pragma once


#include "antlr4-runtime.h"
#include "BatteryStatusParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by BatteryStatusParser.
 */
class  BatteryStatusListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterJson(BatteryStatusParser::JsonContext *ctx) = 0;
  virtual void exitJson(BatteryStatusParser::JsonContext *ctx) = 0;

  virtual void enterBattery(BatteryStatusParser::BatteryContext *ctx) = 0;
  virtual void exitBattery(BatteryStatusParser::BatteryContext *ctx) = 0;

  virtual void enterVoltage(BatteryStatusParser::VoltageContext *ctx) = 0;
  virtual void exitVoltage(BatteryStatusParser::VoltageContext *ctx) = 0;

  virtual void enterArr(BatteryStatusParser::ArrContext *ctx) = 0;
  virtual void exitArr(BatteryStatusParser::ArrContext *ctx) = 0;


};

