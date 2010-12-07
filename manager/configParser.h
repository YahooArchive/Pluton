#ifndef _CONFIGPARSER_H
#define _CONFIGPARSER_H

#include <string>
#include <sstream>
#include <map>

// Yes, there must be 10 million of these classes and 9.9 million of them
// are better than this one...

class configParser {

 public:
  bool	loadFile(const std::string& filename, std::string& error);
  bool	loadString(const std::string& filename, const std::string& s, std::string& error);

  bool	getBool(const std::string& name, bool defaultValue, bool& newValue, std::string& error);
  bool 	getNumber(const std::string& name, long defaultValue, long& newValue, std::string& error);
  bool	getString(const std::string& name, const std::string& defaultValue,
		  std::string& newValue, std::string& error);
  bool	found(const std::string& name) const;

  std::string	getFirstUnfetched();		// Return a config value not fetched

  void		dump();

 private:
  bool 	loadStream(const std::string& filename, std::istream& is, std::string& error);

  std::string	configPath;

  struct parameter {
    bool	fetched;
    int		lineNumber;
    std::string	value;
  };

  std::map<std::string, parameter>				paramMap;
  typedef std::map<std::string, parameter>::iterator		paramIter;
  typedef std::map<std::string, parameter>::const_iterator	paramConstIter;
};

#endif
