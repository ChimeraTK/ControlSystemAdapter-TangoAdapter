#ifndef CHIMERATK_ATTRIBUT_PROPERTY_H
#define CHIMERATK_ATTRIBUT_PROPERTY_H

#include <iostream>
#include <map>
#include <tango.h>
#include <boost/algorithm/string/split.hpp> 
#include <boost/algorithm/string/classification.hpp> 
#define TOKEN ";"

namespace ChimeraTK {
enum AttrDataFormat{SCALAR,SPECTRUM,IMAGE};


  class AttributProperty {
   public:
    //Speed;Board/Reg;SCALAR;DEVShort 
    AttributProperty(std::string name,std::string path,AttrDataFormat dataFormat, long dataType,std::string desc,std::string unit){

      _name = name;
      _path = path;
      _attrDataFormat = dataFormat;
      _dataType = dataType;

      _desc = desc;
      _unit = unit;
    }
    AttributProperty(std::string desc){
      std::vector<std::string> splitDesc;
      boost::algorithm::split(splitDesc, desc, boost::is_any_of(TOKEN));
      if (splitDesc.size()!=6)
      {
        std::cout<<"error AttributProperty"<<std::endl;

      }
      _name = splitDesc[0];
      _path = splitDesc[1];
      _attrDataFormat = regTypeMap[splitDesc[2]];

    if  (!splitDesc[3].compare("DevUChar"))
      _dataType = Tango::DEV_UCHAR;
    if  (!splitDesc[3].compare("DevUShort"))
      _dataType = Tango::DEV_USHORT;
    else if (!splitDesc[3].compare("DevULong"))
      _dataType = Tango::DEV_ULONG;
    else if (!splitDesc[3].compare("DevULong64"))
      _dataType = Tango::DEV_ULONG64;
    else if (!splitDesc[3].compare("DevShort"))
      _dataType = Tango::DEV_SHORT;
    else if (!splitDesc[3].compare("DevLong"))
      _dataType = Tango::DEV_LONG;
    else if (!splitDesc[3].compare("DevLong64"))
      _dataType = Tango::DEV_LONG64;
    else if (!splitDesc[3].compare("DevFloat"))
      _dataType = Tango::DEV_FLOAT;
    else if (!splitDesc[3].compare("DevDouble"))
      _dataType = Tango::DEV_DOUBLE;
    else if (!splitDesc[3].compare("DevBoolean"))
      _dataType = Tango::DEV_BOOLEAN;
    else if (!splitDesc[3].compare("DevString"))
    {
      _dataType = Tango::DEV_STRING;
    }

      _desc = splitDesc[4];
      _unit = splitDesc[5];

    }

    ~AttributProperty(){}

    void operator=(AttributProperty const&) = delete;

    long getTangoType(){return _dataType;} 

    ChimeraTK::AttrDataFormat getDataFormat(){return _attrDataFormat;} 
    
    
    std::map<std::string,ChimeraTK::AttrDataFormat>regTypeMap={ {"SCALAR", ChimeraTK::AttrDataFormat::SCALAR}, {"SPECTRUM",ChimeraTK::AttrDataFormat::SPECTRUM },{"IMAGE",ChimeraTK::AttrDataFormat::IMAGE} };    
    public:
    std::string _unit;  
    std::string _desc;
    std::string _name;  
    std::string _path;
    ChimeraTK::AttrDataFormat _attrDataFormat;
    long _dataType;
    Tango::AttrWriteType _writeType;    
 };


} // namespace ChimeraTK

#endif // CHIMERATK_ATTRIBUT_PROPERTY_H
