
// Generated from BatteryStatus.g4 by ANTLR 4.9

#pragma once


#include "antlr4-runtime.h"
#include "BatteryStatusListener.h"


/**
 * This class provides an empty implementation of BatteryStatusListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  BatteryStatusBaseListener : public BatteryStatusListener {
public:

  virtual void enterJson(BatteryStatusParser::JsonContext * /*ctx*/) override { }
  virtual void exitJson(BatteryStatusParser::JsonContext * /*ctx*/) override { }

  virtual void enterBattery(BatteryStatusParser::BatteryContext * /*ctx*/) override { }
  virtual void exitBattery(BatteryStatusParser::BatteryContext * /*ctx*/) override { }

  virtual void enterVoltage(BatteryStatusParser::VoltageContext * /*ctx*/) override { }
  virtual void exitVoltage(BatteryStatusParser::VoltageContext * /*ctx*/) override { }

  virtual void enterArr(BatteryStatusParser::ArrContext * /*ctx*/) override { }
  virtual void exitArr(BatteryStatusParser::ArrContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

