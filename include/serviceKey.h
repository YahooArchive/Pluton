#ifndef _SERVICEKEY_H
#define _SERVICEKEY_H

#include <string>

#include "pluton/common.h"

//////////////////////////////////////////////////////////////////////
// Encapsulate the serviceKey definitions as they are vulnerable to
// change as the design evolves.
//////////////////////////////////////////////////////////////////////

namespace pluton {
  class serviceKey {
  public:
    serviceKey(const std::string& name="", const std::string& function="",
	       pluton::serializationType sType=pluton::raw,
	       unsigned int sVersion=0);

    void	setApplication(const std::string& newApplication)
      {
	_applicationStr = newApplication;
	_regenEnglish = true;
      }
    void	setApplication(const char* p, int l)
      {
	_applicationStr.assign(p, l);
	_regenEnglish = true;
      }

    void	setFunction(const std::string& newFunction)
      {
	_functionStr = newFunction;
	_regenEnglish = true;
      }

    void	setFunction(const char* p, int l)
      {
	_functionStr.assign(p, l);
	_regenEnglish = true;
      }

    void	setVersion(unsigned int newVersion)
      {
	_version = newVersion;
	_regenEnglish = true;
      }
    void 	setSerialization(pluton::serializationType newS=pluton::raw)
      {
	_serialization = newS;
	_regenEnglish = true;
      }

    void	erase();

  // Getters

    const std::string&		getApplication() const { return _applicationStr; }
    const std::string&		getFunction() const { return _functionStr; }
    pluton::serializationType	getSerialization() const { return _serialization; }
    unsigned int		getVersion() const { return _version; }

    std::string			getEnglishKey() const;
    void			getSearchKey(std::string& skLong) const;
    const char*			parse(const char*, int, bool clientFlag=true);
    const char*			parse(const std::string& key, bool clientFlag=true)
      {
	return parse(key.data(), key.length(), clientFlag);
      }

  private:
    std::string			_applicationStr;
    std::string			_functionStr;
    pluton::serializationType	_serialization;
    unsigned int		_version;

    mutable bool		_regenEnglish;
    mutable std::string		_englishKey;
  };
}

#endif
