
// Generated from BatteryStatus.g4 by ANTLR 4.9


#include "BatteryStatusListener.h"

#include "BatteryStatusParser.h"


using namespace antlrcpp;
using namespace antlr4;

BatteryStatusParser::BatteryStatusParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

BatteryStatusParser::~BatteryStatusParser() {
  delete _interpreter;
}

std::string BatteryStatusParser::getGrammarFileName() const {
  return "BatteryStatus.g4";
}

const std::vector<std::string>& BatteryStatusParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& BatteryStatusParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- JsonContext ------------------------------------------------------------------

BatteryStatusParser::JsonContext::JsonContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<BatteryStatusParser::BatteryContext *> BatteryStatusParser::JsonContext::battery() {
  return getRuleContexts<BatteryStatusParser::BatteryContext>();
}

BatteryStatusParser::BatteryContext* BatteryStatusParser::JsonContext::battery(size_t i) {
  return getRuleContext<BatteryStatusParser::BatteryContext>(i);
}


size_t BatteryStatusParser::JsonContext::getRuleIndex() const {
  return BatteryStatusParser::RuleJson;
}

void BatteryStatusParser::JsonContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterJson(this);
}

void BatteryStatusParser::JsonContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitJson(this);
}

BatteryStatusParser::JsonContext* BatteryStatusParser::json() {
  JsonContext *_localctx = _tracker.createInstance<JsonContext>(_ctx, getState());
  enterRule(_localctx, 0, BatteryStatusParser::RuleJson);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(11);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == BatteryStatusParser::T__0) {
      setState(8);
      battery();
      setState(13);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BatteryContext ------------------------------------------------------------------

BatteryStatusParser::BatteryContext::BatteryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

BatteryStatusParser::VoltageContext* BatteryStatusParser::BatteryContext::voltage() {
  return getRuleContext<BatteryStatusParser::VoltageContext>(0);
}


size_t BatteryStatusParser::BatteryContext::getRuleIndex() const {
  return BatteryStatusParser::RuleBattery;
}

void BatteryStatusParser::BatteryContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBattery(this);
}

void BatteryStatusParser::BatteryContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBattery(this);
}

BatteryStatusParser::BatteryContext* BatteryStatusParser::battery() {
  BatteryContext *_localctx = _tracker.createInstance<BatteryContext>(_ctx, getState());
  enterRule(_localctx, 2, BatteryStatusParser::RuleBattery);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(14);
    match(BatteryStatusParser::T__0);
    setState(15);
    voltage();
    setState(16);
    match(BatteryStatusParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VoltageContext ------------------------------------------------------------------

BatteryStatusParser::VoltageContext::VoltageContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

BatteryStatusParser::ArrContext* BatteryStatusParser::VoltageContext::arr() {
  return getRuleContext<BatteryStatusParser::ArrContext>(0);
}


size_t BatteryStatusParser::VoltageContext::getRuleIndex() const {
  return BatteryStatusParser::RuleVoltage;
}

void BatteryStatusParser::VoltageContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVoltage(this);
}

void BatteryStatusParser::VoltageContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVoltage(this);
}

BatteryStatusParser::VoltageContext* BatteryStatusParser::voltage() {
  VoltageContext *_localctx = _tracker.createInstance<VoltageContext>(_ctx, getState());
  enterRule(_localctx, 4, BatteryStatusParser::RuleVoltage);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(18);
    match(BatteryStatusParser::T__2);
    setState(19);
    match(BatteryStatusParser::T__3);
    setState(20);
    arr();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ArrContext ------------------------------------------------------------------

BatteryStatusParser::ArrContext::ArrContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> BatteryStatusParser::ArrContext::NUMBER() {
  return getTokens(BatteryStatusParser::NUMBER);
}

tree::TerminalNode* BatteryStatusParser::ArrContext::NUMBER(size_t i) {
  return getToken(BatteryStatusParser::NUMBER, i);
}


size_t BatteryStatusParser::ArrContext::getRuleIndex() const {
  return BatteryStatusParser::RuleArr;
}

void BatteryStatusParser::ArrContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArr(this);
}

void BatteryStatusParser::ArrContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<BatteryStatusListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArr(this);
}

BatteryStatusParser::ArrContext* BatteryStatusParser::arr() {
  ArrContext *_localctx = _tracker.createInstance<ArrContext>(_ctx, getState());
  enterRule(_localctx, 6, BatteryStatusParser::RuleArr);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(34);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(22);
      match(BatteryStatusParser::T__4);
      setState(23);
      match(BatteryStatusParser::NUMBER);
      setState(28);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == BatteryStatusParser::T__5) {
        setState(24);
        match(BatteryStatusParser::T__5);
        setState(25);
        match(BatteryStatusParser::NUMBER);
        setState(30);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(31);
      match(BatteryStatusParser::T__6);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(32);
      match(BatteryStatusParser::T__4);
      setState(33);
      match(BatteryStatusParser::T__6);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

// Static vars and initialization.
std::vector<dfa::DFA> BatteryStatusParser::_decisionToDFA;
atn::PredictionContextCache BatteryStatusParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN BatteryStatusParser::_atn;
std::vector<uint16_t> BatteryStatusParser::_serializedATN;

std::vector<std::string> BatteryStatusParser::_ruleNames = {
  "json", "battery", "voltage", "arr"
};

std::vector<std::string> BatteryStatusParser::_literalNames = {
  "", "'{'", "'}'", "'voltage'", "':'", "'['", "','", "']'"
};

std::vector<std::string> BatteryStatusParser::_symbolicNames = {
  "", "", "", "", "", "", "", "", "STRING", "NUMBER", "WS"
};

dfa::Vocabulary BatteryStatusParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> BatteryStatusParser::_tokenNames;

BatteryStatusParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  _serializedATN = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
    0x3, 0xc, 0x27, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 0x9, 
    0x4, 0x4, 0x5, 0x9, 0x5, 0x3, 0x2, 0x7, 0x2, 0xc, 0xa, 0x2, 0xc, 0x2, 
    0xe, 0x2, 0xf, 0xb, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 
    0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 
    0x5, 0x7, 0x5, 0x1d, 0xa, 0x5, 0xc, 0x5, 0xe, 0x5, 0x20, 0xb, 0x5, 0x3, 
    0x5, 0x3, 0x5, 0x3, 0x5, 0x5, 0x5, 0x25, 0xa, 0x5, 0x3, 0x5, 0x2, 0x2, 
    0x6, 0x2, 0x4, 0x6, 0x8, 0x2, 0x2, 0x2, 0x25, 0x2, 0xd, 0x3, 0x2, 0x2, 
    0x2, 0x4, 0x10, 0x3, 0x2, 0x2, 0x2, 0x6, 0x14, 0x3, 0x2, 0x2, 0x2, 0x8, 
    0x24, 0x3, 0x2, 0x2, 0x2, 0xa, 0xc, 0x5, 0x4, 0x3, 0x2, 0xb, 0xa, 0x3, 
    0x2, 0x2, 0x2, 0xc, 0xf, 0x3, 0x2, 0x2, 0x2, 0xd, 0xb, 0x3, 0x2, 0x2, 
    0x2, 0xd, 0xe, 0x3, 0x2, 0x2, 0x2, 0xe, 0x3, 0x3, 0x2, 0x2, 0x2, 0xf, 
    0xd, 0x3, 0x2, 0x2, 0x2, 0x10, 0x11, 0x7, 0x3, 0x2, 0x2, 0x11, 0x12, 
    0x5, 0x6, 0x4, 0x2, 0x12, 0x13, 0x7, 0x4, 0x2, 0x2, 0x13, 0x5, 0x3, 
    0x2, 0x2, 0x2, 0x14, 0x15, 0x7, 0x5, 0x2, 0x2, 0x15, 0x16, 0x7, 0x6, 
    0x2, 0x2, 0x16, 0x17, 0x5, 0x8, 0x5, 0x2, 0x17, 0x7, 0x3, 0x2, 0x2, 
    0x2, 0x18, 0x19, 0x7, 0x7, 0x2, 0x2, 0x19, 0x1e, 0x7, 0xb, 0x2, 0x2, 
    0x1a, 0x1b, 0x7, 0x8, 0x2, 0x2, 0x1b, 0x1d, 0x7, 0xb, 0x2, 0x2, 0x1c, 
    0x1a, 0x3, 0x2, 0x2, 0x2, 0x1d, 0x20, 0x3, 0x2, 0x2, 0x2, 0x1e, 0x1c, 
    0x3, 0x2, 0x2, 0x2, 0x1e, 0x1f, 0x3, 0x2, 0x2, 0x2, 0x1f, 0x21, 0x3, 
    0x2, 0x2, 0x2, 0x20, 0x1e, 0x3, 0x2, 0x2, 0x2, 0x21, 0x25, 0x7, 0x9, 
    0x2, 0x2, 0x22, 0x23, 0x7, 0x7, 0x2, 0x2, 0x23, 0x25, 0x7, 0x9, 0x2, 
    0x2, 0x24, 0x18, 0x3, 0x2, 0x2, 0x2, 0x24, 0x22, 0x3, 0x2, 0x2, 0x2, 
    0x25, 0x9, 0x3, 0x2, 0x2, 0x2, 0x5, 0xd, 0x1e, 0x24, 
  };

  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

BatteryStatusParser::Initializer BatteryStatusParser::_init;
