/*----- PROTECTED REGION ID(TestChimeraTK.cpp) ENABLED START -----*/
//=============================================================================
//
// file :        TestChimeraTK.cpp
//
// description : C++ source for the TestChimeraTK class and its commands.
//               The class is derived from Device. It represents the
//               CORBA servant object which will be accessed from the
//               network. All commands which can be executed on the
//               TestChimeraTK are implemented in this file.
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


#include "TestChimeraTK.h"
#include "TestChimeraTKClass.h"


#include <TangoAdapter.h>

#include "MyApplication.h"
static ExampleApp theExampleApp;
/*----- PROTECTED REGION END -----*/	//	TestChimeraTK.cpp

/**
 *  TestChimeraTK class description:
 *    Test
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

namespace TestChimeraTK_ns
{
/*----- PROTECTED REGION ID(TestChimeraTK::namespace_starting) ENABLED START -----*/

//	static initializations

/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::namespace_starting

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::TestChimeraTK()
 *	Description : Constructors for a Tango device
 *                implementing the classTestChimeraTK
 */
//--------------------------------------------------------
TestChimeraTK::TestChimeraTK(Tango::DeviceClass *cl, string &s)
 : TANGO_BASE_CLASS(cl, s.c_str())
{
	/*----- PROTECTED REGION ID(TestChimeraTK::constructor_1) ENABLED START -----*/
	init_device();
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::constructor_1
}
//--------------------------------------------------------
TestChimeraTK::TestChimeraTK(Tango::DeviceClass *cl, const char *s)
 : TANGO_BASE_CLASS(cl, s)
{
	/*----- PROTECTED REGION ID(TestChimeraTK::constructor_2) ENABLED START -----*/
	init_device();
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::constructor_2
}
//--------------------------------------------------------
TestChimeraTK::TestChimeraTK(Tango::DeviceClass *cl, const char *s, const char *d)
 : TANGO_BASE_CLASS(cl, s, d)
{
	/*----- PROTECTED REGION ID(TestChimeraTK::constructor_3) ENABLED START -----*/
	init_device();
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::constructor_3
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::delete_device()
 *	Description : will be called at device destruction or at init command
 */
//--------------------------------------------------------
void TestChimeraTK::delete_device()
{
	DEBUG_STREAM << "TestChimeraTK::delete_device() " << device_name << endl;
	/*----- PROTECTED REGION ID(TestChimeraTK::delete_device) ENABLED START -----*/
	
	//	Delete device allocated objects
	delete adapter;
	//delete theExampleApp;
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::delete_device
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::init_device()
 *	Description : will be called at device initialization.
 */
//--------------------------------------------------------
void TestChimeraTK::init_device()
{
	DEBUG_STREAM << "TestChimeraTK::init_device() create device " << device_name << endl;
	/*----- PROTECTED REGION ID(TestChimeraTK::init_device_before) ENABLED START -----*/
	
	//	Initialization before get_device_property() call
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::init_device_before
	

	//	Get the device properties from database
	get_device_property();
	
	/*----- PROTECTED REGION ID(TestChimeraTK::init_device) ENABLED START -----*/
	
	//	Initialize device
    //set DMapFilePath from property
    if (!dMapFilePath.empty())
    {
        ChimeraTK::setDMapFilePath(dMapFilePath);
    }


	adapter = new ChimeraTK::TangoAdapter(this, attributList);


	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::init_device
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::get_device_property()
 *	Description : Read database to initialize property data members.
 */
//--------------------------------------------------------
void TestChimeraTK::get_device_property()
{
	/*----- PROTECTED REGION ID(TestChimeraTK::get_device_property_before) ENABLED START -----*/
	
	//	Initialize property data members
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::get_device_property_before


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
	
		//	get instance on TestChimeraTKClass to get class property
		Tango::DbDatum	def_prop, cl_prop;
		TestChimeraTKClass	*ds_class =
			(static_cast<TestChimeraTKClass *>(get_device_class()));
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

	/*----- PROTECTED REGION ID(TestChimeraTK::get_device_property_after) ENABLED START -----*/
	
	//	Check device property data members init
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::get_device_property_after
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::always_executed_hook()
 *	Description : method always executed before any command is executed
 */
//--------------------------------------------------------
void TestChimeraTK::always_executed_hook()
{
	DEBUG_STREAM << "TestChimeraTK::always_executed_hook()  " << device_name << endl;
	/*----- PROTECTED REGION ID(TestChimeraTK::always_executed_hook) ENABLED START -----*/
	
	//	code always executed before all requests
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::always_executed_hook
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::read_attr_hardware()
 *	Description : Hardware acquisition for attributes
 */
//--------------------------------------------------------
void TestChimeraTK::read_attr_hardware(TANGO_UNUSED(vector<long> &attr_list))
{
	DEBUG_STREAM << "TestChimeraTK::read_attr_hardware(vector<long> &attr_list) entering... " << endl;
	/*----- PROTECTED REGION ID(TestChimeraTK::read_attr_hardware) ENABLED START -----*/
	
	//	Add your own code
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::read_attr_hardware
}


//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::add_dynamic_attributes()
 *	Description : Create the dynamic attributes if any
 *                for specified device.
 */
//--------------------------------------------------------
void TestChimeraTK::add_dynamic_attributes()
{
	/*----- PROTECTED REGION ID(TestChimeraTK::add_dynamic_attributes) ENABLED START -----*/
	
	//	Add your own code to create and add dynamic attributes if any
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::add_dynamic_attributes
}

//--------------------------------------------------------
/**
 *	Method      : TestChimeraTK::add_dynamic_commands()
 *	Description : Create the dynamic commands if any
 *                for specified device.
 */
//--------------------------------------------------------
void TestChimeraTK::add_dynamic_commands()
{
	/*----- PROTECTED REGION ID(TestChimeraTK::add_dynamic_commands) ENABLED START -----*/
	
	//	Add your own code to create and add dynamic commands if any
	
	/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::add_dynamic_commands
}

/*----- PROTECTED REGION ID(TestChimeraTK::namespace_ending) ENABLED START -----*/

//	Additional Methods

/*----- PROTECTED REGION END -----*/	//	TestChimeraTK::namespace_ending
} //	namespace
