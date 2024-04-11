#pragma once
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*!
 * \authors See AUTHORS file
 */

// ============================================================================
// DEPENDENCIES
// ============================================================================
#include <tango/tango.h>
#include <iostream>
#include <sstream>
#include <string>

#define SEPERATOR ';'
namespace ChimeraTK {

  //-------------------------------------------------------------------------
  // PropertyHelper::setProperty
  //-------------------------------------------------------------------------
  template<class T>
  void setProperty(Tango::DeviceImpl* dev_p, const std::string& property_name, T value) {
    if(!Tango::Util::_UseDb) {
      //- rethrow exception
      Tango::Except::throw_exception(static_cast<const char*>("TANGO_DEVICE_ERROR"), static_cast<const char*>("NO DB"),
          static_cast<const char*>("PropertyHelper::set_property"));
    }

    Tango::DbDatum current_value(property_name);
    current_value << value;
    Tango::DbData db_data;
    db_data.push_back(current_value);
    try {
      dev_p->get_db_device()->put_property(db_data);
    }
    catch(Tango::DevFailed& df) {
      //- rethrow exception
      Tango::Except::re_throw_exception(df, "TANGO_DEVICE_ERROR", df.errors[0].desc, "PropertyHelper::set_property");
    }
  }

  //-------------------------------------------------------------------------
  // PropertyHelper::getProperty
  //-------------------------------------------------------------------------
  template<class T>
  T getProperty(Tango::DeviceImpl* dev_p, const std::string& property_name) {
    if(!Tango::Util::_UseDb) {
      //- rethrow exception
      Tango::Except::throw_exception("TANGO_DEVICE_ERROR", "NO DB", "PropertyHelper::getProperty");
    }

    T value;
    Tango::DbDatum current_value(property_name);
    Tango::DbData db_data;
    db_data.push_back(current_value);
    try {
      dev_p->get_db_device()->get_property(db_data);
    }
    catch(Tango::DevFailed& df) {
      //- rethrow exception
      Tango::Except::re_throw_exception(df, "TANGO_DEVICE_ERROR", df.errors[0].desc, "PropertyHelper::get_property");
    }
    db_data[0] >> value;
    return value;
  }

  //-------------------------------------------------------------------------
  // PropertyHelper::createPropertyIfEmpty
  //-------------------------------------------------------------------------
  template<class T>
  void createPropertyIfEmpty(
      Tango::DeviceImpl* dev_p, Tango::DbData& dev_prop, T value, const std::string& property_name) {
    std::size_t iNbProperties = dev_prop.size();
    std::size_t i;
    for(i = 0; i < iNbProperties; i++) {
      std::string sPropertyName(dev_prop[i].name);
      if(sPropertyName == property_name) {
        break;
      }
    }
    if(i == iNbProperties) {
      // # TODO: throwing a DevFailed Exception
      return;
    }

    auto iPropertyIndex = i;

    if(dev_prop[iPropertyIndex].is_empty()) {
      Tango::DbDatum current_value(dev_prop[iPropertyIndex].name);
      current_value << value;
      Tango::DbData db_data;
      db_data.push_back(current_value);

      try {
        dev_p->get_db_device()->put_property(db_data);
      }
      catch(Tango::DevFailed& df) {
        //- rethrow exception
        Tango::Except::re_throw_exception(df, static_cast<const char*>("TANGO_DEVICE_ERROR"),
            static_cast<const char*>(std::string(df.errors[0].desc).c_str()),
            static_cast<const char*>("PropertyHelper::create_property_if_empty"));
      }
    }
  }

  template<class T>
  std::vector<T> stringToArray(const std::string& memoried_value) {
    std::vector<T> spectrum_values;
    std::stringstream property_value(memoried_value);
    std::string intermediate;
    T element{};

    while(std::getline(property_value, intermediate, SEPERATOR)) {
      std::stringstream tmp(intermediate);
      tmp >> element;
      spectrum_values.push_back(element);
    }
    return spectrum_values;
  }

  // FIXME: This breaks if string values contain ";"
  template<class T>
  std::string arrayToString(T* values, size_t length) {
    std::ostringstream os;

    for(size_t i = 0; i < length; i++) {
      std::cout << values[i] << " ";
      os << values[i];
      if(i < length - 1) {
        os << SEPERATOR;
      }
    }

    return os.str();
  }

} // namespace ChimeraTK
