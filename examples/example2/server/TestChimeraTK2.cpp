/*----- PROTECTED REGION ID(TestChimeraTK2.cpp) ENABLED START -----*/
//=============================================================================
//
// file :        TestChimeraTK2.cpp
//
// description : C++ source for the TestChimeraTK2 class and its commands.
//               The class is derived from Device. It represents the
//               CORBA servant object which will be accessed from the
//               network. All commands which can be executed on the
//               TestChimeraTK2 are implemented in this file.
//
// project :
//
// This file is part of Tango device class.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
//
//
//
//=============================================================================
//                This file is generated by POGO
//        (Program Obviously used to Generate tango Object)
//=============================================================================
#include <ChimeraTK/TangoAdapter.h>

#include "TestChimeraTK2.h"
#include "TestChimeraTK2Class.h"
#include "ExampleApp.h"

/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2.cpp

/**
 *  TestChimeraTK2 class description:
 *    Test of TangoAdapterfor ChimeraTK
 */

//================================================================
//  The following table gives the correspondence
//  between command and method names.
//
//  Command name  |  Method name
//================================================================
//  State         |  Inherited (no method)
//  Status        |  Inherited (no method)
//================================================================

//================================================================
//  Attributes managed is:
//================================================================
//================================================================

static ChimeraTK::ApplicationFactory<ExampleApp> theExampleAppFactory("demo_example");

namespace TestChimeraTK2_ns
{
/*----- PROTECTED REGION ID(TestChimeraTK2::namespace_starting) ENABLED START -----*/

//	static initializations

/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::namespace_starting

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::TestChimeraTK2()
 *	Description : Constructors for a Tango device
 *                implementing the classTestChimeraTK2
 */
//--------------------------------------------------------
TestChimeraTK2::TestChimeraTK2(Tango::DeviceClass *cl, std::string &s)
 : TANGO_BASE_CLASS(cl, s.c_str())
{
	/*----- PROTECTED REGION ID(TestChimeraTK2::constructor_1) ENABLED START -----*/
	init_device();

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::constructor_1
}
//--------------------------------------------------------
TestChimeraTK2::TestChimeraTK2(Tango::DeviceClass *cl, const char *s)
 : TANGO_BASE_CLASS(cl, s)
{
	/*----- PROTECTED REGION ID(TestChimeraTK2::constructor_2) ENABLED START -----*/
	init_device();

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::constructor_2
}
//--------------------------------------------------------
TestChimeraTK2::TestChimeraTK2(Tango::DeviceClass *cl, const char *s, const char *d)
 : TANGO_BASE_CLASS(cl, s, d)
{
	/*----- PROTECTED REGION ID(TestChimeraTK2::constructor_3) ENABLED START -----*/
	init_device();

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::constructor_3
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::delete_device()
 *	Description : will be called at device destruction or at init command
 */
//--------------------------------------------------------
void TestChimeraTK2::delete_device()
{
	DEBUG_STREAM << "TestChimeraTK2::delete_device() " << device_name << std::endl;
	/*----- PROTECTED REGION ID(TestChimeraTK2::delete_device) ENABLED START -----*/

	//	Delete device allocated objects
	delete tangoAdapter;
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::delete_device
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::init_device()
 *	Description : will be called at device initialization.
 */
//--------------------------------------------------------
void TestChimeraTK2::init_device()
{
	DEBUG_STREAM << "TestChimeraTK2::init_device() create device " << device_name << std::endl;
	/*----- PROTECTED REGION ID(TestChimeraTK2::init_device_before) ENABLED START -----*/

	//	Initialization before get_device_property() call

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::init_device_before


	//	Get the device properties from database
	get_device_property();

	/*----- PROTECTED REGION ID(TestChimeraTK2::init_device) ENABLED START -----*/

	//	Initialize device
    //set DMapFilePath from property
    if (dMapFilePath.empty())
    {
    	ERROR_STREAM << "init_device: The property dMapFilePath is empty"<<std::endl;
    	set_state(Tango::FAULT);
    	set_status("The property dMapFilePath is not configured");
    	return;
    }

   	DEBUG_STREAM << "dMapFilePath: "<<dMapFilePath<<std::endl;
   	try
   	{
   		ChimeraTK::setDMapFilePath(dMapFilePath);
   	}
    catch (ChimeraTK::logic_error& e) {
      ERROR_STREAM << "init_device: "<< e.what()<<std::endl;
      set_state(Tango::FAULT);
      set_status(e.what());
      return;
    }

	tangoAdapter = new ChimeraTK::TangoAdapter(this, attributList);
	if (tangoAdapter==nullptr)
	{
      set_state(Tango::FAULT);
      set_status("Can not create TangoAdapter");
      return;
	}

	DEBUG_STREAM << "ChimeraTKExample2::init_device() end of init_device " << std::endl;
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::init_device
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::get_device_property()
 *	Description : Read database to initialize property data members.
 */
//--------------------------------------------------------
void TestChimeraTK2::get_device_property()
{
	/*----- PROTECTED REGION ID(TestChimeraTK2::get_device_property_before) ENABLED START -----*/

	//	Initialize property data members

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::get_device_property_before


	//	Read device properties from database.
	Tango::DbData	dev_prop;
	dev_prop.push_back(Tango::DbDatum("AttributList"));
	dev_prop.push_back(Tango::DbDatum("DMapFilePath"));

	//	is there at least one property to be read ?
	if (dev_prop.size()>0)
	{
		//	Call database and extract values
		if (Tango::Util::instance()->_UseDb==true)
			get_db_device()->get_property(dev_prop);

		//	get instance on TestChimeraTK2Class to get class property
		Tango::DbDatum	def_prop, cl_prop;
		TestChimeraTK2Class	*ds_class =
			(static_cast<TestChimeraTK2Class *>(get_device_class()));
		int	i = -1;

		//	Try to initialize AttributList from class property
		cl_prop = ds_class->get_class_property(dev_prop[++i].name);
		if (cl_prop.is_empty()==false)	cl_prop  >>  attributList;
		else {
			//	Try to initialize AttributList from default device value
			def_prop = ds_class->get_default_device_property(dev_prop[i].name);
			if (def_prop.is_empty()==false)	def_prop  >>  attributList;
		}
		//	And try to extract AttributList value from database
		if (dev_prop[i].is_empty()==false)	dev_prop[i]  >>  attributList;

		//	Try to initialize DMapFilePath from class property
		cl_prop = ds_class->get_class_property(dev_prop[++i].name);
		if (cl_prop.is_empty()==false)	cl_prop  >>  dMapFilePath;
		else {
			//	Try to initialize DMapFilePath from default device value
			def_prop = ds_class->get_default_device_property(dev_prop[i].name);
			if (def_prop.is_empty()==false)	def_prop  >>  dMapFilePath;
		}
		//	And try to extract DMapFilePath value from database
		if (dev_prop[i].is_empty()==false)	dev_prop[i]  >>  dMapFilePath;

	}

	/*----- PROTECTED REGION ID(TestChimeraTK2::get_device_property_after) ENABLED START -----*/

	//	Check device property data members init

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::get_device_property_after
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::always_executed_hook()
 *	Description : method always executed before any command is executed
 */
//--------------------------------------------------------
void TestChimeraTK2::always_executed_hook()
{
	//DEBUG_STREAM << "TestChimeraTK2::always_executed_hook()  " << device_name << std::endl;
	/*----- PROTECTED REGION ID(TestChimeraTK2::always_executed_hook) ENABLED START -----*/

	//	code always executed before all requests

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::always_executed_hook
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::read_attr_hardware()
 *	Description : Hardware acquisition for attributes
 */
//--------------------------------------------------------
void TestChimeraTK2::read_attr_hardware(TANGO_UNUSED(std::vector<long> &attr_list))
{
	//DEBUG_STREAM << "TestChimeraTK2::read_attr_hardware(vector<long> &attr_list) entering... " << std::endl;
	/*----- PROTECTED REGION ID(TestChimeraTK2::read_attr_hardware) ENABLED START -----*/

	//	Add your own code

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::read_attr_hardware
}


//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::add_dynamic_attributes()
 *	Description : Create the dynamic attributes if any
 *                for specified device.
 */
//--------------------------------------------------------
void TestChimeraTK2::add_dynamic_attributes()
{
	/*----- PROTECTED REGION ID(TestChimeraTK2::add_dynamic_attributes) ENABLED START -----*/

	//	Add your own code to create and add dynamic attributes if any

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::add_dynamic_attributes
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK2::add_dynamic_commands()
 *	Description : Create the dynamic commands if any
 *                for specified device.
 */
//--------------------------------------------------------
void TestChimeraTK2::add_dynamic_commands()
{
	/*----- PROTECTED REGION ID(TestChimeraTK2::add_dynamic_commands) ENABLED START -----*/

	//	Add your own code to create and add dynamic commands if any

	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::add_dynamic_commands
}

/*----- PROTECTED REGION ID(TestChimeraTK2::namespace_ending) ENABLED START -----*/

//	Additional Methods

/*----- PROTECTED REGION END -----*/	//	TestChimeraTK2::namespace_ending
} //	namespace
