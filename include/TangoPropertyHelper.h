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
// PropertyHelper::set_property
//-------------------------------------------------------------------------
template <class T>
void set_property(Tango::DeviceImpl* dev_p, const std::string& property_name, T value)
{
  if (!Tango::Util::instance()->_UseDb)
  {
    //- rethrow exception
    Tango::Except::throw_exception(static_cast<const char*>("TANGO_DEVICE_ERROR"),
                                   static_cast<const char*>("NO DB"),
                                   static_cast<const char*>("PropertyHelper::set_property"));
  }

  Tango::DbDatum current_value(property_name);
  current_value << value;
  Tango::DbData db_data;
  db_data.push_back(current_value);
  try
  {
    dev_p->get_db_device()->put_property(db_data);
  }
  catch (Tango::DevFailed &df)
  {
    //- rethrow exception
    Tango::Except::re_throw_exception(df,
                                      static_cast<const char*>("TANGO_DEVICE_ERROR"),
                                      static_cast<const char*>(std::string(df.errors[0].desc).c_str()),
                                      static_cast<const char*>("PropertyHelper::set_property"));
  }
}

//-------------------------------------------------------------------------
// PropertyHelper::get_property
//-------------------------------------------------------------------------
template <class T>
T get_property(Tango::DeviceImpl* dev_p, const std::string& property_name)
{
  if (!Tango::Util::instance()->_UseDb)
  {
    //- rethrow exception
    Tango::Except::throw_exception(static_cast<const char*>("TANGO_DEVICE_ERROR"),
                                   static_cast<const char*>("NO DB"),
                                   static_cast<const char*>("PropertyHelper::get_property"));
  }

  T value;
  Tango::DbDatum current_value(property_name);
  Tango::DbData db_data;
  db_data.push_back(current_value);
  try
  {
    dev_p->get_db_device()->get_property(db_data);
  }
  catch (Tango::DevFailed &df)
  {
    //- rethrow exception
    Tango::Except::re_throw_exception(df,
                                      static_cast<const char*>("TANGO_DEVICE_ERROR"),
                                      static_cast<const char*>(std::string(df.errors[0].desc).c_str()),
                                      static_cast<const char*>("PropertyHelper::get_property"));
  }
  db_data[0] >> value;
  return value;
}

//-------------------------------------------------------------------------
// PropertyHelper::create_property_if_empty
//-------------------------------------------------------------------------
template <class T>
void create_property_if_empty(Tango::DeviceImpl* dev_p,
                                              Tango::DbData& dev_prop,
                                              T value, std::string property_name)
{
  std::size_t iNbProperties = dev_prop.size();
  std::size_t i;
  for (i = 0; i < iNbProperties; i++)
  {
    std::string sPropertyName(dev_prop[i].name);
    if (sPropertyName == property_name)
      break;
  }
  if (i == iNbProperties)
    //# TODO: throwing a DevFailed Exception
    return;

  int iPropertyIndex = i;

  if (dev_prop[iPropertyIndex].is_empty())
  {
    Tango::DbDatum current_value(dev_prop[iPropertyIndex].name);
    current_value << value;
    Tango::DbData db_data;
    db_data.push_back(current_value);

    try
    {
      dev_p->get_db_device()->put_property(db_data);
    }
    catch (Tango::DevFailed &df)
    {
      //- rethrow exception
      Tango::Except::re_throw_exception(df,
                                        static_cast<const char*>("TANGO_DEVICE_ERROR"),
                                        static_cast<const char*>(std::string(df.errors[0].desc).c_str()),
                                        static_cast<const char*>("PropertyHelper::create_property_if_empty"));
    }
  }
}

template <class T>
std::vector<T> string_to_array(std::string memoried_value)
{
  std::vector <T> spectrum_values;
  std::stringstream property_value(memoried_value);
  std::string intermediate;
  T element;

  while(std::getline(property_value, intermediate, SEPERATOR))
  {
    std::stringstream tmp(intermediate);
    tmp >> element;
    spectrum_values.push_back(element);
  }
  return spectrum_values;
}

template <class T>
std::string array_to_string(T* values, int length)
{
    std::ostringstream os;

    for (int i=0; i<length; i++) {
      std::cout<<values[i]<<" ";
        os << values[i];
        os << SEPERATOR;
    }

    return os.str();
}

} // namespace
