//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "IECore/Profile.h"

#include "tbb/enumerable_thread_specific.h"

#include "boost/container/small_vector.hpp"
#include "boost/variant.hpp"

#include <stack>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <ios>



using namespace IECore;

namespace
{

	struct Event
	{
		Event( const IECore::InternedString &name, int id ) :
				m_start( std::chrono::high_resolution_clock::now() ),
				m_name( name ),
				m_id( id )
		{
		}

		void start()
		{
			m_start =  std::chrono::high_resolution_clock::now();
		}

		void end()
		{
			m_end = std::chrono::high_resolution_clock::now();
		}


		typedef boost::variant<IECore::InternedString, int, float> ArgValue;
		struct KeyValue
		{
			KeyValue(const InternedString& key, float value)
			{
				_key = key;
				_value = value;
			}

			KeyValue(const InternedString& key, int value)
			{
				_key = key;
				_value = value;
			}

			KeyValue(const InternedString& key, const InternedString &value)
			{
				_key = key;
				_value = value;
			}
			IECore::InternedString _key;
			ArgValue _value;
		};

		boost::container::small_vector<KeyValue, 8> args;

		std::chrono::high_resolution_clock::time_point m_start;
		std::chrono::high_resolution_clock::time_point m_end;
		IECore::InternedString m_name;
		int m_id;
	};

	typedef std::stack<Event> EventStack;
	typedef tbb::enumerable_thread_specific<EventStack, tbb::cache_aligned_allocator<EventStack>, tbb::ets_key_per_instance> ThreadSpecificEventStack;

	typedef std::vector<Event> EventVector;
	typedef tbb::enumerable_thread_specific<EventVector, tbb::cache_aligned_allocator<EventVector>, tbb::ets_key_per_instance> ThreadSpecificEventVector;

	static ThreadSpecificEventStack g_threadEventStack;

	static ThreadSpecificEventVector g_threadEvents;
}
namespace IECore
{
namespace Profile
{

void pushRegion( const IECore::InternedString &label, int id)
{
	g_threadEventStack.local().push( Event( label, id ) );
}

void popRegion()
{
	EventStack &eventStack = g_threadEventStack.local();
	Event e = eventStack.top();
	e.end();

	g_threadEvents.local().push_back( e );
	eventStack.pop();
}

IECORE_API void startRegion()
{
	EventStack &eventStack = g_threadEventStack.local();
	eventStack.top().start();
}

void addArg( const IECore::InternedString &key, int value)
{
	EventStack &eventStack = g_threadEventStack.local();
	eventStack.top().args.push_back( Event::KeyValue (key, value));

}

void addArg( const IECore::InternedString &key, float value)
{
	EventStack &eventStack = g_threadEventStack.local();
	eventStack.top().args.push_back( Event::KeyValue (key, value));
}

void addArg( const IECore::InternedString &key, const IECore::InternedString& value)
{
	EventStack &eventStack = g_threadEventStack.local();
	eventStack.top().args.push_back( Event::KeyValue (key, value));
}


void writeProfile( const std::string &fileName )
{

	std::ofstream f (fileName.c_str(), std::ios_base::out );

	f << "{ \"traceEvents\": [";


	bool first = true;

	for (ThreadSpecificEventVector::const_iterator i = g_threadEvents.begin();
		 i != g_threadEvents.end(); ++i)
	{
		const EventVector &events = *i;

		for (const auto &e : events)
		{

			if ( !first )
			{
				f << ", ";
			}

			f << std::endl;

			f << "{ \"pid\":1 , \"ph\":\"X\", \"tid\": " << e.m_id << ", "
					  << "\"ts\": " << std::chrono::duration_cast<std::chrono::microseconds>(e.m_start.time_since_epoch()).count() << ","
					  << "\"dur\": " << std::chrono::duration_cast<std::chrono::microseconds>((e.m_end - e.m_start)).count() << ", "
					 << "\"args\": {";

					bool firstArg = true;
		 			for (const auto &a : e.args)
					{
						if (!firstArg)
						{
							f << ", ";
						}

						if (const IECore::InternedString *p = boost::get<IECore::InternedString>(&a._value))
						{
							f << "\"" << a._key.string() << "\" : \"" << p->string() << "\"";
							firstArg = false;
						}


					}
		  			 f << "},"
					  << "\"name\": \"" << e.m_name << "\"}";

			first = false;
		}


	}

	f << "] }";
}

} // Profile
} // IECore


